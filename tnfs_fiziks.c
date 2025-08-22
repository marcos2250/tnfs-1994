/*
 * The "Fiziks" module
 * 2D car handling dynamics
 */
#include <stdio.h>
#include "tnfs_math.h"
#include "tnfs_base.h"
#include "tnfs_collision_2d.h"
#include "tnfs_collision_3d.h"

// settings/flags
int is_recording_replay = 1;
int general_flags = 0x14; //800dc5a0 DAT_0014383c
int selected_track = 3; //rusty springs
int g_live_player_glue = 0; //8010d2f4
int DAT_0016707c = 0; //8010d30c 0016707c

int DAT_8010c478 = 0; //DAT_00144f10

// debug stats flags
int stats_braking_init_time = 0;
int stats_braking_final_time = 0;
int stats_timer_a = 0;
int stats_timer_b = 0;
int stats_timer_c = 0;
int stats_init_time = 0;
int stats_init_track_slice = 0;
int DAT_800f62c0 = 0;
int is_performance_test_off = 0; //DAT_00146483

void tnfs_record_best_acceleration(int a, int b, int c, int s) {
	if (a > 0)
		printf("Best 0-60 acceleration: %.2f\n", ((float) a) / 30);
	else if (b > 0)
		printf("Best 0-100 acceleration: %.2f\n", ((float) b) / 30);
	else if (c > 0)
		printf("Quarter mile time: %.2f (%d m/s)\n", (((float) c) / 30), (s >> 16));
}

void tnfs_record_best_braking(int a, int b) {
	if (a > 0)
		printf("\nBest 60-0 braking: %.2f\n", ((float) a) / 30);
	else if (b > 0)
		printf("\nBest 80-0 braking: %.2f\n", ((float) b) / 30);
}


void tnfs_engine_rev_limiter(tnfs_car_data *car) {
	tnfs_car_specs *specs;
	int max_rpm;

	specs = car->car_specs_ptr;

	if (car->is_engine_cutoff) {
		car->throttle = 0;
	}

	// speed to RPM
	car->rpm_vehicle = fixmul(fixmul(specs->gear_ratio_table[car->gear_selected + 2], specs->mps_to_rpm_factor), car->speed_drivetrain) >> 16;

	if (car->rpm_vehicle < 700)
		car->rpm_vehicle = 700;

	max_rpm = (specs->rpm_redline * car->throttle) >> 8;

	if (car->is_gear_engaged) {
		if (car->rpm_vehicle > car->rpm_engine) {
			car->rpm_engine += specs->clutchDropRpmInc;
			if (car->rpm_engine > car->rpm_vehicle)
				car->rpm_engine = car->rpm_vehicle;
		} else {
			if ((car->tire_skid_rear & 2) && car->throttle > 220) {
				car->rpm_engine -= fix3(specs->clutchDropRpmDec);
			} else {
				car->rpm_engine -= specs->clutchDropRpmDec;
			}
			if (car->rpm_engine < car->rpm_vehicle)
				car->rpm_engine = car->rpm_vehicle;
		}
	} else if (car->throttle >= 13) {
		if (car->gear_selected == -1) {
			car->rpm_engine += car->car_specs_ptr->gasRpmInc * car->throttle / 2 >> 8;
		} else {
			car->rpm_engine += car->car_specs_ptr->gasRpmInc * car->throttle >> 8;
		}
		if (car->rpm_engine > max_rpm)
			car->rpm_engine = max_rpm;
	} else if (specs->rpm_idle > car->rpm_engine) {
		if (car->gear_selected == -1) {
			car->rpm_engine += car->car_specs_ptr->noGasRpmDec >> 1;
		} else {
			car->rpm_engine += car->car_specs_ptr->noGasRpmDec;
		}
		if (car->rpm_engine > specs->rpm_idle) {
			car->rpm_engine = specs->rpm_idle;
		}
	} else {
		if (car->gear_selected == -1) {
			car->rpm_engine -= car->car_specs_ptr->noGasRpmDec >> 1;
		} else {
			car->rpm_engine -= car->car_specs_ptr->noGasRpmDec >> 1;
		}
		if (car->rpm_engine < specs->rpm_idle) {
			car->rpm_engine = specs->rpm_idle;
		}
	}

	if (car->gear_auto_selected != 0 && car->gear_selected == 0 && car->rpm_engine < specs->rpm_idle) {
		car->rpm_engine = specs->rpm_idle;
	}
	if (car->rpm_engine > 11000) {
		car->rpm_engine = 11000;
	}
	if (car->rpm_vehicle > 11000) {
		car->rpm_vehicle = 11000;
	}
}

void tnfs_engine_auto_shift_change(tnfs_car_data *car_data, tnfs_car_specs *car_specs) {
	int gear;
	int rpm_vehicle;

	gear = car_data->gear_selected;

	// speed to RPM
	rpm_vehicle = fixmul(fixmul(car_specs->gear_ratio_table[gear + 2], car_specs->mps_to_rpm_factor), car_data->speed_local_lon) >> 16;

	if (gear < (car_specs->number_of_gears - 3) && rpm_vehicle > car_specs->gear_upshift_rpm[gear]) {
		// upshift
		car_data->gear_selected++;
	} else if (gear > 0 && rpm_vehicle < car_specs->gear_upshift_rpm[gear - 1] / 2) {
		// downshift
		car_data->gear_selected--;
	}
}

void tnfs_engine_auto_shift_control(tnfs_car_data *car) {
	int gear;
	if (car->is_shifting_gears > 0) {
		car->is_shifting_gears--;
	}
	if (car->is_shifting_gears == 0) {
		car->throttle = car->throttle_previous_pos;
		car->is_engine_cutoff = 0;
		car->is_gear_engaged = 1;
		car->is_shifting_gears = -1;
	}
	if (car->is_shifting_gears < 0) {
		gear = car->gear_selected;
		tnfs_engine_auto_shift_change(car, car->car_specs_ptr);
		if (car->gear_selected != gear) {
			car->throttle_previous_pos = car->throttle;
			car->is_engine_cutoff = 1;
			car->is_gear_engaged = 0;
			if (car->car_model_id == 4 && (iSimTimeClock & 0x31) == 16) { // F512TR
				car->is_shifting_gears = car->car_specs_ptr->shift_timer + 3;
			} else {
				car->is_shifting_gears = car->car_specs_ptr->shift_timer;
			}
		}
	}
}

int tnfs_engine_torque(tnfs_car_specs *specs, int rpm) {
	int iVar1;
	int uVar2;

	uVar2 = math_div_int(200, rpm + 100);
	uVar2 = math_div_int(200, rpm - uVar2 + 100 - specs->torque_table[0]);

	iVar1 = abs(uVar2);
	if (specs->torque_table_entries <= iVar1) {
		iVar1 = specs->torque_table_entries - 1;
	}
	return specs->torque_table[iVar1 * 2 + 1];
}

