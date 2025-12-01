/obj/test
	var/health as num
	var/name as text
	var/flags as num|text
	var/x = 5
	var/y as num = 10
	
	proc/test_proc()
		var/local_num as num
		var/local_text as text
		var/local_mixed as num|text
		var/local_no_type = 42
		return local_num + local_no_type
