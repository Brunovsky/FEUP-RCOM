#ifndef OPTIONS_H___
#define OPTIONS_H___

#include <stddef.h>
#include <stdbool.h>

// NOTE: All externs are resolved in options.c
// Too add/edit an option, do:
//      1. Add a block entry here
//      2. Resolve the extern in options.c
//      3. Update short_options and long_options in options.c
//      4. Update the usage string in options.c
//      5. Update parse_args() in options.c
//      6. Update dump_options() in options.c
// then go on to add the option's functionality...



// <!--- GENERAL OPTIONS
// Show help/usage message and exit
#define HELP_FLAG '0'
#define HELP_LFLAG "help"

// Show help/usage message and exit
#define USAGE_FLAG // NONE
#define USAGE_LFLAG "usage"

// Show version message and exit
#define VERSION_FLAG 'V'
#define VERSION_LFLAG "version"

// Dump options and exit
#define DUMP_FLAG // none
#define DUMP_LFLAG "dump"
// ----> END OF GENERAL OPTIONS



// <!--- OPTIONS
// Set number of send retries for link-layer communications, when the
// receiver does not acknowledge the frame.
#define TIME_RETRIES_FLAG '1'
#define TIME_RETRIES_LFLAG "time"
#define TIME_RETRIES_DEFAULT 5
extern int time_retries;

#define ANSWER_RETRIES_FLAG 'a'
#define ANSWER_RETRIES_LFLAG "answer"
#define ANSWER_RETRIES_DEFAULT 100
extern int answer_retries;

// Set timeout in deciseconds for link-layer communications.
#define TIMEOUT_FLAG '2'
#define TIMEOUT_LFLAG "timeout"
#define TIMEOUT_DEFAULT 10
extern int timeout;

// Set device (presumably serial port) to use.
#define DEVICE_FLAG 'd'
#define DEVICE_LFLAG "device"
#define DEVICE_DEFAULT "/dev/ttyS0"
extern char* device;

// Set packet size, in bytes
#define PACKETSIZE_FLAG 's'
#define PACKETSIZE_LFLAG "packetsize"
#define PACKETSIZE_DEFAULT 1024
extern size_t packetsize;

// Set baudrate
#define BAUDRATE_FLAG 'b'
#define BAUDRATE_LFLAG "baudrate"
#define BAUDRATE_DEFAULT 115200
extern int baudrate;

// Send or do not send filesize in START packet
#define PACKET_FILESIZE_FLAG // none
#define PACKET_FILESIZE_LFLAG "filesize"
#define PACKET_NOFILESIZE_LFLAG "no-filesize"
#define PACKET_FILESIZE_DEFAULT true
extern int send_filesize; // NOT IMPLEMENTED

// Send or do not send filename in START packet
#define PACKET_FILENAME_FLAG // none
#define PACKET_FILENAME_LFLAG "filename"
#define PACKET_NOFILENAME_LFLAG "no-filename"
#define PACKET_FILENAME_DEFAULT true
extern int send_filename; // NOT IMPLEMENTED

#define TRANSMITTER_FLAG 't'
#define TRANSMITTER_LFLAG "transmitter"
#define TRANSMITTER 1
#define RECEIVER_FLAG 'r'
#define RECEIVER_LFLAG "receiver"
#define RECEIVER 2
#define DEFAULT_ROLE RECEIVER
extern int my_role;
extern const char* role_string;

#define HEADER_ERROR_P_FLAG 'h'
#define FRAME_ERROR_P_FLAG 'f'
#define HEADER_ERROR_P_LFLAG "header-p"
#define FRAME_ERROR_P_LFLAG "frame-p"
#define H_ERROR_PROB_DEFAULT 0.0
#define F_ERROR_PROB_DEFAULT 0.0
extern double h_error_prob;
extern double f_error_prob;

#define ETYPE_FLAG // none
#define ETYPE_BYTE_LFLAG "error-byte"
#define ETYPE_FRAME_LFLAG "error-frame"
#define ETYPE_BYTE 0x71
#define ETYPE_FRAME 0x72
#define ETYPE_DEFAULT ETYPE_FRAME
extern int error_type;

#define STATS_FLAG '3'
#define STATS_LFLAG "stats"
#define STATS_DEFAULT false
extern int show_statistics;
// ----> END OF OPTIONS

// <!--- POSITIONAL
extern char** files;

extern size_t number_of_files;
// ----> END POSITIONAL

void parse_args(int argc, char** argv);

#endif // OPTIONS_H___
