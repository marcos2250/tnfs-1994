/*
 * tnfs_ai.h
 */

#ifndef TNFS_AI_H_
#define TNFS_AI_H_


void tnfs_ai_init(int oppcarid);
void tnfs_ai_load_car(tnfs_car_data *car, int newCarModelId);
void tnfs_ai_driving_main(tnfs_car_data * car);
void tnfs_ai_collision_handler();
void tnfs_ai_respawn_main(tnfs_car_data *car);
void tnfs_ai_police_reset_state(int flag);
void tnfs_ai_hidden_traffic(tnfs_car_data *car);
void tnfs_ai_respawn_00028cc4();
void tnfs_ai_respawn_0007d647();

#endif /* TNFS_AI_H_ */
