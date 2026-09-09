#include <stdio.h>
#include <string.h>
#include "tnfs_base.h"
#include "tnfs_front.h"
#include "tnfs_gfx.h"

char * g_tracks_4[4] = { "alpn", "cstl", "city", "test" };
char * g_tracks_t[4] = { "alpt", "cstt", "ctyt", "tstt" };
char * g_tracks_s[9] = { "alp2", "alp2", "alp3", "cst2", "cst2", "cst3", "cty3", "cty2", "cty2" };
char * g_track_names[4] = { "Alpine", "Coastal", "City", "Test" };
char * g_cars[8] = { "spra", "dblo", "p911", "cvet", "f512", "vipr", "ansx", "mrx7" };
char * g_cars_b[9] = { "sprb", "dblb", "911b", "vetb", "512b", "vipb", "nsxb", "rx7b", 0 };
char * g_cars_f[9] = { "sprf", "dblf", "911f", "vetf", "512f", "vipf", "nsxf", "rx7f", 0 };
char * g_cars_names[8] = { "supra", "diablo", "p911", "vette", "512tr", "viper", "nsx", "rx7" };
char * g_skill[6] = { "novc", "intr", "pro ", "pnov", "pint", "ppro" };
char * g_off_on[2] = { "off ", "on  " };
char * g_controls[6] = { "1   ", "2   ", "3   ", "4   ", "5   ", "joy " };
char * g_audio[4] = { "none", "musc", "sfx ", "all " };
char * g_audio_mode[2] = { "mono", "ster"};
char * g_wall[11] = { "64.%d","56.%d","65.%d","57.%d","69.%d","70.1","60.%d","59.%d","68.%d","67.%d","20.2" };
char * g_skill_abr[3] = { "Nov", "Int", "Pro" };
char * g_car_slides[8] = { "TSUPRA", "LDIABLO", "P911", "CZR1", "F512TR", "DVIPER", "ANSX", "MRX7" };

char * g_showcase[8] = {
	"frontend/display/stats/supra.3sh",
	"frontend/display/stats/diablo.3sh",
	"frontend/display/stats/911.3sh",
	"frontend/display/stats/vette.3sh",
	"frontend/display/stats/512.3sh",
	"frontend/display/stats/viper.3sh",
	"frontend/display/stats/nsx.3sh",
	"frontend/display/stats/rx7.3sh",
};

char * g_loading[24] = {
  "frontend/display/loading/supra0.3sh",
  "frontend/display/loading/supra1.3sh",
  "frontend/display/loading/supra2.3sh",
  "frontend/display/loading/diablo0.3sh",
  "frontend/display/loading/diablo1.3sh",
  "frontend/display/loading/diablo2.3sh",
  "frontend/display/loading/9110.3sh",
  "frontend/display/loading/9111.3sh",
  "frontend/display/loading/9112.3sh",
  "frontend/display/loading/vette0.3sh",
  "frontend/display/loading/vette1.3sh",
  "frontend/display/loading/vette2.3sh",
  "frontend/display/loading/512tr0.3sh",
  "frontend/display/loading/512tr1.3sh",
  "frontend/display/loading/512tr2.3sh",
  "frontend/display/loading/viper0.3sh",
  "frontend/display/loading/viper1.3sh",
  "frontend/display/loading/viper2.3sh",
  "frontend/display/loading/nsx0.3sh",
  "frontend/display/loading/nsx1.3sh",
  "frontend/display/loading/nsx2.3sh",
  "frontend/display/loading/rx70.3sh",
  "frontend/display/loading/rx71.3sh",
  "frontend/display/loading/rx72.3sh"
};

int DAT_0000a960[8] = { 1, 0, 1, 1, 1, 0, 1, 1 };
int DAT_0000a9d0[8] = { 1, 0, 0, 1, 0, 0, 1, 0 };
short DAT_0001ca48[8] = { 0x10, 0x2a, 0x46, 0x60, 0x7c, 0x97, 0xb1, 0xcd };

