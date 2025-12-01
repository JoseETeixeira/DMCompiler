// Debug test for bitwise with parentheses

#if 8
var/test_8 = 1
#endif

#if (8)
var/test_paren_8 = 1
#endif

#if 8 == 8
var/test_8_eq_8 = 1
#endif

#if (8) == 8
var/test_paren_8_eq_8 = 1
#endif

#if 8 & 12
var/test_8_and_12 = 1
#endif

#if (8 & 12)
var/test_paren_8_and_12 = 1
#endif

/obj/test
	var/done = 1
