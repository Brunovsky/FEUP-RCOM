#include "options.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <locale.h>
#include <wchar.h>
#include <limits.h>

#define DEBUG true

// <!--- OPTIONS
static int show_help = false; // h, help
static int show_usage = false; // usage
static int show_version = false; // V, version

int retries = RETRIES_DEFAULT; // r, retries
int timeout = TIMEOUT_DEFAULT; // t, timeout
char* device = NULL; // d, device
size_t packetsize = PACKETSIZE_DEFAULT; // p, packetsize
int send_filesize = PACKET_FILESIZE_DEFAULT; // filesize, no-filesize
int send_filename = PACKET_FILENAME_DEFAULT; // filename, no-filename

// Positional
char** files = NULL;
size_t number_of_files = 0;
// ----> END OF OPTIONS



static const struct option long_options[] = {
    // general options
    {HELP_LFLAG,                    no_argument, &show_help,       true},
    {USAGE_LFLAG,                   no_argument, &show_usage,      true},
    {VERSION_LFLAG,                 no_argument, &show_version,    true},

    {RETRIES_LFLAG,           required_argument, NULL,     RETRIES_FLAG},
    {TIMEOUT_LFLAG,           required_argument, NULL,     TIMEOUT_FLAG},
    {DEVICE_LFLAG,            required_argument, NULL,      DEVICE_FLAG},
    {PACKETSIZE_LFLAG,        required_argument, NULL,  PACKETSIZE_FLAG},
    {PACKET_FILESIZE_LFLAG,         no_argument, &send_filesize,   true},
    {PACKET_NOFILESIZE_LFLAG,       no_argument, &send_filesize,  false},
    {PACKET_FILENAME_LFLAG,         no_argument, &send_filename,   true},
    {PACKET_NOFILENAME_LFLAG,       no_argument, &send_filename,  false},
    // end of options
    {0, 0, 0, 0}
    // format: {const char* lflag, int has_arg, int* flag, int val}
    //   {lflag, [no|required|optional]_argument, &var, true|false}
    // or
    //   {lflag, [no|required|optional]_argument, NULL, flag}
};

// Enforce POSIX with leading +
static const char* short_options = "Vhr:t:d:p:";
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
    "Send one or more files through a device using a layered protocol.\n"
    "\n"
    "General:\n"
    "      --help,           \n"
    "      --usage           Show this message and exit\n"
    "  -V, --version         Show 'version' message and exit\n"
    "\n"
    "Options:\n"
    "  -r, --retries=N       Write reattempts for the link-layer.\n"
    "                         Default is 3\n"
    "  -t, --timeout=N       Read timeout for the link-layer, in ms.\n"
    "                         Default is 1000ms\n"
    "  -d, --device=S        Set the device to use.\n"
    "                         Default is /dev/ttyS0\n"
    "  -p, --packetsize=N    Set the packets' size in bytes for the app layer.\n"
    "                         Default is 128\n"
    "      --filesize,\n"
    "      --no-filesize     Whether to send filesize in START packet.\n"
    "      --filename,\n"
    "      --no-filename     Whether to send filename in START packet.\n"
    "                         Default is yes for both\n"
    "\n";

/**
 * Free all resources allocated to contain options by parse_args.
 */
static void clear_options() {
    free(device);

    for (size_t i = 0; i < number_of_files; ++i) {
        free(files[i]);
    }

    free(files);
}

static void print_all() {
    setlocale(LC_ALL, "");
    wprintf(usage);
    wprintf(version);
    exit(EXIT_SUCCESS);
}

static void print_usage() {
    setlocale(LC_ALL, "");
    wprintf(usage);
    exit(EXIT_SUCCESS);
}

static void print_version() {
    setlocale(LC_ALL, "");
    wprintf(version);
    exit(EXIT_SUCCESS);
}

static void print_numpositional(int n) {
    setlocale(LC_ALL, "");
    wprintf(L"Error: Expected 1 or more positional arguments (filenames), but got %d.\n%S", n, usage);
    exit(EXIT_SUCCESS);
}

static void print_badpositional(int i) {
    setlocale(LC_ALL, "");
    wprintf(L"Error: Positional argument #%d is invalid.\n%S", i, usage);
    exit(EXIT_SUCCESS);
}

static void print_badarg(const char* option) {
    setlocale(LC_ALL, "");
    wprintf(L"Error: Bad argument for option %s.\n%S", option, usage);
    exit(EXIT_SUCCESS);
}

static int parse_int(const char* str, int* outp) {
    char* endp;
    long result = strtol(str, &endp, 10);

    if (endp == str || errno == ERANGE || result >= INT_MAX || result <= INT_MIN) {
        return 1;
    } else {
        *outp = (int)result;
        return 0;
    }
}

static int parse_ulong(const char* str, size_t* outp) {
    char* endp;
    long result = strtol(str, &endp, 10);

    if (endp == str || errno == ERANGE || result >= ULONG_MAX || result <= 0) {
        return 1;
    } else {
        *outp = (size_t)result;
        return 0;
    }
}

static void dump_options() {
    static const wchar_t* dump_string = L" === Options ===\n"
        " show_help: %d\n"
        " show_usage: %d\n"
        " show_version: %d\n"
        " retries: %d\n"
        " timeout: %d\n"
        " device: %s\n"
        " packetsize: %lu\n"
        " send_filesize: %d\n"
        " send_filename: %d\n"
        " number_of_files: %d\n"
        " files: %x\n";

    wprintf(dump_string, show_help, show_usage, show_version, retries, timeout,
        device, packetsize, send_filesize, send_filename, number_of_files, files);

    if (files != NULL) {
        for (size_t i = 0; i < number_of_files; ++i) {
            wprintf(L" > file#%lu: %s\n", i, files[i]);
        }
    }
}

/**
 * Standard unix main's argument parsing function. Allocates resources
 * that are automatically freed at exit.
 */
int parse_args(int argc, char** argv) {
    // Uncomment to disable auto-generated error messages for options:
    // opterr = 0;

    atexit(clear_options);

    if (DEBUG) atexit(dump_options);

    // If there are no args, print usage and version messages and exit
    if (argc == 1) {
        print_all();
    }

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
        case RETRIES_FLAG:
            if (parse_int(optarg, &retries) != 0) {
                print_badarg(RETRIES_LFLAG);
            }
            break;
        case TIMEOUT_FLAG:
            if (parse_int(optarg, &timeout) != 0) {
                print_badarg(TIMEOUT_LFLAG);
            }
            break;
        case DEVICE_FLAG:
            device = strdup(optarg);
            break;
        case PACKETSIZE_FLAG:
            if (parse_ulong(optarg, &packetsize) != 0) {
                print_badarg(PACKETSIZE_LFLAG);
            }
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

    if (device == NULL) {
        device = strdup(DEVICE_DEFAULT);
    }

    // Positional arguments processing
    if (optind < argc) {
        number_of_files = argc - optind;
        files = calloc(number_of_files + 1, sizeof(char*));
        
        size_t i = 0;

        do {
            files[i++] = strdup(argv[optind++]);
        } while (optind < argc);
    } else {
        print_numpositional(0);
    }

    return 0;
}

int main(int argc, char** argv) {
    return parse_args(argc, argv);
}