byte g_loading_pic = 0;
byte g_loading_pic2 = 0;

int DAT_00007914 = 0;
int DAT_000016dc;
int DAT_0000bd88;
int PTR_DAT_0000174c;

void FUN_0000953c() {
}
void FUN_000093a4(int a) {
}
int FUN_00000148() {
	return 0;
}
void FUN_00009048() {	
}
void FUN_000033b8() {
}
void FUN_00009554() {
}
void FUN_00009548() {
}
void tnfs_ui_trixie() {
}
void FUN_00009560() {
}
void FUN_00005db4() {
}
void FUN_0000911c() {
}
void FUN_00019714() {
}
void FUN_00004150() {
}
int FUN_00000b2c(int a, int b, int c) {
  return 0;
}

void tnfs_ui_loading_screen(int id) {
	char filename[80];
	CCB * cel;
	byte * load;
	shpm_image * img;

	if (id == 0) {
		g_loading_pic = (g_loading_pic + 1) % 3;
		gfx_clear();
		load = gfx_openfile_9594(g_loading[(g_player_car * 3) + g_loading_pic], 0);
		img = gfx_readshape_9444(load, "load");
		gfx_drawshape_950c(img, img->left_le, img->top_le);
		DisplayScreen(0, 0);

	} else if (id == 1) {
		g_loading_pic2 = (g_loading_pic2 + 1) % 6;
		sprintf(filename, "DriveData/CarData/%s.carSlideArt%d", g_car_slides[g_player_car], g_loading_pic2);
		cel = LoadCel(filename, 0);
		DrawScreenCels(0, cel);
		DisplayScreen(0, 0);
	} else {
		g_loading_pic2 = (g_loading_pic2 + 1) % 6;
		sprintf(filename, "DriveData/CarData/%s.s%d", g_car_slides[g_player_car], g_loading_pic2);
		cel = LoadCel(filename, 0);
		DrawScreenCels(0, cel);
		DisplayScreen(0, 0);
	}
}

int tnfs_ui_userinput(int isTime, int isScore, int isSpeed, int cx, int cy, int icase, char * username) {
	byte *data;
	shpm_image * img;
	char * label;

	//uVar8 = CONCAT44(username, in_r0);
	//if ((int) &stack0xffffffc8 < unaff_r10) {
	//	uVar8 = FUN_00000148();
	//}
	//tnfs_ui_trixie_font();
	//a = FUN_00009530();
	//FUN_0000953c(a, b);
	//FUN_000093a4(0);

	data = gfx_openfile_9594("frontend/display/userinp.3sh", 0);

	img = gfx_readshape_9444(data, "bgnd");
	gfx_drawshape_950c(img, img->left_le, img->top_le);

	if (isTime >= 0) {
		img = gfx_readshape_9444(data, "time");
		gfx_drawshape_950c(img, img->left_le, img->top_le);
	}

	if (isScore >= 0) {
		img = gfx_readshape_9444(data, "scor");
		gfx_drawshape_950c(img, img->left_le, img->top_le);
	}

	if (isSpeed >= 0) {
		img = gfx_readshape_9444(data, "spd ");
		gfx_drawshape_950c(img, img->left_le, img->top_le);
	}

	img = gfx_readshape_9444(data, "nbox");
	gfx_drawshape_950c(img, img->left_le, img->top_le);

	if (icase) {
		label = "ucas";
	} else {
		label = "lcas";
	}
	img = gfx_readshape_9444(data, label);
	gfx_drawshape_950c(img, img->left_le, img->top_le);

	img = gfx_readshape_9444(data, "crsr");
	gfx_draw_shpm(img, cx * 0x17 + 0x51, cy * 0x17 + 0x24);

	gfx_draw_text_9500(username,0x45,0xcd);

	//FUN_000033b8();
	//FUN_00009554();
	//FUN_00009548();
	return 1;
}