int tnfs_engine_thrust(tnfs_car_data *car) {
	int torque;
	int thrust;
	int max_rpm;
	int tireslip;
	int gear;
	tnfs_car_specs *specs;

	specs = car->car_specs_ptr;

	gear = car->gear_selected + 2;

	if (car->is_gear_engaged) {
		if (car->rpm_engine >= car->rpm_vehicle) {
			// acceleration
			max_rpm = car->throttle * specs->rpm_redline >> 8;

			if (car->rpm_engine <= max_rpm) {
				// engine rpm range
				torque = tnfs_engine_torque(specs, car->rpm_engine);
				torque = fixmul(torque, specs->gear_ratio_table[gear]);

				if (torque > 0x70000)
					torque = 0x70000;

				tireslip = math_div_int(car->rpm_engine << 16,
						fixmul(specs->gear_ratio_table[gear], specs->mps_to_rpm_factor) >> 16);

				tireslip -= car->speed_local_lon;

				if (abs(tireslip) > 0x30000 && car->throttle > 250 && car->gear_selected < 1) {
					// engine overpower, doing burnouts
					thrust = tireslip;
					is_drifting = 1;
				} else {
					thrust = (car->throttle * torque) >> 8;
				}

			} else {
				// decceleration
				torque = (car->rpm_engine - max_rpm) * specs->negTorque;
				torque = abs(torque);
				thrust = fixmul(specs->gear_ratio_table[gear], torque);
				thrust = -abs(thrust);

				if (car->speed_local_lon == 0) {
					thrust = 0;
				} else if (thrust > abs(car->speed_local_lon) * 16) {
					thrust = car->speed_local_lon * -16;
				} else if (car->speed_local_lon < 0) {
					thrust = -thrust;
				}
			}
		} else {
			// decceleration
			thrust = fixmul(specs->gear_ratio_table[gear],  specs->negTorque * (car->rpm_vehicle - car->rpm_engine) * -4);
		}
	} else {
		// neutral
		thrust = 0;
	}

	// tire slip
	if ((car->throttle > 0xf0) // full throttle
			&& (abs(thrust) > car->tire_grip_rear - car->tire_grip_loss) // tire grip slipping
			&& (car->rpm_engine < car->car_specs_ptr->gear_upshift_rpm[2] - 500) //) { // before cut off
			&& (car->car_specs_ptr->front_drive_percentage == 0)) { // RWD car ??

		// RPM to speed
		car->speed_drivetrain = math_div_int(car->rpm_engine << 16, fixmul(specs->gear_ratio_table[gear], specs->mps_to_rpm_factor) >> 16);

		// wheel spin faster than car speed
		if (car->speed_drivetrain > abs(car->speed_local_lon)) {
			return thrust;
		}
	}

	car->speed_drivetrain = car->speed_local_lon;
	return thrust;
}

int tnfs_drag_force(tnfs_car_data *car, signed int speed) {
	int max;
	int drag;
	int sq_speed;

	if (road_surface_type_array[car->surface_type].is_unpaved)
		car->surface_type_b = 4;
	else
		car->surface_type_b = 0;

	sq_speed = speed >> 16;
	sq_speed *= sq_speed;

	drag = fix8(road_surface_type_array[car->surface_type].velocity_drag * car->car_specs_ptr->drag) * sq_speed;

	drag += road_surface_type_array[car->surface_type].surface_drag;

	max = abs(speed * car->fps);
	if (drag > max)
		drag = max;

	if (speed > 0)
		drag = -drag;

	return drag;
}

/*
 * crashing cars cheat code, from 3DO version
 */
void tnfs_cheat_crash_cars() {
	int i;
	if (cheat_crashing_cars == 4) {
		for (i = 1; i < g_total_cars_in_scene; i++) {
			tnfs_collision_rollover_start(g_car_ptr_array[i], 0xa0000, 0xa0000, 0xa0000);
		}
	}
}

void tnfs_tire_forces_locked(int *force_lat, int *force_lon, signed int max_grip, int *slide, int braking) {
	int abs_f_lon;
	int abs_f_lat;
	int force;
	int r0, r2;

	// total force
	abs_f_lon = abs(*force_lon);
	abs_f_lat = abs(*force_lat);
	if (abs_f_lon <= abs_f_lat)
		force = (abs_f_lon >> 2) + abs_f_lat;
	else
		force = (abs_f_lat >> 2) + abs_f_lon;

	// calculate resulting forces
	if (force > max_grip) {
		*slide = force - max_grip;
		r2 = max_grip * 5;
		r0 = fix2(r2);
		if (braking > r0) {
			braking = r0;
		}
		r0 = math_div_int(max_grip * 2 - braking, fix8(force));

		/*
		r0 = braking;
		r1 = force;
		r3 = max_grip;
        r2 = force - max_grip; 		// sub        r2,r1,r3
        *slide = r2;				// str        r2,[r12,#0x0]
        r2 = max_grip * 5;			// adds       r2,r3,r3, lsl #0x2
        if (r2 < 0) r2 += 3;		// addmi      r2,r2,#0x3
        r2 >>= 2;					// mov        r2,r2, asr #0x2
        if (r0 > r2) 				// cmp        r0,r2
        	r0 = r2;				// movgt      r0,r2
        r0 -= r3;					// sub        r0,r0,r3
        r3 -= r0;					// sub        r3,r3,r0
        r0 = r1;					// movs       r0,r1
        if (r0 < 0) r0 += 0xff;		// addmi      r0,r0,#0xff
        r0 >>= 8;					// mov        r0,r0, asr #0x8
        r1 = r3;					// mov        r1,r3
        r0 = math_div_int(r1, r0); 	// bl math_div_int
        */

		*force_lat = fix8(r0 * *force_lat);
		*force_lon = fix8(r0 * *force_lon);
	}
}


/*
 * read grip table for a given slip angle, 2 x 512 bytes
 */
int tnfs_tire_slide_table(tnfs_car_data *car, int slip_angle, int is_rear_wheels) {
	if (slip_angle > 0x1ffffe)
		slip_angle = 0x1ffffe;

	return car->car_specs_ptr->grip_table[(is_rear_wheels * 512) + (slip_angle >> 12)] << 9;
}


