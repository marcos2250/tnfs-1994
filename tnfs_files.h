/*
 * tnfs_files.h
 */

#ifndef TNFS_FILES_H_
#define TNFS_FILES_H_

#define byte unsigned char

typedef struct file_assets {
    char name[128];
    int length;
    byte * content;
} file_assets;

void fileWrite(byte * data, int size);
byte * openFile(char * filename, int * fileSize);
byte * openFileBuffer(char * filename, int * fileSize);
int readbits(unsigned char * data, int * idx_, byte size);
int readFixed32(unsigned char *buffer, int pos);
int readSigned16(unsigned char *buffer, int pos);

byte * read_wwww(byte *data, int path[], int depth);
int locate_wwww(byte *data, byte *pointer, int depth, char *path_result);
int read_tri_file(char * file);
int read_carspecs_file(char * file);
int read_tddyn_file(char * file, tnfs_car_data *car);
int read_skill_file(int skill);
int read_carmodel_file(char * carname, tnfs_carmodel3d * carmodel);
int read_track_pkt_file(char * trackname);
int read_hud_dash_file();

#endif /* TNFS_FILES_H_ */
