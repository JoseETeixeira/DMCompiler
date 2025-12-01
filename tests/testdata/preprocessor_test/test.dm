// Test preprocessor expression evaluation

#define VALUE_10 10
#define VALUE_5 5
#define ENABLED
#undef DISABLED

// Test arithmetic expressions
#if 2 + 3 == 5
var/arithmetic_add = 1
#endif

#if 10 - 3 == 7
var/arithmetic_sub = 1
#endif

#if 4 * 5 == 20
var/arithmetic_mul = 1
#endif

#if 20 / 4 == 5
var/arithmetic_div = 1
#endif

#if 17 % 5 == 2
var/arithmetic_mod = 1
#endif

// Test comparison operators
#if 5 < 10
var/comparison_lt = 1
#endif

#if 10 > 5
var/comparison_gt = 1
#endif

#if 5 <= 5
var/comparison_lte = 1
#endif

#if 10 >= 10
var/comparison_gte = 1
#endif

#if 5 == 5
var/comparison_eq = 1
#endif

#if 5 != 10
var/comparison_neq = 1
#endif

// Test logical operators
#if 1 && 1
var/logical_and_true = 1
#endif

#if 0 && 1
var/logical_and_false_should_not_appear = 1
#endif

#if 1 || 0
var/logical_or_true = 1
#endif

#if !0
var/logical_not = 1
#endif

// Test bitwise operators
#if (8 & 12) == 8
var/bitwise_and = 1
#endif

#if (8 | 4) == 12
var/bitwise_or = 1
#endif

#if (12 ^ 8) == 4
var/bitwise_xor = 1
#endif

#if (1 << 3) == 8
var/bitwise_shift_left = 1
#endif

#if (16 >> 2) == 4
var/bitwise_shift_right = 1
#endif

// Test defined() operator
#if defined(ENABLED)
var/defined_test_true = 1
#endif

#if !defined(DISABLED)
var/defined_test_false = 1
#endif

#if defined ENABLED
var/defined_no_parens = 1
#endif

// Test macro expansion in expressions
#if VALUE_10 > VALUE_5
var/macro_comparison = 1
#endif

#if VALUE_10 + VALUE_5 == 15
var/macro_arithmetic = 1
#endif

// Test complex expressions with parentheses
#if (2 + 3) * 4 == 20
var/complex_expr1 = 1
#endif

#if 2 + (3 * 4) == 14
var/complex_expr2 = 1
#endif

// Test ternary operator
#if 1 ? 42 : 0
var/ternary_true = 1
#endif

#if 0 ? 0 : 99
var/ternary_false = 1
#endif

// Test combined conditions
#if (VALUE_10 > 5) && (VALUE_5 < 10)
var/combined_condition = 1
#endif

// Test unary minus
#if -5 + 10 == 5
var/unary_minus = 1
#endif

/obj/test
	var/done = 1
