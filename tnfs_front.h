#ifndef TNFS_FRONT_H_
#define TNFS_FRONT_H_

int tnfs_ui_pause(int id);
int tnfs_ui_control(int param_1);
int tnfs_ui_route(int param_1, int param_2, int param_3, int param_4);
int tnfs_ui_options(int param_1, struct tnfs_config *param_2);
int tnfs_ui_showcase(int param_1,int param_2);
int tnfs_ui_wall(int param_1);
void tnfs_ui_loading_screen();
int tnfs_ui_credits(int param_1, int param4);
int tnfs_ui_checkpoint(int param_1);
int tnfs_ui_checkopts(int param_1,  struct tnfs_config *param_2);
int tnfs_ui_finish(int param_1);
void tnfs_ui_cop_ticket(int param_1);
void tnfs_ui_initial(int param_1);

#endif 