int tnfs_ui_wall(int param_1) {
	byte *data;
	int iVar5;
	char acStack_78[80];
	struct tnfs_hiscores *stats;
	int i;
	shpm_image * img;

	//if ((int)&stack0xffffffd8 < unaff_r10) {
	//  param_1 = FUN_00000148();
	//}
	tnfs_ui_trixie();
	data = gfx_openfile_9594("frontend/display/wall.3sh", 0);
	FUN_00009048();
	FUN_0000953c();
	FUN_00009560();
	FUN_000093a4(0);
	FUN_00009554();

	img = gfx_readshape_9444(data, "bgnd");
	gfx_drawshape_950c(img, img->left_le, img->top_le);

	for (i = 0; i < 12; i++) {
		stats = &g_hiscores[i];
		if (stats->score == 0)
			break;

		sprintf(acStack_78, "%-10s %5d", stats->name, stats->score);
		iVar5 = i * 0xc + 0x2c;
		gfx_draw_text_9500(acStack_78, 0x1d, iVar5);
		sprintf(acStack_78, "%-6s %-7s %-6s", g_cars_names[stats->car_id], g_track_names[stats->track_id], g_skill_abr[stats->skill]);
		gfx_draw_text_9500(acStack_78, 0x9b, iVar5);
		FUN_00009554();
	}

	FUN_000033b8();
	FUN_00009554();
	FUN_00009548();
	FUN_0000911c(data);

	/*
	if (FUN_00000b2c(900, 0, 0) == 0) {
		tnfs_ui_credits(param_1, param_1);
	} else {
		FUN_00005db4();
		FUN_00019714(0x78);
	}
	*/

	return 1;
}

int tnfs_ui_showcase(int param_1, int param_2) {
	byte *pbVar2 = 0;
	int iVar3;
	byte *puVar4 = 0;
	int iVar5;
	int iVar6;
	shpm_image * img;

	//uVar7 = CONCAT44(param_2,param_1);
	//if ((int)&stack0xffffffe0 < unaff_r10) {
	//  uVar7 = FUN_00000148();
	//}
	//iVar6 = (int)((ulonglong)uVar7 >> 0x20);
	iVar6 = param_1;

	FUN_0000953c();
	FUN_000093a4(0);
	//iVar1 = g_cars_names;
	if (pbVar2 == 0) {
		pbVar2 = gfx_openfile_9594("frontend/display/stats.3sh", 0);
		FUN_00009048();
	}
	if (puVar4 == 0) {
		puVar4 = gfx_openfile_9594(g_showcase[g_player_car], 0);
		FUN_00009048();
	}
	img = gfx_readshape_9444(pbVar2, "bgnd");
	gfx_drawshape_950c(img, img->left_le, img->top_le);
	iVar3 = 0; //*(int *)(pbVar2 + 4);

	img = gfx_readshape_9444(puVar4, "text");
	gfx_drawshape_94f4_at(img, 40, 120 - iVar6);
	iVar5 = 0; //puVar4[1];
	img = gfx_readshape_9444(pbVar2, "topc");
	gfx_drawshape_950c(img, img->left_le, img->top_le);
	img = gfx_readshape_9444(pbVar2, "botc");
	gfx_drawshape_950c(img, img->left_le, img->top_le);
	if (iVar6 != 0) {
		img = gfx_readshape_9444(pbVar2, "upar");
		gfx_drawshape_950c(img, img->left_le, img->top_le);
	}
	iVar3 = (((iVar5 << 0x14) >> 0x14) - ((iVar3 << 0x14) >> 0x14)) + 2;
	if (iVar6 < iVar3) {
		img = gfx_readshape_9444(pbVar2, "dnar");
		gfx_drawshape_950c(img, img->left_le, img->top_le);
	}
	img = gfx_readshape_9444(puVar4, "car ");
	gfx_drawshape_94f4(img, img->left_le, img->top_le);
	img = gfx_readshape_9444(puVar4, "titl");
	gfx_drawshape_94f4(img, img->left_le, img->top_le);
	FUN_000033b8();
	FUN_00009554();
	FUN_00009548();
	return iVar3;
}

short g_pausegame_coords[] = { 67, 92, 117, 142 };

