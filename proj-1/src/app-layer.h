#ifndef APP_LAYER_H___
#define APP_LAYER_H___

#include "strings.h"

#include <stdbool.h>

#define PCONTROL_DATA          0x01
#define PCONTROL_START         0x02
#define PCONTROL_END           0x03
#define PCONTROL_BAD_PACKET    0x40

#define PCONTROL_TYPE_FILESIZE 0x00
#define PCONTROL_TYPE_FILENAME 0x01

#define FILESIZE_TLV_N         0
#define FILENAME_TLV_N         1

#define PRECEIVE_DATA          0x51
#define PRECEIVE_START         0x52
#define PRECEIVE_END           0x53
#define PRECEIVE_BAD_PACKET    0x54

typedef struct {
    char type;
    string value;
} tlv;

typedef struct {
    char c;
    tlv* tlvs;
    size_t n;
} control_packet;

typedef struct {
    int index;
    string data;
} data_packet;


void free_control_packet(control_packet packet);

void free_data_packet(data_packet packet);


bool get_tlv(control_packet controlp, char type, string* outp);

bool get_tlv_filename(control_packet controlp, char** outp);

bool get_tlv_filesize(control_packet controlp, size_t* outp);


int send_data_packet(int fd, string packet);

int send_start_packet(int fd, size_t filesize, char* filename);

int send_end_packet(int fd, size_t filesize, char* filename);

int receive_packet(int fd, data_packet* datap, control_packet* controlp);

#endif // APP_LAYER_H___
