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
#define HELP_FLAG 'h'
#define HELP_LFLAG "help"

// Show help/usage message and exit
#define USAGE_FLAG // NONE
#define USAGE_LFLAG "usage"

// Show version message and exit
#define VERSION_FLAG 'V'
#define VERSION_LFLAG "version"
// ----> END OF GENERAL OPTIONS



// <!--- OPTIONS
// Set number of send retries for link-layer communications, when the
// receiver does not acknowledge the frame.
#define TIME_RETRIES_FLAG 'r'
#define TIME_RETRIES_LFLAG "time-retries"
#define TIME_RETRIES_DEFAULT 5
extern int time_retries;

#define ANSWER_RETRIES_FLAG 'a'
#define ANSWER_RETRIES_LFLAG "answer-retries"
#define ANSWER_RETRIES_DEFAULT 3
extern int answer_retries;

// Set timeout in milliseconds for link-layer communications.
#define TIMEOUT_FLAG 't'
#define TIMEOUT_LFLAG "timeout"
#define TIMEOUT_DEFAULT 1000
extern int timeout;

// Set device (presumably serial port) to use.
#define DEVICE_FLAG 'd'
#define DEVICE_LFLAG "device"
#define DEVICE_DEFAULT "/dev/ttyS0"
extern char* device;

// Set packet size, in bytes
#define PACKETSIZE_FLAG 'p'
#define PACKETSIZE_LFLAG "packetsize"
#define PACKETSIZE_DEFAULT 128
extern size_t packetsize;

// Send or do not send filesize in START packet
#define PACKET_FILESIZE_FLAG // none
#define PACKET_FILESIZE_LFLAG "filesize"
#define PACKET_NOFILESIZE_LFLAG "no-filesize"
#define PACKET_FILESIZE_DEFAULT true
extern int send_filesize;

// Send or do not send filename in START packet
#define PACKET_FILENAME_FLAG // none
#define PACKET_FILENAME_LFLAG "filename"
#define PACKET_NOFILENAME_LFLAG "no-filename"
#define PACKET_FILENAME_DEFAULT true
extern int send_filename;

// Define the behavior when answering incoherent acknowledgments
#define INCOHERENT_CRASH "crash"
#define INCOHERENT_CONTINUE "continue"
#define INCOHERENT_FLAG 'i'
#define INCOHERENT_LFLAG "incoherent"
#define INCOHERENT_DEFAULT "crash"
extern const char* incoherent;
// ----> END OF OPTIONS



void parse_args(int argc, char** argv);

#endif // OPTIONS_H___