int tnfs_ui_pause(int id) {
	byte *pbVar2;
	shpm_image * img;

	memset(g_backbuffer, 0, 307200);
	pbVar2 = gfx_openfile_9594("frontend/display/Pausegame.3sh", 0);

	img = gfx_readshape_9444(pbVar2, "menu");
	gfx_drawshape_950c(img, img->left_le, img->top_le);

	img = gfx_readshape_9444(pbVar2, "lite");
	gfx_drawshape_94f4_at(img, 61, g_pausegame_coords[id]);
	return 1;
}

short DAT_0000750c[] = { 0x39, 0x55, 0x72, 0x8D, 0xA9 };

int tnfs_ui_finish(int param_1) {
  byte *pbVar2;
  shpm_image * img;

  //if ((int)&stack0xffffffe8 < unaff_r10) {
  //  param_1 = FUN_00000148();
  //}
  //uVar3 = FUN_0000953c();
  //FUN_00009560(uVar3);
  //FUN_000093a4(0);

  //if (*(int *)(DAT_000073f4 + 0x140) == 0) {
  pbVar2 = gfx_openfile_9594("frontend/display/finish.3sh",0);
  // *(byte **)(iVar1 + 0x140) = pbVar2;
  //  FUN_00009048();
  //}

  img = gfx_readshape_9444(pbVar2, "menu");
  gfx_drawshape_950c(img, img->left_le, img->top_le);

  img = gfx_readshape_9444(pbVar2, "lite");
  gfx_drawshape_94f4_at(img, 0x52, DAT_0000750c[param_1]);
  FUN_000033b8();
  FUN_00009554();
  FUN_00009548();
  return 1;
}


short DAT_00002378[] = { 0x27, 0x43, 0x60, 0x7B, 0x97, 0xB3 };

int tnfs_ui_checkpoint(int param_1) {
	byte *pbVar2;
	shpm_image * img;

	//if ((int)&stack0xffffffe8 < unaff_r10) {
	//  param_1 = FUN_00000148();
	//}
	FUN_0000953c();
	FUN_00009560();
	FUN_000093a4(0);

	//iVar1 = DAT_0000225c;
	//if (*(int *)(DAT_0000225c + 100) == 0) {
	pbVar2 = gfx_openfile_9594("frontend/display/checkpoint.3sh", 0);
	//  *(byte **)(iVar1 + 100) = pbVar2;
	//  FUN_00009048();
	//}
	//pbVar2 = *(byte**) (iVar1 + 100);

	img = gfx_readshape_9444(pbVar2, "menu");
	gfx_drawshape_950c(img, img->left_le, img->top_le);

	img = gfx_readshape_9444(pbVar2, "lite");
	gfx_drawshape_94f4_at(img, 0x52, DAT_00002378[param_1]);

	FUN_000033b8();
	FUN_00009554();
	FUN_00009548();
	return 1;
}

short DAT_00003078[] = { 0x3C, 0x66, 0x91 };

int tnfs_ui_checkopts(int param_1,  struct tnfs_config *param_2) {
  byte *pbVar3;
  char *pcVar6;
  shpm_image * img;

  //if ((int)&stack0xffffffe4 < unaff_r10) {
  //  param_1 = FUN_00000148();
  //}
  //uVar5 = FUN_0000953c();
  //FUN_00009560(uVar5);
  FUN_000093a4(0);
  //iVar1 = DAT_0000225c;

  //if (*(int *)(DAT_0000225c + 0xa8) == 0) {
  pbVar3 = gfx_openfile_9594("frontend/display/checkopts.3sh",0);
  //  *(byte **)(iVar1 + 0xa8) = pbVar3;
  //  FUN_00009048();
  //}

  //pbVar3 = *(byte **)(iVar1 + 0xa8);
  img = gfx_readshape_9444(pbVar3, "bgnd");
  gfx_drawshape_950c(img, img->left_le, img->top_le);

  img = gfx_readshape_9444(pbVar3, g_audio[param_2->audio]);
  gfx_drawshape_94f4(img, img->left_le, img->top_le);

  if (param_2->audio_mode == 0) {
    pcVar6 = "mono";
  } else {
    pcVar6 = "ster";
  }
  img = gfx_readshape_9444(pbVar3,pcVar6);
  gfx_drawshape_94f4(img, img->left_le, img->top_le);

  if (param_2->opp_video == 0) {
    pcVar6 = "off ";
  } else {
    pcVar6 = "on  ";
  }
  img = gfx_readshape_9444(pbVar3, pcVar6);
  gfx_drawshape_94f4(img, img->left_le, img->top_le);

  img = gfx_readshape_9444(pbVar3, "lite");
  gfx_drawshape_94f4_at(img, 0x27, DAT_00003078[param_1]);

  FUN_000033b8();
  FUN_00009554();
  FUN_00009548();
  return 1;
}


