// Test file for improved error handling

/proc/test_unknown_identifier()
    var/x = unknown_var  // Should report: not found as local, field, or global
    return x

/proc/test_unknown_proc()
    unknown_proc()  // Should report: not found as global proc or member proc
    return 1

/proc/test_type_not_found()
    var/obj/nonexistent/item/x = new /obj/nonexistent/item()  // Should report type not found with path
    return x

/proc/test_builtin_arg_count()
    var/a = pick()  // Should report: requires at least 1 argument
    var/b = locate()  // Should report: requires 1, 2, or 3 arguments
    var/c = prob()  // Should report: requires exactly 1 argument
    var/d = istype()  // Should report: requires exactly 2 arguments
    return a + b + c + d

/proc/test_break_outside_loop()
    break  // Should report: break statement outside of loop
    return 1

/proc/test_continue_outside_loop()
    continue  // Should report: continue statement outside of loop
    return 1

/proc/test_duplicate_var()
    var/x = 1
    var/x = 2  // Should report: variable already declared
    return x
