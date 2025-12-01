/obj/edge_test
	// Test various type combinations
	var/single_type as num
	var/two_types as num|text
	var/three_types as num|text|obj
	var/with_spaces as num | text
	var/uppercase_TYPE as NUM
	var/mixed_case as Num|Text
	
	// Test with initialization
	var/typed_init as num = 100
	var/multi_typed_init as num|text = "hello"
	
	proc/test_local_vars()
		var/local1 as num
		var/local2 as text|num
		var/local3 as mob|obj|turf
		return 0
