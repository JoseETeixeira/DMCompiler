// Test file for parser error recovery
// This file contains intentionally malformed syntax to test error recovery

/obj/test
    name = "test object"

/proc/test_good()
    return 1

/proc/test_bad_expr()
    var/x = 1 + 
    // Missing operand - should recover
    var/y = 2
    return y

/proc/test_unbalanced()
    var/z = ((1 + 2)
    // Missing closing paren - should recover
    return 3

/proc/test_after_error()
    // This proc should still be parsed after errors above
    return 42

/obj/widget
    var/count = 0
    
/obj/widget/proc/increment()
    count++
    return count

/proc/test_missing_semicolon()
    var/a = 1 var/b = 2
    // Missing separator - should recover
    return a + b

/proc/final_test()
    // Final proc to ensure parser continued
    return "success"
