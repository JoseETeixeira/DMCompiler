// Test file for preprocessor directive handlers

// Test simple macro
#define VERSION 1
#define AUTHOR "TestUser"

// Test function-like macro
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Test conditional compilation
#ifdef VERSION
var/version_defined = VERSION
#endif

#ifndef UNDEFINED_MACRO
var/not_defined = 1
#endif

// Test #if with expressions
#define DEBUG_MODE 1

#if DEBUG_MODE
var/debug_enabled = 1
#else
var/debug_enabled = 0
#endif

// Test nested conditionals
#define FEATURE_A 1
#define FEATURE_B 0

#ifdef FEATURE_A
    #if FEATURE_B
        var/both_features = 1
    #else
        var/only_feature_a = 1
    #endif
#endif

// Test #elif
#define BUILD_TYPE 2

#if BUILD_TYPE == 1
var/build_type = "development"
#elif BUILD_TYPE == 2
var/build_type = "testing"
#else
var/build_type = "production"
#endif

// Test #undef
#define TEMP_MACRO 100
var/before_undef = TEMP_MACRO
#undef TEMP_MACRO
// TEMP_MACRO should no longer be defined here

// Test macro expansion
var/max_value = MAX(10, 20)
var/min_value = MIN(5, 3)
var/author_name = AUTHOR
