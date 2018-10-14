#include "app-layer.h"
#include "ll-interface.h"
#include "debug.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

void free_control_packet(control_packet packet) {
    for (size_t i = 0; i < packet.n; ++i) {
        free(packet.tlvs[i].value.s);
    }
    free(packet.tlvs);
}

void free_data_packet(data_packet packet) {
    free(packet.data.s);
}

bool isDATApacket(string packet_str, data_packet* outp) {
    if (packet_str.len < 5 || packet_str.s == NULL) return false;

    char c = packet_str.s[0];
    int index = (unsigned char)packet_str.s[1];
    unsigned char l2 = packet_str.s[2];
    unsigned char l1 = packet_str.s[3];
    size_t len = (size_t)l1 + 256 * (size_t)l2;

    bool b = c == PCONTROL_DATA && len == (packet_str.len - 4);

    if (b) {
        string data;

        data.len = len;
        data.s = malloc((len + 1) * sizeof(char));
        memcpy(data.s, packet_str.s + 4, len + 1);

        data_packet out = {index, data};

        *outp = out;
    }

    if (DEBUG) printf("[APP] isDATApacket() ? %d | index=%d\n", (int)b, index % 256);
    return b;
}

bool isCONTROLpacket(char c, string packet_str, control_packet* outp) {
    if (packet_str.len == 0 || packet_str.s == NULL) return false;

    char control_char = packet_str.s[0];

    if (control_char != c) return false;

    control_packet out;
    out.c = c;
    out.n = 0;
    
    out.tlvs = malloc(2 * sizeof(tlv));
    size_t reserved = 2;

    size_t j = 1;

    while (j + 2 < packet_str.len) {
        char type = packet_str.s[j++];
        size_t l = (unsigned char)packet_str.s[j++];

        if (j + l > packet_str.len) break;

        if (out.n == reserved) {
            out.tlvs = realloc(out.tlvs, 2 * reserved);
            reserved *= 2;
        }

        string value;

        value.len = l;
        value.s = malloc((l + 1) * sizeof(char));
        memcpy(value.s, packet_str.s + j, l);
        value.s[l] = '\0';

        out.tlvs[out.n++] = (tlv){type, value};

        j += l;
    }

    bool b = j == packet_str.len;

    if (!b) {
        free_control_packet(out);
    } else {
        *outp = out;
    }

    return b;
}

bool isSTARTpacket(string packet_str, control_packet* outp) {
    bool b = isCONTROLpacket(PCONTROL_START, packet_str, outp);

    if (DEBUG) printf("[APP] isSTARTpacket() ? %d\n", (int)b);
    return b;
}

bool isENDpacket(string packet_str, control_packet* outp) {
    bool b = isCONTROLpacket(PCONTROL_END, packet_str, outp);

    if (DEBUG) printf("[APP] isENDpacket() ? %d\n", (int)b);
    return b;
}


bool get_tlv(control_packet control, char type, string* outp) {
    for (size_t i = 0; i < control.n; ++i) {
        if (control.tlvs[i].type == type) {
            string value;
            value.len = control.tlvs[i].value.len;
            value.s = strdup(control.tlvs[i].value.s);
            *outp = value;
            return true;
        }
    }
    return false;
}

bool get_tlv_filename(control_packet control, string* outp) {
    return get_tlv(control, PCONTROL_TYPE_FILENAME, outp);
}

bool get_tlv_filesize(control_packet control, size_t* outp) {
    string value;
    bool b = get_tlv(control, PCONTROL_TYPE_FILESIZE, &value);

    if (!b) return false;

    long parse = strtol(value.s, NULL, 10);

    free(value.s);
    if (parse <= 0) return false;

    *outp = (size_t)parse;
    return true;
}

int build_data_packet(string fragment, char index, string* outp) {
    static const size_t mod = 256;
    static const size_t max_len = 0x0ffff;

    if (fragment.len > max_len) return 1;

    string data_packet;

    data_packet.len = fragment.len + 4;
    data_packet.s = malloc((data_packet.len + 1) * sizeof(char));

    data_packet.s[0] = PCONTROL_DATA;
    data_packet.s[1] = index;
    data_packet.s[2] = fragment.len / mod;
    data_packet.s[3] = fragment.len % mod;
    memcpy(data_packet.s + 4, fragment.s, fragment.len + 1);

    if (DEBUG) {
        printf("[APP] Built DP %x %x %x %x\n", data_packet.s[0],
            data_packet.s[1], data_packet.s[2], data_packet.s[3]);
        print_stringn(data_packet);
    }

    *outp = data_packet;
    return 0;
}

