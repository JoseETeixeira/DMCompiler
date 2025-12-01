// pixloc type definition
/pixloc
	var/turf/loc as opendream_unimplemented
	var/step_x as opendream_unimplemented
	var/step_y as opendream_unimplemented
	var/x as opendream_unimplemented
	var/y as opendream_unimplemented
	var/z as opendream_unimplemented

	proc/New(x, y, z)

proc/pixloc(x, y, z)
	return new /pixloc(x, y, z)
