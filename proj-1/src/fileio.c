#include "fileio.h"
#include "app-layer.h"
#include "ll-interface.h"
#include "options.h"
#include "debug.h"
#include "timing.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

static void free_packets(string* packets, size_t number_packets) {
    for (size_t i = 0; i < number_packets; ++i) {
        free(packets[i].s);
    }
    free(packets);
}

int send_file(int fd, char* filename) {
    int s = 0;

    int filefd = open(filename, O_RDONLY);
    if (filefd == -1) {
        printf("[FILE] Error: Failed to open file %s [%s]\n",
            filename, strerror(errno));
        return 1;
    }

    FILE* file = fdopen(filefd, "r");
    if (file == NULL) {
        printf("[FILE] Error: Failed to open file stream %s [%s]\n",
            filename, strerror(errno));
        close(filefd);
        return 1;
    }

    // Seek to the end of the file to extract its size.
    s = fseek(file, 0, SEEK_END);
    if (s != 0) {
        printf("[FILE] Error: Failed to seek file %s [%s]\n",
            filename, strerror(errno));
        fclose(file);
        return 1;
    }

    long lfs = ftell(file);
    size_t filesize = lfs;
    rewind(file);

    if (lfs <= 0) {
        printf("[FILE] Error: Invalid filesize %ld (probably 0) %s\n",
            filesize, filename);
        fclose(file);
        return 1;
    }

    // Read the entire file and close it.
    char* buffer = malloc((filesize + 1) * sizeof(char));
    fread(buffer, filesize, 1, file);
    buffer[filesize] = '\0';

    fclose(file);

    if (TRACE_FILE) {
        printf("[FILE] File read and closed [filesize=%lu,filename=%s]\n",
            filesize, filename);
    }

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
    begin_timing(0);
    s = llopen(fd);
    if (s != LL_OK) goto error;

    if (TRACE_FILE) printf("[FILE] BEGIN Packets %s\n", filename);

    begin_timing(1);
    s = send_start_packet(fd, filesize, filename);
    if (s != LL_OK) goto error;

    // Send data packets.
    for (size_t i = 0; i < number_packets; ++i) {
        s = send_data_packet(fd, packets[i]);
        if (s != LL_OK) goto error;
    }

    // End communications.
    s = send_end_packet(fd, filesize, filename);
    if (s != LL_OK) goto error;
    end_timing(1);

    if (TRACE_FILE) printf("[FILE] END Packets %s\n", filename);

    s = llclose(fd);
    end_timing(0);

    print_stats(1, filesize);

    free_packets(packets, number_packets);
    return s ? 1 : 0;

error:
    free_packets(packets, number_packets);
    return 1;
}

int receive_file(int fd) {
    int s = 0;

    // File variables.
    size_t filesize = 0;
    char* filename = NULL;

    // Packet variables.
    int type;
    control_packet cp;
    data_packet dp;

    // Start communications.
    begin_timing(0);
    s = llopen(fd);
    if (s != LL_OK) return 1;

    if (TRACE_FILE) printf("[FILE] BEGIN Packets\n");
    begin_timing(1);

    type = receive_packet(fd, &dp, &cp);

    switch (type) {
    case PRECEIVE_START:
        get_tlv_filesize(cp, &filesize);
        get_tlv_filename(cp, &filename);
        if (TRACE_FILE) {
            printf("[FILE] Received START packet [filesize=%lu,filename=%s]\n",
                filesize, filename);
        }
        free_control_packet(cp);
        break;
    case PRECEIVE_DATA:
        printf("[FILE] Error: Expected START packet, received DATA packet. Exiting\n");
        free_data_packet(dp);
        return 1;
    case PRECEIVE_END:
        printf("[FILE] Error: Expected START packet, received END packet. Exiting\n");
        free_control_packet(cp);
        return 1;
    case PRECEIVE_BAD_PACKET: default:
        printf("[FILE] Error: Expected START packet, received BAD packet. Exiting\n");
        return 1;
    }

    string* packets = malloc(10 * sizeof(string));
    size_t reserved = 10, number_packets = 0;
    bool done = false, reached_end = false;

    while (!done) {
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
            packets[number_packets++] = dp.data;
            break;
        case PRECEIVE_END:
            done = true;
            reached_end = true;

            size_t end_filesize = 0;
            char* end_filename = NULL;
            get_tlv_filesize(cp, &end_filesize);
            get_tlv_filename(cp, &end_filename);

            if (TRACE_FILE) {
                printf("[FILE] Received END packet [ndata=%lu,filesize=%lu,filename=%s]\n",
                    number_packets, filesize, filename);

                if (end_filesize == filesize) {
                    printf("[FILE] END packet: filesize OK\n");
                } else {
                    printf("[FILE] END packet: filesize NOT OK [start=%lu]",
                        filesize);
                }
                
                if (strcmp(filename, end_filename) == 0) {
                    printf("[FILE] END packet: filename OK\n");
                } else {
                    printf("[FILE] END packet: filename NOT OK [start=%s]",
                        filename);
                }
            }

            free(end_filename);
            free_control_packet(cp);
            break;
        case PRECEIVE_BAD_PACKET: default:
            printf("[FILE] Error: Expected DATA/END packet, received BAD packet. Exiting\n");
            done = true;
            break;
        }
    }

    if (!reached_end) goto error;

    end_timing(1);
    if (TRACE_FILE) printf("[FILE] END Packets %s\n", filename);

    s = llclose(fd);
    if (s != LL_OK) {
        printf("[FILE] llclose failed. Writing to file %s anyway\n", filename);
    }
    end_timing(0);

    print_stats(1, filesize);

    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        perror("[FILE] Failed to open output file");
        goto error;
    }

    if (TRACE_FILE) printf("[FILE] Writing to file %s...\n", filename);

    for (size_t i = 0; i < number_packets; ++i) {
        fwrite(packets[i].s, packets[i].len, 1, file);
    }

    if (TRACE_FILE) printf("[FILE] Finished writing to file %s\n", filename);

    fclose(file);

    free_packets(packets, number_packets);
    free(filename);
    return s ? 1 : 0;

error:
    free_packets(packets, number_packets);
    free(filename);
    return 1;
}

int send_files(int fd) {
    for (size_t i = 0; i < number_of_files; ++i) {
        int s = send_file(fd, files[i]);
        if (s != 0) return 1;
    }
    return 0;
}

int receive_files(int fd) {
    for (size_t i = 0; i < number_of_files; ++i) {
        int s = receive_file(fd);
        if (s != 0) return 1;
    }
    return 0;
}