int build_tlv_str(char type, string value, string* outp) {
    static const size_t max_len = 0x0ff;

    if (value.len > max_len) return 1;

    string tlv;

    tlv.len = value.len + 2;
    tlv.s = malloc((tlv.len + 3) * sizeof(char));

    tlv.s[0] = type;
    tlv.s[1] = value.len;
    memcpy(tlv.s + 2, value.s, value.len + 1);

    if (DEBUG) {
        printf("[APP] Built TLV %x %lx\n", type, value.len);
        print_stringn(tlv);
    }

    *outp = tlv;
    return 0;
}

int build_tlv_uint(char type, long unsigned value, string* outp) {
    char buf[10];
    string tmp;
    tmp.s = buf;
    sprintf(tmp.s, "%lu", value);
    tmp.len = strlen(tmp.s);
    return build_tlv_str(type, tmp, outp);
}

int build_control_packet(char control, string* tlvp, size_t n, string* outp) {
    string control_packet;
    control_packet.len = 1;

    for (size_t i = 0; i < n; ++i) {
        control_packet.len += tlvp[i].len;
    }

    control_packet.s = malloc((control_packet.len + 1) * sizeof(char));
    char* tmp = control_packet.s + 1;

    control_packet.s[0] = control;
    control_packet.s[1] = '\0';

    for (size_t i = 0; i < n; ++i) {
        memcpy(tmp, tlvp[i].s, tlvp[i].len);
        tmp += tlvp[i].len;
        free(tlvp[i].s);
    }
    free(tlvp);

    if (DEBUG) {
        printf("[APP] Built CP %x\n", control);
        print_stringn(control_packet);
    }

    *outp = control_packet;
    return 0;
}



static int out_packet_index = 0; // only supports one fd.
static int in_packet_index = 0; // only supports one fd.

int send_data_packet(int fd, string packet) {
    int s;

    if (DEBUG) {
        printf("[APP] Sending packet #%d\n", out_packet_index);
        print_stringn(packet);
    }

    string data_packet;
    s = build_data_packet(packet, out_packet_index++ % 256lu, &data_packet);
    if (s != 0) return s;

    s = llwrite(fd, data_packet);
    free(data_packet.s);
    return s;
}

int send_start_packet(int fd, size_t filesize, char* filename) {
    int s;
    string tlvs[2];

    out_packet_index = 0;

    s = build_tlv_uint(PCONTROL_TYPE_FILESIZE,
        filesize, tlvs + FILESIZE_TLV_N);
    if (s != 0) return s;

    s = build_tlv_str(PCONTROL_TYPE_FILENAME,
        string_from(filename), tlvs + FILENAME_TLV_N);
    if (s != 0) return s;

    string start_packet;
    s = build_control_packet(PCONTROL_START, tlvs, 2, &start_packet);
    if (s != 0) return s;

    s = llwrite(fd, start_packet);
    free(start_packet.s);
    return s;
}

int send_end_packet(int fd, size_t filesize, char* filename) {
    int s;
    string tlvs[2];

    s = build_tlv_uint(PCONTROL_TYPE_FILESIZE,
        filesize, tlvs + FILESIZE_TLV_N);
    if (s != 0) return s;

    s = build_tlv_str(PCONTROL_TYPE_FILENAME,
        string_from(filename), tlvs + FILENAME_TLV_N);
    if (s != 0) return s;

    string end_packet;
    s = build_control_packet(PCONTROL_END, tlvs, 2, &end_packet);
    if (s != 0) return s;

    s = llwrite(fd, end_packet);
    free(end_packet.s);
    return s;
}

int receive_packet(int fd, data_packet* datap, control_packet* controlp) {
    int s;

    string packet;
    s = llread(fd, &packet);
    if (s != 0) return s;

    data_packet data;
    control_packet control;

    if (DEBUG) {
        printf("Whatis?\n");
        print_stringn(packet);
    }

    free(packet.s);

    if (isSTARTpacket(packet, &control)) {
        in_packet_index = 0;
        *controlp = control;

        return PCONTROL_START;
    }

    if (isDATApacket(packet, &data)) {
        if (data.index != in_packet_index) {
            printf("[APP] Error: Expected packet #%d, got #%d\n",
                in_packet_index, data.index);
        }
        ++in_packet_index;
        *datap = data;

        return PCONTROL_DATA;
    }

    if (isENDpacket(packet, &control)) {
        in_packet_index = 0;
        *controlp = control;

        return PCONTROL_END;
    }

    return PCONTROL_BAD_PACKET;
}
