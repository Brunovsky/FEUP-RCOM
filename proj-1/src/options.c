#include "options.h"
#include "ll-setup.h"
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
static int dump = false; // dump

int time_retries = TIME_RETRIES_DEFAULT; // time-retries
int answer_retries = ANSWER_RETRIES_DEFAULT; // answer-retries
int timeout = TIMEOUT_DEFAULT; // timeout
int baudrate = BAUDRATE_DEFAULT;
char* device = DEVICE_DEFAULT; // d, device
size_t packetsize = PACKETSIZE_DEFAULT; // p, packetsize
int send_filesize = PACKET_FILESIZE_DEFAULT; // filesize, no-filesize
int send_filename = PACKET_FILENAME_DEFAULT; // filename, no-filename
int my_role = DEFAULT_ROLE; // t, transmitter, r, receiver
const char* role_string = NULL;
double h_error_prob = H_ERROR_PROB_DEFAULT; // header-p
double f_error_prob = F_ERROR_PROB_DEFAULT; // frame-p
int error_type = ETYPE_DEFAULT; // error-byte, error-frame
int show_statistics = STATS_DEFAULT;

// Positional
char** files = NULL;
size_t number_of_files = 0;
// ----> END OF OPTIONS



static const struct option long_options[] = {
    // general options
    {HELP_LFLAG,                    no_argument, &show_help,                true},
    {USAGE_LFLAG,                   no_argument, &show_usage,               true},
    {VERSION_LFLAG,                 no_argument, &show_version,             true},
    {DUMP_LFLAG,                    no_argument, &dump,                     true},

    {TIME_RETRIES_LFLAG,      required_argument, NULL,         TIME_RETRIES_FLAG},
    {ANSWER_RETRIES_LFLAG,    required_argument, NULL,       ANSWER_RETRIES_FLAG},
    {TIMEOUT_LFLAG,           required_argument, NULL,              TIMEOUT_FLAG},
    {BAUDRATE_LFLAG,          required_argument, NULL,             BAUDRATE_FLAG},
    {DEVICE_LFLAG,            required_argument, NULL,               DEVICE_FLAG},
    {PACKETSIZE_LFLAG,        required_argument, NULL,           PACKETSIZE_FLAG},
    //{PACKET_FILESIZE_LFLAG,         no_argument, &send_filesize,            true},
    //{PACKET_NOFILESIZE_LFLAG,       no_argument, &send_filesize,           false},
    //{PACKET_FILENAME_LFLAG,         no_argument, &send_filename,            true},
    //{PACKET_NOFILENAME_LFLAG,       no_argument, &send_filename            false},
    {TRANSMITTER_LFLAG,             no_argument, NULL,          TRANSMITTER_FLAG},
    {RECEIVER_LFLAG,                no_argument, NULL,             RECEIVER_FLAG},
    {HEADER_ERROR_P_LFLAG,    required_argument, NULL,       HEADER_ERROR_P_FLAG},
    {FRAME_ERROR_P_LFLAG,     required_argument, NULL,        FRAME_ERROR_P_FLAG},
    {ETYPE_BYTE_LFLAG,              no_argument, &error_type,         ETYPE_BYTE},
    {ETYPE_FRAME_LFLAG,             no_argument, &error_type,        ETYPE_FRAME},
    {NOSTATS_LFLAG,                 no_argument, &show_statistics,    STATS_NONE},
    {STATS_LFLAG,                   no_argument, &show_statistics,    STATS_LONG},
    {COMPACT_LFLAG,                 no_argument, &show_statistics, STATS_COMPACT},
    // end of options
    {0, 0, 0, 0}
    // format: {const char* lflag, int has_arg, int* flag, int val}
    //   {lflag, [no|required|optional]_argument, &var, true|false}
    // or
    //   {lflag, [no|required|optional]_argument, NULL, flag}
};

// Enforce POSIX with leading +
static const char* short_options = "Vtra:d:s:b:i:h:f:";
// x for no_argument, x: for required_argument,
// x:: for optional_argument (GNU extension),
// x; to transform  -x foo  into  --foo

static const wchar_t* version = L"FEUP RCOM 2018-2019\n"
    "RCOM Comunicação Série v1.0\n"
    "  Bruno Carvalho        up201606517\n"
    "  João Malheiro         up201605926\n"
    "  Carlos Daniel Gomes   up201603404\n"
    "\n";

