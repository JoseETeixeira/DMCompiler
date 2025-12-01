// Test comparison

#if 8 == 8
var/comparison_test = 1
#endif

#if (8 & 12) == 8
var/bitwise_with_parens = 1
#endif

#if 8 == 8 & 12
var/without_parens = 1
#endif

/obj/test
	var/done = 1
