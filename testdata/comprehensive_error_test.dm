// Comprehensive error handling test
// This file intentionally contains various errors to test error reporting

/obj/test_object
    var/field_var = 10

/obj/test_object/proc/test_all_errors()
    // Test 1: Unknown identifier (not local, field, or global)
    var/result = completely_unknown_identifier
    
    // Test 2: Unknown proc call
    unknown_function_call()
    
    // Test 3: Type path not found
    var/bad_type = /obj/this/type/does/not/exist
    
    // Test 4: Built-in function argument errors
    var/a = pick()  // No arguments
    var/b = locate()  // No arguments (needs 1-3)
    var/c = prob()  // No arguments (needs 1)
    var/d = istype()  // No arguments (needs 2)
    var/e = rgb(1, 2)  // Too few arguments (needs 3-5)
    var/f = input(1, 2, 3, 4, 5)  // Too many arguments (max 4)
    
    // Test 5: Control flow errors
    break  // Outside loop
    continue  // Outside loop
    
    // Test 6: Duplicate variable
    var/x = 1
    var/x = 2
    
    return result

/proc/test_assignment_error()
    // Test 7: Assignment to unknown variable
    nonexistent_variable = 42
    return 1
