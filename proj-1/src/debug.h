#ifndef DEBUG_H___
#define DEBUG_H___

// Call asserts
//#define NDEBUG

#include <assert.h>

// Trace LL interface behaviour (ll-interface)
#define TRACE_LL 0

// Trace LL is functions (ll-frames)
#define TRACE_LL_IS 0

// Trace LL write functions (ll-frames)
#define TRACE_LL_WRITE 0

// Trace LL read errors (ll-core)
#define TRACE_LLERR_READ 0

// Trace LL write errors (ll-core)
#define TRACE_LLERR_WRITE 0

// Trace chars read in LL (ll-core)
#define DEEP_DEBUG 0

// Trace LL frame corruption errors (ll-core)
#define TRACE_LL_ERRORS 0

// Trace IntroduceError behaviour (ll-errors)
#define TRACE_CORRUPTION 0

// Trace APP behaviour (app-layer)
#define TRACE_APP 1

// Trace APP internals (app-layer)
#define TRACE_APP_INTERNALS 0

// Trace FILE behaviour (fileio)
#define TRACE_FILE 1

// Print strings to stdout
#define TEXT_DEBUG 0

// Trace Setup (signals, options, ll-setup)
#define TRACE_SETUP 1

// Trace Signals (signals)
#define TRACE_SIG 1

// Trace Timing (signals)
#define TRACE_TIME 0

// Dump Options (options)
#define DUMP_OPTIONS 0

#endif // DEBUG_H___
