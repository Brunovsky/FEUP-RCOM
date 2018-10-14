#include "fileio.h"
#include "app-layer.h"
#include "ll-interface.h"
#include "options.h"
#include "debug.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static void free_packets(string* packets, size_t number_packets) {
    for (size_t i = 0; i < number_packets; ++i) {
        free(packets[i].s);
    }
    free(packets);
}

int send_file(int fd, char* filename) {
    int filefd = open(filename, O_RDONLY);
    if (filefd == -1) {
        perror("[FILE] Failed to open file");
        return 1;
    }

    FILE* file = fdopen(filefd, "r");
    if (file == NULL) {
        perror("[FILE]: Failed to open file stream");
        close(filefd);
        return 1;
    }

    // Seek to the end of the file to extract its size.
    int s = fseek(file, 0, SEEK_END);
    if (s != 0) {
        perror("[FILE]: Failed to seek file");
        fclose(file);
        return 1;
    }

    long lfs = ftell(file);
    size_t filesize = lfs;
    rewind(file);

    if (lfs <= 0) {
        printf("[FILE]: Error: filesize could not be read, or is empty\n");
        fclose(file);
        return 1;
    }

    // Read the entire file and close it.
    char* buffer = malloc((filesize + 1) * sizeof(char));
    fread(buffer, filesize, 1, file);
    buffer[filesize] = '\0';

    fclose(file);

    // Split the buffer into various strings.
    // The last one will have a smaller size.
    size_t number_packets = (filesize + packetsize - 1) / packetsize;
    string* packets = malloc(number_packets * sizeof(string));

    for (size_t i = 0; i < number_packets - 1; ++i) {
        size_t offset = i * packetsize;

        packets[i].s = malloc((packetsize + 1) * sizeof(char));
        packets[i].len = packetsize;

        memcpy(packets[i].s, buffer + offset, packetsize);
        packets[i].s[packetsize] = '\0';
    }

    // Last packet
    {
        size_t offset = (number_packets - 1) * packetsize;
        size_t size = filesize - offset;

        packets[number_packets - 1].s = malloc((size + 1) * sizeof(char));
        packets[number_packets - 1].len = size;

        memcpy(packets[number_packets - 1].s, buffer + offset, size);
        packets[number_packets - 1].s[size] = '\0';
    }

    free(buffer);

    // Start communications.
    llopen(fd);
    send_start_packet(fd, filesize, filename);
    if (DEBUG) printf("[FILE] START comms for file %s\n", filename);

    // Send data packets.
    for (size_t i = 0; i < number_packets; ++i) {
        send_data_packet(fd, packets[i]);
    }

    // End communications.
    send_end_packet(fd, filesize, filename);
    llclose(fd);
    if (DEBUG) printf("[FILE] END comms for file %s\n", filename);

    // Free sent packets.
    free_packets(packets, number_packets);

    return 0;
}

int receive_file(int fd) {
    // File variables.
    size_t filesize = 0;
    char* filename = NULL;

    // Packet variables.
    int type;
    control_packet cp;
    data_packet dp;

    // Start communications.
    llopen(fd);
    type = receive_packet(fd, &dp, &cp);

    switch (type) {
    case PRECEIVE_START:
        get_tlv_filesize(cp, &filesize);
        get_tlv_filename(cp, &filename);
        printf("[FILE] START packet: [filesize=%lu] [filename=%s]\n",
            filesize, filename);
        free_control_packet(cp);
        break;
    case PRECEIVE_DATA:
        printf("[FILE] Error: Expected START packet, received DATA packet\n");
        free_data_packet(dp);
        return 1;
    case PRECEIVE_END:
        printf("[FILE] Error: Expected START packet, received END packet\n");
        free_control_packet(cp);
        return 1;
    default:
        printf("[FILE] Error: Expected START packet, received BAD packet\n");
        return 1;
    }

    string* packets = malloc(10 * sizeof(string));
    size_t reserved = 10, number_packets = 0;
    bool reached_end = false;

    while (!reached_end) {
        type = receive_packet(fd, &dp, &cp);

        if (number_packets == reserved) {
            packets = realloc(packets, 2 * reserved * sizeof(string));
            reserved *= 2;
        }

        switch (type) {
        case PRECEIVE_START:
            printf("[FILE] Error: Expected DATA/END packet, received START packet. Continuing\n");
            free_control_packet(cp);
            break;
        case PRECEIVE_DATA:
            if (DEBUG) printf("[FILE] Received DATA packet #%d\n", dp.index);
            packets[number_packets++] = dp.data;
            break;
        case PRECEIVE_END:
            printf("[FILE] END packet: Total DATA packets: %lu\n", number_packets);
            reached_end = true;

            size_t end_filesize = 0;
            char* end_filename = NULL;
            get_tlv_filesize(cp, &end_filesize);
            get_tlv_filename(cp, &end_filename);

            printf("[FILE] END packet: [filesize=%lu] [filename=%s]\n",
                filesize, filename);

            if (DEBUG) {
                if (end_filesize == filesize) {
                    printf("[FILE] END packet: filesize OK\n");
                } else {
                    printf("[FILE] END packet: filesize NOT OK [start=%lu]", filesize);
                }
                if (strcmp(filename, end_filename) == 0) {
                    printf("[FILE] END packet: filename OK\n");
                } else {
                    printf("[FILE] END packet: filename NOT OK [start=%s]", filename);
                }
            }

            free(end_filename);
            free_control_packet(cp);
            break;
        default:
            printf("[FILE] Error: Expected DATA/END packet, received BAD packet. Continuing\n");
            break;
        }
    }

    llclose(fd);
    if (DEBUG) printf("[FILE] Ended comms for file %s\n", filename);

    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        perror("[FILE] Failed to open file");
        free_packets(packets, number_packets);
        return 1;
    }

    printf("[FILE] Writing to file %s...\n", filename);

    for (size_t i = 0; i < number_packets; ++i) {
        fwrite(packets[i].s, packets[i].len, 1, file);
    }

    printf("[FILE] Finished writing to file %s\n", filename);

    fclose(file);

    free_packets(packets, number_packets);
    free(filename);
    return 0;
}