void tnfs_tire_limit_max_grip(tnfs_car_data *car_data, //
		int *force_lat, signed int *force_lon, signed int max_grip, int *slide, int grip_factor) {

	int f_lat_abs;
	int f_lon_abs;
	int max_grip2;
	int s_grip;
	int factor;
	int grip_loss;
	int f_total;
	int lat_grip;

	lat_grip = fix8(max_grip * grip_factor);
	max_grip2 = max_grip + lat_grip;
	grip_loss = 0;

	// total force
	f_lat_abs = abs(*force_lat);
	f_lon_abs = abs(*force_lon);
	f_total = f_lat_abs + f_lon_abs;

	// TCS
	if (car_data->abs_on || car_data->tcs_on) {
		if (f_lon_abs > max_grip) {
			if (*force_lon < 0) {
				if (car_data->tcs_on == 0) {
					*force_lon = -max_grip;
					f_lon_abs = max_grip - 1;
				} else {
					*force_lon = -max_grip / 2;
					f_lon_abs = max_grip / 2;
				}
			} else {
				*force_lon = max_grip;
				if (car_data->tcs_on == 0) {
					f_lon_abs = max_grip - 1;
				} else {
					f_lon_abs = max_grip / 2;
				}
			}
		}

		if (f_lat_abs > lat_grip) {
			if (*force_lat < 0) {
				*force_lat = -lat_grip;
			} else {
				*force_lat = lat_grip;
			}
			f_lat_abs = lat_grip - 1;
		}
		f_total = f_lat_abs + f_lon_abs;
	}

	if (f_total > max_grip2) {
		// power drift or locked wheels
		if (car_data->throttle < 0xf1) {
			*slide = (f_total - max_grip2) | *slide;
		} else {
			*slide = (f_total - max_grip2) * 0x80;
			s_grip = f_total / 2;
			if (*slide > s_grip) { //???
				*slide = s_grip;
			}
		}

		s_grip = f_total;
		if (f_total > fix3(max_grip2 * 0xb)) {
			s_grip = fix3(max_grip2 * 0xb);
		}
		grip_loss = s_grip - max_grip2;

		factor = math_div_int(max_grip2 - grip_loss, f_total >> 8);

		*force_lat = fix8(factor * *force_lat);
		*force_lon = fix8(factor * *force_lon);

	} else {
		if (f_lat_abs > lat_grip) {
			*slide = f_lat_abs - lat_grip;

			s_grip = fix3(lat_grip * 0xb);
			if (f_lat_abs > s_grip) {
				f_lat_abs = s_grip;
			}
			f_lat_abs = lat_grip * 2 - f_lat_abs;
			if (*force_lat < 0) {
				f_lat_abs = -f_lat_abs;
			}
			*force_lat = f_lat_abs;
		}

		if (f_lon_abs > max_grip) {
			*slide = *slide | (f_lon_abs - max_grip) * 0x80;

			s_grip = fix3(max_grip * 0xb);
			if (f_lon_abs > s_grip) {
				f_lon_abs = s_grip;
			}

			grip_loss = f_lon_abs - max_grip;
			if (*force_lon < 0) {
				*force_lon = -(max_grip - grip_loss);
			} else {
				*force_lon = max_grip - grip_loss;
			}
		}
	}


	if (&car_data->slide_rear == slide) {
		if ((*force_lon > 0 && car_data->speed_local_lon > 0) || (*force_lon < 0 && car_data->speed_local_lon < 0)) {
			car_data->tire_grip_loss = grip_loss;
		}
	}
}

