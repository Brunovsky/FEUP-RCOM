#include "options.h"
#include "debug.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <locale.h>
#include <wchar.h>
#include <limits.h>

// <!--- OPTIONS
static int show_help = false; // h, help
static int show_usage = false; // usage
static int show_version = false; // V, version

int time_retries = TIME_RETRIES_DEFAULT; // time-retries
int answer_retries = ANSWER_RETRIES_DEFAULT; // answer-retries
int timeout = TIMEOUT_DEFAULT; // timeout
char* device = DEVICE_DEFAULT; // d, device
size_t packetsize = PACKETSIZE_DEFAULT; // p, packetsize
int send_filesize = PACKET_FILESIZE_DEFAULT; // filesize, no-filesize
int send_filename = PACKET_FILENAME_DEFAULT; // filename, no-filename
int my_role = DEFAULT_ROLE; // t, transmitter, r, receiver

// Positional
char** files = NULL;
size_t number_of_files = 0;
// ----> END OF OPTIONS



static const struct option long_options[] = {
    // general options
    {HELP_LFLAG,                    no_argument, &show_help,          true},
    {USAGE_LFLAG,                   no_argument, &show_usage,         true},
    {VERSION_LFLAG,                 no_argument, &show_version,       true},

    {TIME_RETRIES_LFLAG,      required_argument, NULL,   TIME_RETRIES_FLAG},
    {ANSWER_RETRIES_LFLAG,    required_argument, NULL, ANSWER_RETRIES_FLAG},
    {TIMEOUT_LFLAG,           required_argument, NULL,        TIMEOUT_FLAG},
    {DEVICE_LFLAG,            required_argument, NULL,         DEVICE_FLAG},
    {PACKETSIZE_LFLAG,        required_argument, NULL,     PACKETSIZE_FLAG},
    {PACKET_FILESIZE_LFLAG,         no_argument, &send_filesize,      true},
    {PACKET_NOFILESIZE_LFLAG,       no_argument, &send_filesize,     false},
    {PACKET_FILENAME_LFLAG,         no_argument, &send_filename,      true},
    {PACKET_NOFILENAME_LFLAG,       no_argument, &send_filename,     false},
    {TRANSMITTER_LFLAG,             no_argument, NULL,    TRANSMITTER_FLAG},
    {RECEIVER_LFLAG,                no_argument, NULL,       RECEIVER_FLAG},
    // end of options
    {0, 0, 0, 0}
    // format: {const char* lflag, int has_arg, int* flag, int val}
    //   {lflag, [no|required|optional]_argument, &var, true|false}
    // or
    //   {lflag, [no|required|optional]_argument, NULL, flag}
};

// Enforce POSIX with leading +
static const char* short_options = "Vhd:p:i:rt";
// x for no_argument, x: for required_argument,
// x:: for optional_argument (GNU extension),
// x; to transform  -x foo  into  --foo

static const wchar_t* version = L"FEUP RCOM 2018-2019\n"
    "RCOM Comunicação Série v1.0\n"
    "  Bruno Carvalho     up201606517\n"
    "  João Malheiro      up123456789\n"
    "  Daniel Gomes       up123456789\n"
    "\n";

static const wchar_t* usage = L"usage: ll [option]... files...\n"
    "Send one or more files through a device using a layered protocol.       \n"
    "                                                                        \n"
    "General:                                                                \n"
    "      --help,                                                           \n"
    "      --usage                  Show this message and exit               \n"
    "  -V, --version                Show 'version' message and exit          \n"
    "                                                                        \n"
    "Options:                                                                \n"
    "      --time-retries=N         Write stop&wait attempts for the         \n"
    "                               link-layer when timeout occurs.          \n"
    "                                Default is 3                            \n"
    "      --answer-retries=N       Write stop&wait attempts for the         \n"
    "                               link-layer when an answer is invalid.    \n"
    "                                Default is 5                            \n"
    "      --timeout=N              Read timeout for the link-layer, in ds.  \n"
    "                                Default is 10 (1s)                      \n"
    "  -d, --device=S               Set the device to use.                   \n"
    "                                Default is /dev/ttyS0                   \n"
    "  -p, --packetsize=N           Set the packets' size, in bytes.         \n"
    "                                Default is 128 bytes                    \n"
    "      --filesize,                                                       \n"
    "      --no-filesize            Send filesize in START packet.           \n"
    "      --filename,                                                       \n"
    "      --no-filename            Send filename in START packet.           \n"
    "                                Default is yes for both                 \n"
    "  -t, --transmitter,                                                    \n"
    "  -r, --receiver               Set the program's role.                  \n"
    "                                Default is receiver                     \n"
    "\n";

/**
 * Free all resources allocated to contain options by parse_args.
 */
static void clear_options() {    
    for (size_t i = 0; i < number_of_files; ++i) {
        free(files[i]);
    }

    free(files);
}

