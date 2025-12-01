// Test file for preprocessor error handling

#if  // Missing expression
    var/x = 1
#endif

#ifdef  // Missing identifier
    var/y = 2
#endif

#ifndef  // Missing identifier
    var/z = 3
#endif

#elif  // Without matching #if
    var/a = 4

#else  // Without matching #if
    var/b = 5

#endif  // Without matching #if

#undef  // Missing identifier

/proc/test()
    return 1
