// pixloc type definition
/pixloc
	var/turf/loc
	var/step_x
	var/step_y
	var/x
	var/y
	var/z

	proc/New(x, y, z)
		src.x = x
		src.y = y
		src.z = z

proc/pixloc(x, y, z)
	return new /pixloc(x, y, z)