int tnfs_ui_options(int param_1, struct tnfs_config *param_2) {
	byte *iVar1_408;
	//astruct_3 *extraout_r1;
	unsigned int uVar4;
	shpm_image * img;

	//if ((int)&stack0xffffffd8 < unaff_r10) {
	FUN_00000148();
	//  param_2 = extraout_r1;
	//}

	uVar4 = param_2->skill_level;
	//uVar4 = param_2->skill_level & 3;
	//if ((param_2->skill_level & 0x80) != 0) {
	//	uVar4 = uVar4 + 3;
	//}
	tnfs_ui_trixie();
	FUN_0000953c();
	FUN_00009560();
	FUN_000093a4(0);

	//iVar1 = _DAT_0000a4a0;
	//if (*(int *)(_DAT_0000a4a0 + 0x408) == 0) {
	iVar1_408 = gfx_openfile_9594("frontend/display/options.3sh", 0);
	FUN_00009048();
	//}

	img = gfx_readshape_9444(iVar1_408, "bgnd");
	gfx_drawshape_950c(img, img->left_le, img->top_le);
	img = gfx_readshape_9444(iVar1_408, g_skill[uVar4]);
	gfx_drawshape_94f4(img, img->left_le, img->top_le);
	img = gfx_readshape_9444(iVar1_408, g_audio[param_2->audio]);
	gfx_drawshape_94f4(img, img->left_le, img->top_le);
	img = gfx_readshape_9444(iVar1_408, g_audio_mode[param_2->audio_mode]);
	gfx_drawshape_94f4(img, img->left_le, img->top_le);

	img = gfx_readshape_9444(iVar1_408, g_off_on[param_2->opp_video]);
	gfx_drawshape_94f4(img, img->left_le, img->top_le); //, 0x28, 0x60);

	if (DAT_0000a960[g_player_car] == 0) {
		img = gfx_readshape_9444(iVar1_408, "na  ");
		gfx_drawshape_94f4_at(img, 0x28 + 0xd5, 0x7C + 7);
		param_2->abs = 0;
	} else {
		img = gfx_readshape_9444(iVar1_408, g_off_on[param_2->abs]);
		gfx_drawshape_94f4_at(img, 0x28 + 0xd5, 0x7C + 7);
	}

	if (DAT_0000a9d0[g_player_car] == 0) {
		img = gfx_readshape_9444(iVar1_408, "na  ");
		gfx_drawshape_94f4_at(img, 0x28 + 0xd5, 0x97 + 7);
		param_2->tcs = 0;
	} else {
		img = gfx_readshape_9444(iVar1_408, g_off_on[param_2->tcs]);
		gfx_drawshape_94f4_at(img, 0x28 + 0xd5, 0x97 + 7);
	}

	img = gfx_readshape_9444(iVar1_408, g_controls[param_2->control]);
	gfx_drawshape_94f4(img, img->left_le, img->top_le);
	img = gfx_readshape_9444(iVar1_408, "lite");
	gfx_drawshape_94f4_at(img, 0x28, DAT_0001ca48[param_1]);
	FUN_000033b8();
	FUN_00009554();
	FUN_00009548();
	return 1;
}

