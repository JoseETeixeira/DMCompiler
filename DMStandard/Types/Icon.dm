// Icon type definition - runtime icon manipulation
/icon
	parent_type = /datum
	var/icon

	New(icon, icon_state, dir, frame, moving)

	proc/Blend(icon, function = ICON_ADD, x = 1, y = 1)

	proc/Crop(x1, y1, x2, y2)

	proc/DrawBox(rgb, x1, y1, x2 = x1, y2 = y1)

	proc/Flip(dir)

	proc/GetPixel(x, y, icon_state, dir = 0, frame = 0, moving = -1)

	proc/Height()

	proc/IconStates(mode = 0)
		return icon_states(src, mode)

	proc/Insert(new_icon, icon_state, dir, frame, moving, delay)

	proc/MapColors(...)

	proc/Scale(width, height)

	proc/SetIntensity(r, g = r, b = r)

	proc/Shift(dir, offset, wrap = 0)

	proc/SwapColor(old_rgb, new_rgb)

	proc/Turn(angle)

	proc/Width()

proc/icon(icon, icon_state, dir, frame, moving)
	return new /icon(icon, icon_state, dir, frame, moving)
