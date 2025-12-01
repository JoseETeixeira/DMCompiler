// Comprehensive preprocessor macro expansion test

#define VERSION 515
#define BUILD 1630
#define MAX_PLAYERS 100
#define MIN_PLAYERS 1
#define DEFAULT_PORT 5000

#define ENABLED
#undef DISABLED

// Test macro expansion in arithmetic
#if VERSION + BUILD == 2145
var/macro_add = 1
#endif

#if VERSION - 500 == 15
var/macro_sub = 1
#endif

#if BUILD / 10 == 163
var/macro_div = 1
#endif

#if BUILD % 1000 == 630
var/macro_mod = 1
#endif

// Test macro expansion in comparisons
#if MAX_PLAYERS > MIN_PLAYERS
var/macro_comparison_gt = 1
#endif

#if MIN_PLAYERS < MAX_PLAYERS
var/macro_comparison_lt = 1
#endif

#if VERSION >= 515
var/macro_comparison_gte = 1
#endif

#if MIN_PLAYERS <= MAX_PLAYERS
var/macro_comparison_lte = 1
#endif

// Test macro expansion in logical operations
#if MAX_PLAYERS && MIN_PLAYERS
var/macro_logical_and = 1
#endif

#if VERSION || BUILD
var/macro_logical_or = 1
#endif

// Test macro expansion in bitwise operations
#if (VERSION & 512) == 512
var/macro_bitwise_and = 1
#endif

#if (DEFAULT_PORT | 15) == 5007
var/macro_bitwise_or = 1
#endif

// Test macro expansion with defined()
#if defined(ENABLED) && VERSION > 500
var/macro_defined_and_comparison = 1
#endif

#if !defined(DISABLED) || BUILD < 1000
var/macro_not_defined_or_comparison = 1
#endif

// Test complex macro expression
#if (MAX_PLAYERS - MIN_PLAYERS) * 5 == 495
var/macro_complex = 1
#endif

// Test nested macro references
#define TOTAL_RANGE MAX_PLAYERS

#if TOTAL_RANGE == 100
var/nested_macro = 1
#endif

// Test macro in ternary
#if VERSION > 500 ? 1 : 0
var/macro_ternary = 1
#endif

/obj/test
	var/done = 1
