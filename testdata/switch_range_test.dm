/proc/test_switch_range()
	var/x = 5
	switch(x)
		if(1 to 3)
			return "low"
		if(4 to 6)
			return "mid"
		if(7 to 10)
			return "high"
		else
			return "other"
