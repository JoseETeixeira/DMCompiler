// atom movable type definition
/atom/movable
	var/screen_loc

	var/animate_movement = FORWARD_STEPS as opendream_unimplemented
	var/list/locs = null as opendream_unimplemented
	var/glide_size = 0
	var/step_size as opendream_unimplemented
	var/tmp/bound_x as opendream_unimplemented
	var/tmp/bound_y as opendream_unimplemented
	var/tmp/bound_width as opendream_unimplemented
	var/tmp/bound_height as opendream_unimplemented

	//Undocumented var. "[x],[y]" or "[x],[y] to [x2],[y2]" based on bound_* vars
	var/bounds as opendream_unimplemented

	var/particles/particle_system 

	proc/Bump(atom/Obstacle)

	proc/Move(atom/NewLoc, Dir=0) as num
		if (isnull(NewLoc) || src.loc == NewLoc)
			return FALSE

		if (Dir != 0)
			src.dir = Dir

		if(!isnull(src.loc))
			if (!src.loc.Exit(src, NewLoc))
				return FALSE
			// Ensure the atoms on the turf also permit this exit
			for (var/atom/movable/exiting in src.loc)
				if (!exiting.Uncross(src))
					return FALSE

		if (NewLoc.Enter(src, src.loc))
			var/atom/oldloc = src.loc
			var/area/oldarea = oldloc?.loc
			var/area/newarea = NewLoc.loc
			src.loc = NewLoc

			// First, call Exited() on the old area
			if (newarea != oldarea)
				oldarea?.Exited(src, src.loc)

			// Second, call Exited() on the old turf and Uncrossed() on its contents
			oldloc?.Exited(src, src.loc)
			for (var/atom/movable/uncrossed in oldloc)
				uncrossed.Uncrossed(src)

			// Third, call Entered() on the new turf and Crossed() on its contents
			src.loc.Entered(src, oldloc)
			for (var/atom/movable/crossed in src.loc)
				crossed.Crossed(src)

			// Fourth, call Entered() on the new area
			if (newarea != oldarea)
				newarea.Entered(src, oldloc)

			return TRUE
		else
			return FALSE

	//proc/New()