#include "app-core.h"
#include "ll-interface.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PCONTROL_DATA          0x01
#define PCONTROL_START         0x02
#define PCONTROL_END           0x03

#define PCONTROL_TYPE_FILESIZE 0x00
#define PCONTROL_TYPE_FILENAME 0x01

#define FILESIZE_TLV_N         0
#define FILENAME_TLV_N         1
/*
static int make_data_packet(const char* packet, char index, char** outp) {
    size_t packet_len = strlen(packet);

    if (packet_len > 0xffff) return 1;

    char* data = malloc((packet_len + 5) * sizeof(char));

    data[0] = PCONTROL_DATA;
    data[1] = index;
    data[2] = packet_len / 256lu;
    data[3] = packet_len % 256lu;
    strncpy(data + 4, packet, packet_len + 1);

    *outp = data;
    return 0;
}

static int make_tlv_str(char type, const char* value, char** outp) {
    size_t len = strlen(value);

    if (len > 0xff) return 1;

    char* tlv = malloc((len + 3) * sizeof(char));

    tlv[0] = type;
    tlv[1] = len;
    strncpy(tlv + 2, value, len + 1);

    *outp = tlv;
    return 0;
}

static int make_tlv_int(char type, long unsigned value, char** outp) {
    char buf[9];
    sprintf(buf, "%lu", value);
    return make_tlv_str(type, buf, outp);
}

static int make_control_packet(char control, char** tlvp, size_t n, char** outp) {
    size_t data_len = 1;

    for (size_t i = 0; i < n; ++i) {
        data_len += strlen(tlvp[i]);
    }

    char* data = malloc((data_len + 1) * sizeof(char));
    char* tmp = data + 1;

    data[0] = control;
    data[1] = '\0';

    for (size_t i = 0; i < n; ++i) {
        tmp = stpncpy(tmp, tlvp[i], strlen(tlvp[i]));
    }

    *outp = data;
    return 0;
}

int send_data_packet(int fd, const char* packet) {
    static size_t index = 0; // only supports one fd.

    char* data = NULL;
    make_data_packet(packet, index % 256lu, &data);

    llwrite(fd, data);

    free(data);
    return 0;
}

int send_start_packet(int fd, size_t filesize, const char* filename) {
    char* tlvs[2];

    make_tlv_int(PCONTROL_TYPE_FILESIZE, filesize, tlvs + FILESIZE_TLV_N);
    make_tlv_str(PCONTROL_TYPE_FILENAME, filename, tlvs + FILENAME_TLV_N);

    char* data = NULL;
    make_control_packet(PCONTROL_START, tlvs, 2, &data);

    llwrite(fd, data);

    free(data);
    return 0;
}

int send_end_packet(int fd) {
    char data[2] = {PCONTROL_END, '\0'};

    llwrite(fd, data);

    return 0;
}
*/
