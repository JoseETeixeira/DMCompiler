/proc/test_input()
    // Test basic input
    var/result1 = input("Enter text")
    
    // Test input with type
    var/result2 = input("Enter number") as num
    
    // Test input with multiple types
    var/result3 = input("Enter value") as num|text
    
    // Test input with list
    var/list/options = list("A", "B", "C")
    var/result4 = input("Choose") in options
    
    // Test input with type and list
    var/result5 = input("Choose object") as obj in options
    
    // Test input with all parameters
    var/result6 = input(usr, "Message", "Title", "Default") as text
    
    return TRUE
