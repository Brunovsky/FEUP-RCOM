#ifndef DEBUG_H___
#define DEBUG_H___

// Call asserts
//#define NDEBUG

#include <assert.h>

// Trace LL interface behaviour
#define TRACE_LL 1

// Trace LL read errors
#define TRACE_LL_READ 0

// Trace LL write errors
#define TRACE_LL_WRITE 0

// Trace chars read in LL
#define DEEP_DEBUG 0

// Trace LL frame corruption errors
#define TRACE_LL_ERRORS 0

// Trace IntroduceError behaviour
#define TRACE_CORRUPTION 0

// Trace APP behaviour
#define TRACE_APP 1

// Trace APP internals
#define TRACE_APP_INTERNALS 0

// Trace FILE behaviour
#define TRACE_FILE 1

// Print strings to stdout
#define TEXT_DEBUG 0

// Trace Setup
#define TRACE_SETUP 1

// Trace Signals
#define TRACE_SIG 1

// Trace Timing
#define TRACE_TIME 0

// Dump Options
#define DUMP_OPTIONS 0

#endif // DEBUG_H___