void tnfs_tire_forces(tnfs_car_data *car, //
		int *_result_Lat, int *_result_Lon, //
		int force_Lat, int force_Lon, //
		signed int steering, int thrust_force, int braking_force, //
		int is_front_wheels) {

	unsigned int slip_angle;
	int f_lat_loc_abs2;
	int f_lon_loc_abs2;
	int grip_force;
	int *slide;
	char *skid;
	int max_grip;
	int max_grip_factor;
	int skid_amount;
	int force_lon_local;
	int force_lat_local;
	int result_brake_thrust;
	int slip_angle_grip;
	int grip_factor;
	int grip_factor_inv;
	int force_lat_local_0;
	int force_lon_local_0;
	int aux;

	skid_amount = 0;
	if (car->time_off_ground > 0) {
		*_result_Lon = 0;
		*_result_Lat = 0;
	} else {

		if (is_front_wheels == 1) {
			math_rotate_2d(force_Lat, force_Lon, -steering, &force_lat_local, &force_lon_local);
			max_grip = car->tire_grip_front;
			skid = &car->tire_skid_front;
			slide = &car->slide_front;
			grip_factor = car->car_specs_ptr->frontGripMult;
			grip_factor_inv = car->car_specs_ptr->frontGripMult_inv;
		} else {
			force_lat_local = force_Lat;
			force_lon_local = force_Lon;
			max_grip = car->tire_grip_rear;
			skid = &car->tire_skid_rear;
			slide = &car->slide_rear;
			grip_factor = car->car_specs_ptr->rearGripMult;
			grip_factor_inv = car->car_specs_ptr->rearGripMult_inv;
		}
		force_lat_local_0 = force_lat_local;
		force_lon_local_0 = force_lon_local;
		*skid = 0;

		slip_angle = math_atan2(abs(force_lon_local), abs(force_lat_local));

		if (slip_angle > car->car_specs_ptr->cutoff_slip_angle) {
			slip_angle = car->car_specs_ptr->cutoff_slip_angle;
			*skid |= 1;
		}

		if (max_grip < abs(braking_force) //
				&& ( car->abs_enabled == 0 || car->handbrake != 0 )) {

			// locked wheels: hard braking or handbrake
			if (abs(force_lon_local) > max_grip)
				*skid |= 1;

			tnfs_tire_forces_locked(&force_lat_local, &force_lon_local, max_grip, slide, braking_force);

		} else {
			// not locked wheels

			max_grip_factor = max_grip * grip_factor;

			// lateral tire grip factor
			slip_angle_grip = math_mul(tnfs_tire_slide_table(car, slip_angle, is_front_wheels - 1),
					fix8(max_grip_factor));

			if (abs(force_lat_local) > abs(slip_angle_grip)) {
				//lateral force exceeding max grip

				// for skid sounds/visual effects
				grip_force = max_grip; //???
				if (abs(force_lat_local) > grip_force && slip_angle > 0xf0000) {
					if (slip_angle <= 0x1E0000) {
						skid_amount = fix2(abs(force_lat_local) - grip_force);
						*skid |= 4;
					} else {
						*skid |= 1;
						skid_amount = abs(force_lat_local) - grip_force;
					}
				}

				if (force_lat_local <= 0)
					force_lat_local = -slip_angle_grip;
				else
					force_lat_local = slip_angle_grip;

			}

			if (force_lon_local <= 0)
				braking_force = -braking_force;

			result_brake_thrust = braking_force + thrust_force;

			if (abs(thrust_force) > abs(braking_force)) {
				//acceleration
				force_lon_local = result_brake_thrust;
				f_lon_loc_abs2 = abs(force_lon_local);
				f_lat_loc_abs2 = abs(fixmul(force_lat_local, grip_factor_inv));
				//f_lat_loc_abs2 = abs(force_lat_local); //pc version

				if (f_lat_loc_abs2 + f_lon_loc_abs2 > max_grip) {

					*skid = 2;

					if (f_lon_loc_abs2 > max_grip) {
						if (is_drifting == 0 || car->tcs_on) {
							// powerslide

							/*
							int r0, r1, r2;
							r0 = f_lat_loc_abs2;
							r2 = max_grip * 7;		// rsbs       r2,r5,r5, lsl #0x3
							r1 = r2;				// mov        r1,r2
							if (r2 < 0) r2 += 3;	// addmi      r2,r2,#0x3
							r2 >>= 2;				// mov        r2,r2, asr #0x2
							if (r0 > r2) {			// cmp        r2,r0
													// bge        LAB_0001133c
							   r0 = r1;				// movs       r0,r1
							   if (r0 < 0) r0 += 3;	// addmi      r0,r0,#0x3
							   r0 >>= 2;			// mov        r0,r0, asr #0x2
							}						// LAB_0001133c:
							r0 = max_grip*2 - r0;	// rsb        r0,r0,r5, lsl #0x1
							r1 = r0 >> 8;			// mov        r1,r0, asr #0x8
							r0 = force_lat_local;	// ldr        r0,[sp,#result_Lat]
							r1 *= r0;				// mul        r1,r0,r1
							r0 = max_grip >> 8;		// mov        r0,r5, asr #0x8
							r0 = math_div_int(r1, r0);	// bl         math_div_int
							force_lat_local = r0;
							*/

       	   	   	   	   	  	aux = fix2(max_grip * 7);
							if (f_lon_loc_abs2 > aux) {
								f_lon_loc_abs2 = aux;
							}
							force_lat_local = math_div_int(((max_grip * 2 - f_lon_loc_abs2) >> 8) * force_lat_local, max_grip >> 8);

						} else {
							// drift
							force_lat_local = fix2(force_lat_local);

							// enhance donut spins (just 3DO version)
							if (car->rpm_engine > 4000 //
								&& abs(car->speed_local_lon) < 0xC0000 // low speeds
								&& abs(car->steer_angle) > 0x190000 //
								&& car->throttle > 0xF0 //
								&& car->gear_selected == 0) { //1st gear
									if (car->steer_angle < 0) {
										force_lat_local = -fix4(force_lon_local);
									} else {
										force_lat_local = fix4(force_lon_local);
									}
									if (car->tire_skid_front == *skid) { //????
										force_lat_local = fix2(force_lat_local);
									}
							}
						}
					}

					tnfs_tire_limit_max_grip(car, &force_lat_local, &force_lon_local, max_grip, slide, grip_factor);
				} else {
					tnfs_tire_limit_max_grip(car, &force_lat_local, &force_lon_local, max_grip, slide, grip_factor);
				}

				if (*slide != 0) {
					*slide <<= 3;
				}
				if (*slide < skid_amount) {
					*slide = skid_amount;
				}

			} else {
				//braking
				if (abs(result_brake_thrust) < abs(force_lon_local)) {
					if (force_lon_local <= 0) {
						force_lon_local = -abs(result_brake_thrust);
					} else {
						force_lon_local = abs(result_brake_thrust);
					}
				}

				int iVar4, iVar5, iVar6;
				iVar5 = abs(force_lat_local);
				iVar4 = grip_factor * force_lon_local;
				iVar4 = abs(fix8(iVar4));
				iVar6 = max_grip_factor * 5;
				iVar6 = fix10(iVar6);

				if (iVar6 < iVar5 + iVar4) {
					if (car->brake < 0x90 && car->abs_enabled) {
						tnfs_tire_limit_max_grip(car, &force_lat_local, &force_lon_local, max_grip, slide, grip_factor);
					} else {
						tnfs_tire_limit_max_grip(car, &force_lat_local_0, &force_lon_local_0, max_grip, slide, grip_factor);
					}
				}
				*slide |= skid_amount;
			}
		}

		// function return, rotate back to car frame
		if (steering != 0) {
			math_rotate_2d(force_lat_local, force_lon_local, steering, _result_Lat, _result_Lon);
		} else {
			*_result_Lat = force_lat_local;
			*_result_Lon = force_lon_local;
		}
	}
}


/*
 * Main physics routine
 */