static const wchar_t* usage = L"usage:\n"
    "                                                                     \n"
    "When TRANSMITTER:                                                    \n"
    "    ./ll -t [option...] files...                                     \n"
    "                                                                     \n"
    "When RECEIVER:                                                       \n"
    "    ./ll -r [option...] number_of_files                              \n"
    "                                                                     \n"
    "Send one or more files through a device using a layered protocol.    \n"
    "                                                                     \n"
    "General:                                                             \n"
    "      --help,                                                        \n"
    "      --usage                  Show this message and exit            \n"
    "  -V, --version                Show 'version' message and exit       \n"
    "      --dump                   Dump options and exit                 \n"
    "                                                                     \n"
    "Options:                                                             \n"
    "      --time=N                 Write stop&wait attempts for the      \n"
    "                               link-layer when timeout occurs.       \n"
    "                                 [Default is 5]                      \n"
    "  -a, --answer=N               Write stop&wait attempts for the      \n"
    "                               link-layer when an answer is invalid. \n"
    "                                 [Default is 100]                    \n"
    "      --timeout=N              Timeout for the link-layer, in ds.    \n"
    "                                 [Default is 10]                     \n"
    "  -b, --baudrate=N             Set the connection's baudrate.        \n"
    "                               Should be equal for T and R.          \n"
    "                                 [Default is 115200]                 \n"
    "  -d, --device=S               Set the device.                       \n"
    "                                 [Default is /dev/ttyS0]             \n"
    "  -s, --packetsize=N           Set the packets' size, in bytes.      \n"
    "                               * Relevant only for the Transmitter.  \n"
    "                                 [Default is 1024 bytes]             \n"
    "  -t, --transmitter,                                                 \n"
    "  -r, --receiver               Set the program's role.               \n"
    "                                 [Default is Receiver]               \n"
    "  -h, --header-p=P             Probability of inputing an error in   \n"
    "                               a read frame's header.                \n"
    "                                 [Default is 0]                      \n"
    "  -f, --frame-p=P              Probability of inputing an error in   \n"
    "                               a read frame's data.                  \n"
    "                                 [Default is 0]                      \n"
    "                               * Relevant only for the Receiver.     \n"
    "      --error-byte,                                                  \n"
    "      --error-frame            Introduce the errors per-byte         \n"
    "                               or per-frame.                         \n"
    "                                 [Default is per-frame]              \n"
    "                               Introducing errors per-byte may cause \n"
    "                               corrupted messages to pass undetected,\n"
    "                               corrupting the output file(s).        \n"
    "      --no-stats,                                                    \n"
    "      --compact,                                                     \n"
    "      --stats                  Show performance statistics.          \n"
    "\n";

/**
 * Free all resources allocated to contain options by parse_args.
 */
static void clear_options() {
    free(files);
}

static void dump_options() {
    static const char* dump_string = " === Options ===\n"
        " show_help: %d            \n"
        " show_usage: %d           \n"
        " show_version: %d         \n"
        " time_retries: %d         \n"
        " answer_retries: %d       \n"
        " timeout: %d              \n"
        " baudrate: %d             \n"
        " device: %s               \n"
        " packetsize: %lu          \n"
        " my_role: %d (T=%d, R=%d) \n"
        " number_of_files: %d      \n"
        " files: 0x%08x            \n"
        " header-p: %lf            \n"
        " frame-p: %lf             \n"
        " show_statistics: %d      \n"
        "\n";

    printf(dump_string, show_help, show_usage, show_version, time_retries,
        answer_retries, timeout, baudrate, device, packetsize, my_role,
        TRANSMITTER, RECEIVER, number_of_files, files, h_error_prob,
        f_error_prob, show_statistics);

    if (files != NULL) {
        for (size_t i = 0; i < number_of_files; ++i) {
            printf(" > file#%lu: %s\n", i, files[i]);
        }
    }
}

static void exit_usage() {
    if (DUMP_OPTIONS || dump) dump_options();

    setlocale(LC_ALL, "");
    printf("%ls", usage);
    exit(EXIT_SUCCESS);
}

static void exit_version() {
    if (DUMP_OPTIONS || dump) dump_options();

    setlocale(LC_ALL, "");
    printf("%ls", version);
    exit(EXIT_SUCCESS);
}

