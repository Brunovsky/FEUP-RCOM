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

static int make_data_packet(string fragment, char index, string* outp) {
    if (fragment.len > 0xfffflu) return 1;

    string data_packet;

    data_packet.len = fragment.len + 4;
    data_packet.s = malloc((data_packet.len + 1) * sizeof(char));

    data_packet.s[0] = PCONTROL_DATA;
    data_packet.s[1] = index;
    data_packet.s[2] = fragment.len / 256lu;
    data_packet.s[3] = fragment.len % 256lu;
    strncpy(data_packet.s + 4, fragment.s, fragment.len + 1);

    *outp = data_packet;
    return 0;
}

static int make_tlv_str(char type, string value, string* outp) {
    if (value.len > 0xfflu) return 1;

    string tlv;

    tlv.len = value.len + 2;
    tlv.s = malloc((tlv.len + 3) * sizeof(char));

    tlv.s[0] = type;
    tlv.s[1] = value.len;
    strncpy(tlv.s + 2, value.s, value.len + 1);

    *outp = tlv;
    return 0;
}

static int make_tlv_uint(char type, long unsigned value, string* outp) {
    char buf[10];
    string tmp;
    tmp.s = buf;
    sprintf(tmp.s, "%lu", value);
    tmp.len = strlen(tmp.s);
    return make_tlv_str(type, tmp, outp);
}

static int make_control_packet(char control, string* tlvp, size_t n, string* outp) {
    string control_packet;
    control_packet.len = 1;

    for (size_t i = 0; i < n; ++i) {
        control_packet.len += strlen(tlvp[i].s);
    }

    control_packet.s = malloc((control_packet.len + 1) * sizeof(char));
    char* tmp = control_packet.s + 1;

    control_packet.s[0] = control;
    control_packet.s[1] = '\0';

    for (size_t i = 0; i < n; ++i) {
        tmp = stpncpy(tmp, tlvp[i].s, tlvp[i].len);
    }

    *outp = control_packet;
    return 0;
}

int send_data_packet(int fd, string packet) {
    static size_t index = 0; // only supports one fd.

    string data_packet;
    make_data_packet(packet, index % 256lu, &data_packet);

    llwrite(fd, data_packet);

    free(data_packet.s);
    return 0;
}

int send_start_packet(int fd, size_t filesize, char* filename) {
    string tlvs[2];

    make_tlv_uint(PCONTROL_TYPE_FILESIZE, filesize, tlvs + FILESIZE_TLV_N);
    make_tlv_str(PCONTROL_TYPE_FILENAME, string_from(filename), tlvs + FILENAME_TLV_N);

    string start_packet;
    make_control_packet(PCONTROL_START, tlvs, 2, &start_packet);

    llwrite(fd, start_packet);

    free(start_packet.s);
    return 0;
}

int send_end_packet(int fd) {
    static char data[2] = {PCONTROL_END, '\0'};
    string end_packet = {data, 1};

    llwrite(fd, end_packet);

    return 0;
}