void FUN_0000b344(int itime, char *param_2) {
	int iVar1;
	int iVar2;
	int iVar3;

	iVar1 = itime / 3600;
	iVar2 = (itime / 60) % 60;
	iVar3 = (itime / 6) % 10;

	//itime *= 120;
	//iVar1 = math_remainder(itime, 0x34bc0);
	//iVar2 = math_remainder(iVar1, 0xe10);
	//iVar3 = math_remainder(iVar2, 0x3c);

	//FUN_00018f84((int)uVar4 + iVar1 * -0x34bc0 + iVar2 * -0xe10 + iVar3 * -0x3c, 0x1AAA6);
	//FUN_0000030c();
	sprintf(param_2, "%2d:%02d.%d", iVar1, iVar2, iVar3);
} 

void FUN_0000b45c(int in, char *out) {
	float mph = ((float)in/ 0x10000) * 2.23693629f;
	sprintf(out, "%4.1f", mph);
}

int tnfs_ui_route(int route, int param_2, int param_3, int segment) {
	struct tnfs_hiscores *stats;
	char auStack_84[12];
	char acStack_78[80];
	byte *iVar4_10 = 0; 
	int i;
	shpm_image * img;

	//if ((int)&stack0xffffffd8 < unaff_r10) {
	//  param_1 = FUN_00000148();
	//}
	tnfs_ui_trixie();
	FUN_0000953c();
	FUN_000093a4(0);
	//iVar4 = DAT_0000bd88;

	if (iVar4_10 == 0) { //*(int *)(DAT_0000bd88 + 0x10) == 0) {
		iVar4_10 = gfx_openfile_9594("frontend/display/route.3sh",0);
		FUN_00009048();
	}

	img = gfx_readshape_9444(iVar4_10, "bgnd");
	gfx_drawshape_95a0(img, img->left_le, img->top_le);
	img = gfx_readshape_9444(iVar4_10, "text");
	gfx_drawshape_94f4(img, img->left_le, img->top_le);
	img = gfx_readshape_9444(iVar4_10, g_tracks_4[route]);
	gfx_drawshape_94f4(img , img->left_le, img->top_le);
	img = gfx_readshape_9444(iVar4_10, g_tracks_t[route]);
	gfx_drawshape_94f4(img, img->left_le, img->top_le);
	img = gfx_readshape_9444(iVar4_10, g_tracks_s[route]);
	gfx_drawshape_94f4(img, img->left_le, img->top_le);

	FUN_0000b45c(g_best_times[(route * 3)].max_speed, acStack_78);
	gfx_draw_text_9500(acStack_78,0x87,0x8d);
	
	for (i = 0; i < 3; i++) {
		stats = &g_best_times[(route * 3) + i];

		FUN_0000b344(stats->time, auStack_84);

		if (stats->time == 0) {
			sprintf(acStack_78, "%d.", i + 1);
		} else {
			sprintf(acStack_78, "%d. %10s %6s %6s %s", i + 1, //
			stats->name, auStack_84, g_cars_names[stats->car_id], g_skill_abr[stats->skill]);
		}
		gfx_draw_text_9500(acStack_78, 0x25, i * 0xc + 0xb4);
	}

	FUN_000033b8();
	FUN_00009554();
	FUN_00009548();
	return 1;
}