void tnfs_physics_update(tnfs_car_data *car_data) {
	int braking_total;
	int sideslip;
	int weight_transfer;
	int angular_accel;
	int stats_elapsed_brake_time_1;
	int stats_elapsed_brake_time_2;
	int stats_elapsed_acc_time_1;
	int stats_elapsed_acc_time_2;
	int local_speed_lon;
	int local_speed_lat;
	int limit_tire_force;
	int abs_speed_x;
	int abs_speed_y;
	int speed_lon_rear;
	int speed_lat_rear;
	int speed_lon_front;
	int speed_lat_front;
	tnfs_stats_data *stats_data_ptr;
	int braking_rear;
	int braking_front;
	int traction_rear;
	int traction_front;
	int drag_lon;
	int drag_lat;
	int thrust_force;
	int accel_lon;
	int accel_lat;
	int ftire_lon_rear;
	int ftire_lat_rear;
	int ftire_lon_front;
	int ftire_lat_front;
	int speed_lon;
	int speed_lat;
	tnfs_car_specs *car_specs;
	int aux;
	int g_easteregg_0001277c = 0;

	stats_data_ptr = &g_stats_data;

	is_drifting = 0;
	car_specs = car_data->car_specs_ptr;

	// fast vec2 length
	abs_speed_x = abs(car_data->speed_x);
	abs_speed_y = abs(car_data->speed_z);
	if (abs_speed_x <= abs_speed_y)
		car_data->speed = (abs_speed_x >> 2) + abs_speed_y;
	else
		car_data->speed = (abs_speed_y >> 2) + abs_speed_x;

	// framerate fixed values
	car_data->delta_time = 2184;
	car_data->fps = 30;

	// TCS/ABS controls
	if ((general_flags & 0x10) != 0 && car_data->handbrake == 0) {
		car_data->abs_on = car_data->abs_enabled;
		car_data->tcs_on = car_data->tcs_enabled;
	} else {
		car_data->abs_on = 0;
		car_data->tcs_on = 0;
	}

	// gear shift control
	if ((general_flags & 4) || car_data->gear_selected == -1) {
		tnfs_engine_rev_limiter(car_data);
		if (car_data->gear_auto_selected > 0) { // automatic transmission
			switch (car_data->gear_auto_selected) {
			case 3:
				tnfs_engine_auto_shift_control(car_data);
				break;
			case 2:
				car_data->gear_selected = -1;
				break;
			case 1:
				car_data->gear_selected = -2;
				break;
			}
		}
	}

	//engine thrust
	car_data->thrust = tnfs_engine_thrust(car_data);
	thrust_force = car_data->thrust;

	car_data->tire_grip_loss = 0;

	// traction forces
	traction_front = fixmul(car_specs->front_drive_percentage, thrust_force);
	traction_rear = thrust_force - traction_front;

	// braking forces
	braking_total = fixmul(330 * car_data->brake, car_data->tire_grip_rear + car_data->tire_grip_front);
	braking_front = fixmul(car_specs->front_brake_percentage, braking_total);
	braking_rear = (braking_total - braking_front);

	//fixme??? only for 3do
	//if (fix3(car_data->tire_grip_front * 9) < braking_total || fix3(car_data->tire_grip_rear * 9) < braking_total) {
	//	car_data->field358_0x4a4 = 1;
	//} else {
	//	car_data->field358_0x4a4 = 0;
	//}

	// handbrake
	if (car_data->handbrake == 1) {
		braking_rear = car_specs->max_brake_force;
		tnfs_cheat_crash_cars();
	}

	// gear wheel lock
	if (abs(car_data->speed_local_lat) < 0x1999 //
	&& ((car_data->speed_local_lon > 0x10000 && car_data->gear_selected == -2) //
	|| (car_data->speed_local_lon < -0x10000 && car_data->gear_selected >= 0))) {
		if (traction_front != 0 && traction_front < car_data->tire_grip_front) {
			traction_front = 0;
			braking_front = car_specs->max_brake_force;
		}
		if (traction_rear != 0 && traction_rear < car_data->tire_grip_rear) {
			traction_rear = 0;
			braking_rear = car_specs->max_brake_force;
		}
	}


	//TCS
	if (car_data->throttle < 40) {
		car_data->tcs_on = 0;
	} else {
		if (car_data->tcs_on != 0) {
			if (0x70000 <= abs(thrust_force) && car_data->throttle > 83)
				car_data->throttle -= 12;
		}
	}
	if (car_data->brake < 40)
		car_data->abs_on = 0;

	// gear change delay
	if (car_data->gear_selected != car_data->gear_shift_current) {
		car_data->gear_shift_interval = 16;
		car_data->gear_shift_previous = car_data->gear_shift_current;
		car_data->gear_shift_current = car_data->gear_selected;
	}
	if (car_data->gear_shift_interval > 0) {
		if (car_data->gear_shift_interval > 12 || car_data->is_shifting_gears < 1)
			car_data->gear_shift_interval--;

		if (car_data->car_id2 == g_player_id //
		&& car_data->gear_shift_interval == 11 //
		&& selected_camera == 0 // dashboard view
		&& car_data->is_shifting_gears < 1) {
			//play gear shift sound
			tnfs_sfx_play(-1, 13, 0, 0, 1, 0xF00000);
		}
	}

	// aero/drag forces
	drag_lat = tnfs_drag_force(car_data, car_data->speed_local_lat);
	drag_lon = tnfs_drag_force(car_data, car_data->speed_local_lon);

	if (car_data->speed_local_lon > car_data->car_specs_ptr->top_speed) {
		if (drag_lon > 0 && drag_lon < thrust_force) {
			drag_lon = abs(thrust_force);
		} else if (drag_lon < 0) {
			if (abs(drag_lon) < abs(thrust_force)) {
				drag_lon = -abs(thrust_force);
			}
		}
	}

	// BEGIN of car traction/slip trajectory
	math_angle_wrap(car_data->steer_angle + car_data->angle.y);

	// convert to local frame of reference
	math_rotate_2d(car_data->speed_x, car_data->speed_z, -car_data->angle.y, &car_data->speed_local_lat, &car_data->speed_local_lon);

	// time scale speeds (m/s)
	speed_lat = car_data->fps * car_data->speed_local_lat;
	speed_lon = car_data->fps * car_data->speed_local_lon;

	// split front and rear moments
	// lateral speeds, front and rear
	aux = -math_mul(car_data->weight_distribution_front, speed_lat);
	sideslip = math_mul(car_data->wheel_base, car_data->fps * car_data->angular_speed);
	speed_lat_front = aux - (drag_lat / 2) + sideslip;
	speed_lat_rear = -(speed_lat + aux) - (drag_lat / 2) - sideslip;

	// longitudinal speeds, front and rear
	aux = -math_mul(car_data->weight_distribution_front, speed_lon);
	speed_lon_rear = -(speed_lon + aux) - (drag_lon / 2);
	speed_lon_front = aux - (drag_lon / 2);

	// tire forces (bicycle model)
	car_data->slide_front = 0;
	car_data->slide_rear = 0;

	tnfs_tire_forces(car_data, &ftire_lat_front, &ftire_lon_front, speed_lat_front, speed_lon_front, car_data->steer_angle, traction_front, braking_front, 1);
	tnfs_tire_forces(car_data, &ftire_lat_rear,  &ftire_lon_rear,  speed_lat_rear,  speed_lon_rear,  0,                     traction_rear,  braking_rear,  2);

	accel_lat = ftire_lat_rear + ftire_lat_front;
	accel_lon = ftire_lon_rear + ftire_lon_front;

	// limit braking forces
	if (car_data->brake > 100) {
		limit_tire_force = car_specs->max_brake_force;
		if (abs(accel_lon) > limit_tire_force) {
			if (accel_lon > 0) {
				accel_lon = limit_tire_force;
			} else {
				accel_lon = -limit_tire_force;
			}
		}
		limit_tire_force = limit_tire_force + limit_tire_force / 2;
		if (abs(accel_lat) > limit_tire_force) {
			if (accel_lat > 0) {
				accel_lat = limit_tire_force;
			} else {
				accel_lat = -limit_tire_force;
			}
		}
	}

	car_data->accel_lat = accel_lat;
	car_data->accel_lon = accel_lon;

	// convert speeds to world scale (m/s >> 16)
	if (abs(car_data->speed_local_lon) + abs(car_data->speed_local_lat) < 19660) {
		// car stopped
		car_data->speed_local_lat += math_mul(accel_lat, car_data->delta_time);
		car_data->speed_local_lon += math_mul(accel_lon, car_data->delta_time);

		if (car_data->gear_selected == -1 || car_data->throttle == 0) {
			car_data->speed_local_lon = 0;
			car_data->speed_local_lat = 0;
		}
	} else {
		// car moving
		car_data->speed_local_lat += math_mul((drag_lat + accel_lat + car_data->slope_force_lat), car_data->delta_time);
		car_data->speed_local_lon += math_mul((drag_lon + accel_lon + car_data->slope_force_lon), car_data->delta_time);
	}

	// rotate back to global frame of reference
	math_rotate_2d(car_data->speed_local_lat, car_data->speed_local_lon, car_data->angle.y, &car_data->speed_x, &car_data->speed_z);

	// move the car
	// 3DO version
	car_data->position.z += math_mul(car_data->speed_z, car_data->delta_time);
	car_data->position.x -= math_mul(car_data->speed_x, car_data->delta_time);

	// suspension body roll
	if (car_data->speed_local_lat + car_data->speed_local_lon > 6553) {
		car_data->body_roll += (-car_data->body_roll -fixmul(car_data->accel_lat, car_specs->body_roll_factor)) / 2;
		car_data->body_pitch += (-car_data->body_pitch -fixmul(car_data->accel_lon, car_specs->body_pitch_factor)) / 2;
		weight_transfer = fixmul(accel_lon, car_data->weight_transfer_factor);
	} else {
		car_data->body_roll += -car_data->body_roll / 2;
		car_data->body_pitch += -car_data->body_pitch / 2;
		weight_transfer = 0;
	}

	// calculate grip forces
	if (g_easteregg_0001277c == 0) {
		aux = road_surface_type_array[car_data->surface_type + car_data->surface_type_2].roadFriction * (car_data->front_friction_factor - weight_transfer);
		car_data->tire_grip_front = fix8(aux);
		aux = road_surface_type_array[car_data->surface_type + car_data->surface_type_2].roadFriction * (car_data->rear_friction_factor + weight_transfer);
		car_data->tire_grip_rear = fix8(aux);
	} else {
		aux = road_surface_type_array[car_data->surface_type].roadFriction * (car_data->front_friction_factor - weight_transfer);
		car_data->tire_grip_front = fix8(aux);
		aux = road_surface_type_array[car_data->surface_type].roadFriction * (car_data->rear_friction_factor + weight_transfer);
		car_data->tire_grip_rear = fix8(aux);
	}

	// body rotation torque
	angular_accel = math_mul(car_data->rear_yaw_factor, ftire_lat_rear) - math_mul(car_data->front_yaw_factor, ftire_lat_front);

	// apply body rotation
	car_data->angular_speed += math_mul(car_data->delta_time, angular_accel);

	// rotate car body (3DO version)
	car_data->angle.y += math_mul(car_data->delta_time, car_data->angular_speed);

	// wrap angle
	car_data->angle.y = math_angle_wrap(car_data->angle.y);

	// track fence collision
	tnfs_track_fence_collision(car_data);

	// replay recording
	if ((general_flags & 4) && is_recording_replay == 1) {
		if (car_data->speed_local_lon / 2 <= 0)
			local_speed_lon = car_data->speed_local_lon / -2;
		else
			local_speed_lon = car_data->speed_local_lon / 2;
		if (car_data->speed_local_lat <= 0)
			local_speed_lat = -car_data->speed_local_lat;
		else
			local_speed_lat = car_data->speed_local_lat;
		if (local_speed_lat > local_speed_lon && car_data->speed_local_lat > 0x8000)
			tnfs_replay_highlight_record(87);
		if ((car_data->slide_front || car_data->slide_rear) //
			&& car_data->speed > 0x140000 //
			&& ((car_data->tire_skid_rear & 1) || (car_data->tire_skid_front & 1))) {
				tnfs_replay_highlight_record(40);
		}
		if (DAT_800f62c0 == 0) {
			if (abs(car_data->speed_local_lon) > 3276) {
				tnfs_replay_highlight_record(120);
				DAT_800f62c0 = iSimTimeClock + 1;
			}
		}
	}

	// Performance test - only PC version and Rusty Springs track
	if (!is_performance_test_off && selected_track == 3) {
		if (car_data->track_slice <= 97 || car_data->track_slice >= 465) {
			if (car_data->speed_local_lon < 0x3333) {
				stats_init_time = iSimTimeClock;
				stats_init_track_slice = car_data->track_slice_lap;
				stats_timer_a = 99999;
				stats_timer_b = 99999;
				stats_timer_c = 99999;
			}
			stats_elapsed_acc_time_1 = iSimTimeClock - stats_init_time;
			if (car_data->throttle > 50 && stats_elapsed_acc_time_1 < 1500 && stats_elapsed_acc_time_1 > 100) {
				if (car_data->speed_local_lon > 1755447 && stats_timer_a > stats_elapsed_acc_time_1) {
					stats_timer_a = iSimTimeClock - stats_init_time;
					tnfs_record_best_acceleration(stats_elapsed_acc_time_1, 0, 0, 0);
					if (stats_data_ptr->best_accel_time_1 > stats_elapsed_acc_time_1)
						stats_data_ptr->best_accel_time_1 = stats_elapsed_acc_time_1;
				}
				if (car_data->speed_local_lon > 2926182 && stats_timer_b > stats_elapsed_acc_time_1) {
					stats_timer_b = stats_elapsed_acc_time_1;
					tnfs_record_best_acceleration(0, stats_elapsed_acc_time_1, 0, 0);
					if (stats_data_ptr->best_accel_time_2 > stats_elapsed_acc_time_1)
						stats_data_ptr->best_accel_time_2 = stats_elapsed_acc_time_1;
				}
			}
			if (car_data->track_slice_lap - stats_init_track_slice > 83) {
				stats_elapsed_acc_time_2 = iSimTimeClock - stats_init_time;
				if (stats_timer_c > iSimTimeClock - stats_init_time && stats_elapsed_acc_time_2 < 1000) {
					if (stats_elapsed_acc_time_2 < stats_data_ptr->quarter_mile_time) {
						stats_data_ptr->quarter_mile_time = stats_elapsed_acc_time_2;
						stats_data_ptr->quarter_mile_speed = car_data->speed_local_lon;
					}
					tnfs_record_best_acceleration(0, 0, stats_elapsed_acc_time_2, car_data->speed_local_lon);
					stats_timer_c = stats_elapsed_acc_time_2;
				}
			}
			if (car_data->brake > 50) {
				if (car_data->speed_local_lon > 2340290)
					stats_braking_init_time = iSimTimeClock;
				if (car_data->speed_local_lon > 1755447)
					stats_braking_final_time = iSimTimeClock;
				if (car_data->speed_local_lon < 6553 && stats_braking_final_time > 0) {
					stats_elapsed_brake_time_1 = iSimTimeClock - stats_braking_final_time;

					printf("TUNING STATS : 60-0 in seconds %d = feet %d", 100 * (iSimTimeClock - stats_braking_final_time) / 60, stats_elapsed_brake_time_1);
					tnfs_record_best_braking(stats_elapsed_brake_time_1, 0);
					stats_braking_final_time = 0;
					if (stats_data_ptr->best_brake_time_1 > stats_elapsed_brake_time_1)
						stats_data_ptr->best_brake_time_1 = stats_elapsed_brake_time_1;
				}
				if (car_data->speed_local_lon < 6553 && stats_braking_init_time > 0) {
					stats_elapsed_brake_time_2 = iSimTimeClock - stats_braking_init_time;

					printf("TUNING STATS : 80-0 in %d seconds = %d feet", 100 * (iSimTimeClock - stats_braking_init_time) / 60, stats_elapsed_brake_time_2);
					tnfs_record_best_braking(0, stats_elapsed_brake_time_2);
					stats_braking_init_time = 0;
					if (stats_data_ptr->best_brake_time_2 > stats_elapsed_brake_time_2)
						stats_data_ptr->best_brake_time_2 = stats_elapsed_brake_time_2;
				}
			}
		}
		if (car_data->tire_skid_rear) {
			if (abs(car_data->angular_speed) > 3276800)
				tnfs_replay_highlight_record(87);
		}
	}

}

