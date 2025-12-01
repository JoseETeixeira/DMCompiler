// Test bitwise with comparison

#if (8 & 12) == 8
var/bitwise_and_compare = 1
#endif

#if 8 & 12
var/bitwise_and_no_compare = 1
#endif

/obj/test
	var/done = 1