/*
* control central options: 
* 0 - race
* 1 - car select
* 2 - xman shut
* 3 - track select
* 4 - options
*/
int tnfs_ui_control(int param_1) {
	char *pcVar4;
	//int stack0xffffffe4;
	byte *iVar_d4 = 0;
	byte *iVar_d8 = 0;
	shpm_image * img;

	//if ((int) &stack0xffffffe4 < unaff_r10) {
		//param_1 = FUN_00000148();
	//}
	FUN_0000953c();
	FUN_000093a4(0);
	//iVar1 = DAT_000016dc;
	if (iVar_d4 == 0) { //*(int *)(DAT_000016dc + 0xd4) == 0) {
		iVar_d4 = gfx_openfile_9594("frontend/display/control.3sh", 0);
		FUN_00009048();
	}
	if (iVar_d8 == 0) {
		iVar_d8 = gfx_openfile_9594("frontend/display/ctrlcars.3sh", 0);
		FUN_00009048();
	}
	img = gfx_readshape_9444(iVar_d4, "bgnd");
	gfx_drawshape_95a0(img, img->left_le, img->top_le);
	img = gfx_readshape_9444(iVar_d4, "keys");
	gfx_drawshape_94f4(img, img->left_le, img->top_le);
	if (param_1 == 0) {
		img = gfx_readshape_9444(iVar_d4, "keyb");
		gfx_drawshape_94f4(img, img->left_le, img->top_le);
	}
	img = gfx_readshape_9444(iVar_d4, "optn");
	gfx_drawshape_94f4(img, img->left_le, img->top_le);
	if (param_1 == 4) {
		img = gfx_readshape_9444(iVar_d4, "optb");
		gfx_drawshape_94f4(img, img->left_le, img->top_le);
	}

	img = gfx_readshape_9444(iVar_d4, g_tracks_4[g_track_sel]);
	gfx_drawshape_94f4(img, img->left_le, img->top_le);
	if (param_1 == 2) {
		img = gfx_readshape_9444(iVar_d4, "trkb");
		gfx_drawshape_94f4(img, img->left_le, img->top_le);
	}
	if (g_opp_car == 8) {
		pcVar4 = "clok";
	} else if (g_config.opp_video == 0) {
		pcVar4 = "xsht";
	} else {
		pcVar4 = "xman";
	}
	img = gfx_readshape_9444(iVar_d4, pcVar4);
	gfx_drawshape_94f4(img, img->left_le, img->top_le);
	if (param_1 == 3) {
		img = gfx_readshape_9444(iVar_d4, "oppb");
		gfx_drawshape_94f4(img, img->left_le, img->top_le);
	}
	if (g_opp_car == 8) {
		img = gfx_readshape_9444(iVar_d8, "rod1");
		gfx_drawshape_94f4(img, img->left_le, img->top_le);
		img = gfx_readshape_9444(iVar_d8, "frm1");
		gfx_drawshape_94f4(img, img->left_le, img->top_le);
	} else {
		img = gfx_readshape_9444(iVar_d8, "rod2");
		gfx_drawshape_94f4(img, img->left_le, img->top_le);
		img = gfx_readshape_9444(iVar_d8, "frm2");
		gfx_drawshape_94f4(img, img->left_le, img->top_le);
	}

	pcVar4 = g_cars_b[g_opp_car];
	if (pcVar4 != 0) {
		img = gfx_readshape_9444(iVar_d8, pcVar4);
		gfx_drawshape_94f4(img, img->left_le, img->top_le);
	} 
	img = gfx_readshape_9444(iVar_d8, g_cars_f[g_player_car]);
	gfx_drawshape_94f4(img, img->left_le, img->top_le);

	if (param_1 == 1) {
		if (g_opp_car == 8) {
			pcVar4 = "brd1";
		} else {
			pcVar4 = "brd2";
		}
		img = gfx_readshape_9444(iVar_d8, pcVar4);
		gfx_drawshape_94f4(img, img->left_le, img->top_le);
	}
	FUN_000033b8();
	FUN_00009554();
	FUN_00009548();
	return 1;
}


void tnfs_ui_initial(int option) {
	shpm_image * img;
	byte * iVar1_408 = gfx_openfile_9594("frontend/display/checkopts.3sh",0);

	gfx_write_alpha_channel(g_backbuffer, 307200, 0xFF);
	img = gfx_readshape_9444(iVar1_408, "lite");
	gfx_draw_shpm(img, 0x28, option * 40 + 70);

	gfx_draw_text_9500("Start NFS", 0x40, 80);
	gfx_draw_text_9500("Image file viewer", 0x40, 120);
	gfx_draw_text_9500("Audio file player", 0x40, 160);
}