void tnfs_height_3B76F(tnfs_car_data *car) {
	tnfs_vec3 front;
	tnfs_vec3 side;

	math_rotate_2d(0, car->car_length, -car->angle_dy, &front.x, &front.z);
	math_rotate_2d(-car->car_width, 0, -car->angle_dy, &side.x, &side.z);

	car->front_edge.x = front.x;
	car->front_edge.y = 0;
	car->front_edge.z = front.z;
	car->side_edge.x = side.x;
	car->side_edge.y = 0;
	car->side_edge.z = side.z;
}

void tnfs_height_3B6AB(tnfs_car_data *car) {
	tnfs_vec3 front;
	tnfs_vec3 side;

	math_rotate_2d(0, car->car_length, -car->angle.y, &front.x, &front.z);
	math_rotate_2d(-car->car_width, 0, -car->angle.y, &side.x, &side.z);

	car->front_edge.x = front.x;
	car->front_edge.y = 0;
	car->front_edge.z = front.z;
	car->side_edge.x = side.x;
	car->side_edge.y = 0;
	car->side_edge.z = side.z;
}

void tnfs_height_road_position(tnfs_car_data *car_data, int mode) {
	tnfs_vec3 *pR;
	tnfs_vec3 pC;
	tnfs_vec3 pB;
	tnfs_vec3 pA;
	int node;

	if (mode) {
		tnfs_height_3B76F(car_data);
		car_data->world_position.x = car_data->position.x;
		car_data->world_position.y = car_data->position.y;
		car_data->world_position.z = car_data->position.z;
		pR = &car_data->world_position;
	} else {
		tnfs_height_3B6AB(car_data);
		car_data->road_ground_position.x = car_data->position.x;
		car_data->road_ground_position.y = car_data->position.y;
		car_data->road_ground_position.z = car_data->position.z;
		pR = &car_data->road_ground_position;
	}

	// get 3 surface points to triangulate surface position
	node = car_data->track_slice & g_slice_mask;
	pA.x = track_data[node].pos.x;
	pA.y = track_data[node].pos.y;
	pA.z = track_data[node].pos.z;

	pB.x = (track_data[node].side_normal_x) * 2 + pA.x;
	pB.y = (track_data[node].side_normal_y) * 2 + pA.y;
	pB.z = (track_data[node].side_normal_z) * 2 + pA.z;

	node = (car_data->track_slice + 1) & g_slice_mask;
	pC.x = track_data[node].pos.x;
	pC.y = track_data[node].pos.y;
	pC.z = track_data[node].pos.z;

	math_height_coordinates(&car_data->front_edge, &car_data->side_edge, pR, &pA, &pB, &pC);
}