static void dump_options() {
    static const char* dump_string = " === Options ===\n"
        " show_help: %d\n"
        " show_usage: %d\n"
        " show_version: %d\n"
        " time_retries: %d\n"
        " answer_retries: %d\n"
        " timeout: %d\n"
        " device: %s\n"
        " packetsize: %lu\n"
        " send_filesize: %d\n"
        " send_filename: %d\n"
        " my_role: %d (T=%d, R=%d)\n"
        " number_of_files: %d\n"
        " files: 0x%08x\n";

    printf(dump_string, show_help, show_usage, show_version, time_retries,
        answer_retries, timeout, device, packetsize, send_filesize,
        send_filename, my_role, TRANSMITTER, RECEIVER, number_of_files, files);

    if (files != NULL) {
        for (size_t i = 0; i < number_of_files; ++i) {
            printf(" > file#%lu: %s\n", i, files[i]);
        }
    }
}

static void print_all() {
    if (DUMP_OPTIONS) dump_options();

    setlocale(LC_ALL, "");
    printf("%ls%ls", usage, version);
    exit(EXIT_SUCCESS);
}

static void print_usage() {
    if (DUMP_OPTIONS) dump_options();
    
    setlocale(LC_ALL, "");
    printf("%ls", usage);
    exit(EXIT_SUCCESS);
}

static void print_version() {
    if (DUMP_OPTIONS) dump_options();
    
    setlocale(LC_ALL, "");
    printf("%ls", version);
    exit(EXIT_SUCCESS);
}

static void print_numpositional(int n) {
    if (DUMP_OPTIONS) dump_options();
    
    setlocale(LC_ALL, "");
    printf("[ARGS] Error: Expected 1 or more positional arguments (filenames), but got %d.\n", n);
    printf("%ls", usage);
    exit(EXIT_SUCCESS);
}

static void print_badpositional(int i) {
    if (DUMP_OPTIONS) dump_options();
    
    setlocale(LC_ALL, "");
    printf("[ARGS] Error: Positional argument #%d is invalid.\n", i);
    printf("%ls", usage);
    exit(EXIT_SUCCESS);
}

static void print_badarg(const char* option) {
    if (DUMP_OPTIONS) dump_options();
    
    setlocale(LC_ALL, "");
    wprintf(L"[ARGS] Error: Bad argument for option %s.\n%S", option, usage);
    exit(EXIT_SUCCESS);
}

static int parse_int(const char* str, int* outp) {
    char* endp;
    long result = strtol(str, &endp, 10);

    if (endp == str || errno == ERANGE || result >= INT_MAX || result <= INT_MIN) {
        return 1;
    }

    *outp = (int)result;
    return 0;
}

static int parse_ulong(const char* str, size_t* outp) {
    char* endp;
    long result = strtol(str, &endp, 10);

    if (endp == str || errno == ERANGE || result <= 0) {
        return 1;
    }

    *outp = (size_t)result;
    return 0;
}

/**
 * Standard unix main's argument parsing function. Allocates resources
 * that are automatically freed at exit.
 */
void parse_args(int argc, char** argv) {
    // Uncomment to disable auto-generated error messages for options:
    // opterr = 0;

    atexit(clear_options);

    // Standard getopt_long Options Loop
    while (true) {
        int c, lindex = 0;

        c = getopt_long(argc, argv, short_options,
            long_options, &lindex);

        if (c == -1) break; // No more options

        switch (c) {
        case 0:
            // If this option set a flag, do nothing else now.
            if (long_options[lindex].flag == NULL) {
                // ... Long option with non-null var ...
                // getopt_long already set the flag.
                // Inside this is normally a nested switch
                // *** Access option using long_options[index].*
                // (or struct option opt = long_options[index])
                // optarg - contains value of argument
                // optarg == NULL if no argument was provided
                // to a field with optional_argument.
            }
            break;
        case HELP_FLAG:
            show_help = true;
            break;
        case VERSION_FLAG:
            show_version = true;
            break;
        case TIME_RETRIES_FLAG:
            if (parse_int(optarg, &time_retries) != 0) {
                print_badarg(TIME_RETRIES_LFLAG);
            }
            break;
        case ANSWER_RETRIES_FLAG:
            if (parse_int(optarg, &answer_retries) != 0) {
                print_badarg(ANSWER_RETRIES_LFLAG);
            }
            break;
        case TIMEOUT_FLAG:
            if (parse_int(optarg, &timeout) != 0) {
                print_badarg(TIMEOUT_LFLAG);
            }
            break;
        case DEVICE_FLAG:
            device = optarg;
            break;
        case PACKETSIZE_FLAG:
            if (parse_ulong(optarg, &packetsize) != 0) {
                print_badarg(PACKETSIZE_LFLAG);
            }
            break;
        case TRANSMITTER_FLAG:
            my_role = TRANSMITTER;
            break;
        case RECEIVER_FLAG:
            my_role = RECEIVER;
            break;
        case '?':
        default:
            // getopt_long already printed an error message.
            print_usage();
        }
    } // End Options loop

    if (show_help || show_usage) {
        print_usage();
    }

    if (show_version) {
        print_version();
    }

    // Positional arguments processing
    if (optind < argc) {
        number_of_files = argc - optind;
        files = calloc(number_of_files + 1, sizeof(char*));
        
        size_t i = 0;

        do {
            files[i++] = strdup(argv[optind++]);
        } while (optind < argc);
    }

    if (DUMP_OPTIONS) dump_options();
}