static void exit_nofiles() {
    if (DUMP_OPTIONS || dump) dump_options();

    setlocale(LC_ALL, "");
    printf("[ARGS] Error: Expected 1 or more positionals (filenames), but got none.\n");
    printf("%ls", usage);
    exit(EXIT_SUCCESS);
}

static void exit_nonumber(int n) {
    if (DUMP_OPTIONS || dump) dump_options();

    setlocale(LC_ALL, "");
    printf("[ARGS] Error: Expected 1 positionals (number of files), but got %d.\n", n);
    printf("%ls", usage);
    exit(EXIT_SUCCESS);
}

static void exit_badpos(int n, const char* pos) {
    if (DUMP_OPTIONS || dump) dump_options();

    setlocale(LC_ALL, "");
    printf("[ARGS] Error: Bad positional #%d: %s.\n%ls", n, pos, usage);
    exit(EXIT_SUCCESS);
}

static void exit_badarg(const char* option) {
    if (DUMP_OPTIONS || dump) dump_options();

    setlocale(LC_ALL, "");
    printf("[ARGS] Error: Bad argument for option %s.\n%ls", option, usage);
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

static int parse_double(const char* str, double* outp) {
    char* endp;
    double result = strtod(str, &endp);

    if (endp == str || errno == ERANGE) {
        return 1;
    }

    *outp = result;
    return 0;
}

static int parse_ulong(const char* str, size_t* outp) {
    char* endp;
    long result = strtol(str, &endp, 10);

    if (endp == str || errno == ERANGE || result < 0) {
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
            if (parse_int(optarg, &time_retries) != 0 || time_retries <= 0) {
                exit_badarg(TIME_RETRIES_LFLAG);
            }
            break;
        case ANSWER_RETRIES_FLAG:
            if (parse_int(optarg, &answer_retries) != 0 || answer_retries <= 0) {
                exit_badarg(ANSWER_RETRIES_LFLAG);
            }
            break;
        case TIMEOUT_FLAG:
            if (parse_int(optarg, &timeout) != 0 || timeout <= 0) {
                exit_badarg(TIMEOUT_LFLAG);
            }
            break;
        case BAUDRATE_FLAG:
            if (parse_int(optarg, &baudrate) != 0 || !is_valid_baudrate(baudrate)) {
                exit_badarg(BAUDRATE_LFLAG);
            }
            break;
        case DEVICE_FLAG:
            device = optarg;
            break;
        case PACKETSIZE_FLAG:
            if (parse_ulong(optarg, &packetsize) != 0 || packetsize == 0) {
                exit_badarg(PACKETSIZE_LFLAG);
            }
            break;
        case TRANSMITTER_FLAG:
            my_role = TRANSMITTER;
            break;
        case RECEIVER_FLAG:
            my_role = RECEIVER;
            break;
        case HEADER_ERROR_P_FLAG:
            if (parse_double(optarg,&h_error_prob) != 0
              || h_error_prob < 0 || h_error_prob >= 1) {
                exit_badarg(HEADER_ERROR_P_LFLAG);
            }
            break;
        case FRAME_ERROR_P_FLAG:
            if (parse_double(optarg,&f_error_prob) != 0
              || f_error_prob < 0 || f_error_prob >= 1) {
                exit_badarg(FRAME_ERROR_P_LFLAG);
            }
            break;
        case '?':
        default:
            // getopt_long already printed an error message.
            exit_usage();
        }
    } // End Options loop

    if (show_help || show_usage) {
        exit_usage();
    }

    if (show_version) {
        exit_version();
    }

    role_string = my_role == TRANSMITTER ? "Transmitter" : "Receiver";

    // Positional arguments processing
    switch (my_role) {
    case TRANSMITTER:
        if (optind == argc) {
            exit_nofiles();
        }

        number_of_files = argc - optind;
        files = malloc(number_of_files * sizeof(char*));

        for (size_t i = 0; i < number_of_files; ++i, ++optind) {
            files[i] = argv[optind];
        }

        break;
    case RECEIVER:
        if (optind + 1 != argc) {
            exit_nonumber(argc - optind);
        }

        if (parse_ulong(argv[optind++], &number_of_files) != 0 || number_of_files == 0) {
            exit_badpos(1, argv[argc - 1]);
        }

        break;
    }

    if (DUMP_OPTIONS || dump) dump_options();
    if (dump) exit(EXIT_SUCCESS);
}