void tnfs_height_car_inclination(tnfs_car_data *car, int rX, int rZ) {
	int angle_x;
	int angle_z;
	int dz;
	int dx;
	int aux;

	angle_x = math_atan2(car->car_length, rX - car->front_edge.y);
	angle_z = math_atan2(car->car_width,  -rZ - car->side_edge.y);

	// track slice inclination transition
	dx = abs(fix2(angle_x - car->angle.x));
	dz = abs(fix2(angle_z - car->angle.z));

	aux = car->angle.x - angle_x;
	if (aux <= dx) {
		if (aux < -dx)
			car->angle.x += dx;
	} else {
		car->angle.x -= dx;
	}

	aux = car->angle.z - angle_z;
	if (aux <= dz) {
		if (aux < -dz)
			car->angle.z += dz;
	} else {
		car->angle.z -= dz;
	}
}

void tnfs_height_get_surface_inclination(tnfs_car_data *car, int *x, int *z) {
	*x = math_atan2(car->car_length, -car->front_edge.y);
	*z = math_atan2(car->car_width, -car->side_edge.y);
}

int DAT_00145380 = 0;

/*
 * Simulate gravity and position the car above the road.
 */
void tnfs_height_position(tnfs_car_data *car, int is_driving_mode) {
	int bounce;
	int angleX;
	int angleZ;
	int nextHeight;
	int aux;
	int aux2 = 0;

	tnfs_height_road_position(car, 0);

	aux2 = car->road_ground_position.y;
	DAT_00145380 = car->track_slice;

	/* apply gravity */
	car->speed_y -= ((car->delta_time >> 2) * 0x9cf5c) >> 14;

	if ((g_game_settings & 0x20) == 0) {
		aux = ((car->delta_time >> 2) * 0x9cf5c);
		car->speed_y -= ((aux >> 14) - (aux >> 0x1f)) >> 1;
	}

	// next frame falling height
	nextHeight = (car->delta_time >> 4) * ((fix2(car->speed_y) + car->speed_y) >> 0xc) + car->position.y;

	if (!is_driving_mode) {
		// in crash mode
		car->position.y = aux2;
		car->speed_y = 0;
		car->slope_force_lon = 0;
		car->slope_force_lat = 0;
		return;
	}

	if (car->road_ground_position.y >= nextHeight - 0x40) {
		// hit the ground

		nextHeight = car->road_ground_position.y;

		tnfs_height_car_inclination(car, 0, 0);
		tnfs_height_get_surface_inclination(car, &angleX, &angleZ);

		// vertical ramp speed
		car->speed_y = -fixmul(car->speed_local_lat, math_sin_3(angleZ)) - fixmul(car->speed_local_lon, math_sin_3(angleX));

		if (car->time_off_ground > 20) {
			if (g_game_settings & 0x20)
				//bounce = fix2(car_data->time_off_ground / 6) * (((car_data->delta_time >> 2) * 0x9cf5c) >> 14); //PSX
				bounce = (((car->delta_time >> 2) * 0x9cf5c) >> 14) * (((car->time_off_ground >> 0x1f) - car->time_off_ground) >> 1);
			else
				bounce = (((car->delta_time >> 2) * 0x9cf5c) >> 14) * fix2(car->time_off_ground);

			car->speed_y += bounce;
			printf("Boink %d %d\n", car->speed_y >> 16, 0);
		}

		if (car->time_off_ground > 10) {
			tnfs_replay_highlight_record(84);
			car->time_off_ground = -2;
			car->angle.x = angleX;
			car->angle.z = angleZ;
			if (car->car_id2 == 0)
				DAT_8010c478 = 5;
		}

		if (car->time_off_ground < 0)
			car->time_off_ground++;
		else
			car->time_off_ground = 0;

		if (car->time_off_ground == -1 && car->car_id2 == 0) {
			tnfs_sfx_play(-1, 4, 1, abs(car->speed_y), 1, 0x50000);
		}

		if (!car->wheels_on_ground && car->gear_selected != -1)
			car->is_gear_engaged = 1;

		car->wheels_on_ground = 1;

	} else {
		// wheels on air
		if (nextHeight - car->road_ground_position.y >= 0x4000) {
			car->is_gear_engaged = 0;
			car->wheels_on_ground = 0;
			car->time_off_ground++;

			if (car->angle.z >= 0)
				tnfs_height_car_inclination(car, 983 * car->time_off_ground, -655 * car->time_off_ground);
			else
				tnfs_height_car_inclination(car, 983 * car->time_off_ground, 655 * car->time_off_ground);
		} else {
			tnfs_height_car_inclination(car, 0, 0);

			if (!car->wheels_on_ground && car->gear_selected != -1)
				car->is_gear_engaged = 1;

			car->wheels_on_ground = 1;
		}
	}

	car->slope_force_lon = (math_sin_3(car->angle.x) >> 8) * 0x9cf;
	car->unknown_flag_3DD = 0;
	car->slope_force_lat = 0;

	if (car->speed_y > 0) {
		car->slope_force_lon = math_mul(0x2666, car->slope_force_lon);
	} else {
		if (car->speed_y < 0)
			car->slope_force_lon = math_mul(0x10000, car->slope_force_lon);
	}

	if (DAT_8010c478)
		DAT_8010c478--;

	car->position.y = nextHeight;
}

/*
 * Main simulation loop
 * EXE address:
 * - 3DO 0xf8d8,
 * - PC DOS DEMO 0x3b4f6
 * - PC DOS 0x580A5
 * - PSX 8002df18
 * - WIN95 TNFSSE 0x4286a0
 */
void tnfs_driving_main(tnfs_car_data *car) {
	tnfs_physics_update(car);
	tnfs_track_node_update(car);
	tnfs_height_position(car, 1);
}


/*
 * do the "180 spin" crossing the finish/checkpoint line
 */
void tnfs_driving_checkpoint_flick(tnfs_car_data *car) {
	int iVar1;
	int iVar3;
	int iVar4;
	int iVar5;
	int iVar6;
	int iVar7;
	int local_34;
	int local_30;

	iVar4 = track_data[car->track_slice & g_slice_mask].heading * 0x400;
	iVar3 = abs(iVar4 - (car->angle).y);

	if (iVar3 > 0x800000) {
		iVar3 = 0x1000000 - iVar3;
	}
	iVar1 = iVar3 - 0x400000;
	if (iVar1 < 1) {
		iVar1 = 0x400000 - iVar3;
	}

	if ((iVar1 < 0x80000) || (car->field_4cd == 2)) {
		car->handbrake = 0;
		car->brake = 0xfa;
		car->throttle = 0;
		iVar7 = car->angular_speed >> 0x1f;
		car->angular_speed = car->angular_speed - (((car->angular_speed + iVar7 * -8) - (iVar7 << 2 < 0)) >> 3);
		car->speed_local_lon = 0;
		if (car->speed_local_lat < 1) {
			if (car->field_4d1 < 0x190000) {
				car->field_4d3 += 3;
			}
		} else {
			if (car->field_4d1 > -0x190000) {
				car->field_4d1 -= 0x30000;
			}
		}
		car->steer_angle = car->field_4d1;
		car->field_4cd = 2;
	} else {
		if (car->field_4cd == 0) {
			car->field_4c9 = 0;
			car->field_4d1 = 0;

			iVar5 = math_sin_2(iVar4);
			iVar6 = math_cos_2(iVar4);
			iVar4 = track_data[car->track_slice & g_slice_mask].pos.x;
			iVar7 = track_data[car->track_slice & g_slice_mask].pos.z;

			car->field_4c5 = fixmul(iVar5, (iVar7 - car->position.z)) - fixmul(iVar6, (iVar4 - car->position.x));
			car->field_4c9 = 0;
		}
		car->throttle = 0;
		car->handbrake = 1;
		car->field_4cd = 1;
		if ((track_data[car->track_slice & g_slice_mask].roadLeftFence * 0x2000 + car->field_4c5) < 1) {
			local_34 = -(track_data[car->track_slice & g_slice_mask].roadLeftFence * 0x2000 + car->field_4c5);
		} else {
			local_34 = car->field_4c5 + track_data[car->track_slice & g_slice_mask].roadLeftFence * 0x2000;
		}
		if (car->field_4c5 == track_data[car->track_slice & g_slice_mask].roadRightFence * 0x2000
				|| (car->field_4c5 + track_data[car->track_slice & g_slice_mask].roadRightFence * -0x2000) < 0) {
			local_30 = -(car->field_4c5 + track_data[car->track_slice & g_slice_mask].roadRightFence * -0x2000);
		} else {
			local_30 = car->field_4c5 + track_data[car->track_slice & g_slice_mask].roadRightFence * -0x2000;
		}
		//flick the steering wheel
		if (local_34 < local_30) {
			if (car->field_4c9 < 6) {
				car->steer_angle = -0x1e0000;
			} else {
				car->steer_angle = 0x1e0000;
			}
		} else if (car->field_4c9 < 6) {
			car->steer_angle = 0x1e0000;
		} else {
			car->steer_angle = -0x1e0000;
		}
	}

	car->field_4c9++;
	//iVar7 = car->speed_local_lat >> 0x1f;
	//car->speed_local_lat = car->speed_local_lat - (((car->speed_local_lat + iVar7 * -8) - (iVar7 << 2 < 0)) >> 3);
	//iVar7 = car->speed_local_lon >> 0x1f;
	//car->speed_local_lon -= -(((car->speed_local_lon + iVar7 * -8) - (iVar7 << 2 < 0)) >> 3);
}


