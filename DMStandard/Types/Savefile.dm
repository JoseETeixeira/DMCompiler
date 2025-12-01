// savefile type definition
/savefile
	var/byond_build
	var/byond_version
	var/cd
	var/list/dir
	var/eof
	var/name

	proc/New(filename, timeout)
	proc/Flush()
	proc/ExportText(path = cd, file)

	proc/ImportText(path = cd, source)
		set opendream_unimplemented = TRUE

	proc/Lock(timeout)
		set opendream_unimplemented = TRUE

	proc/Unlock()
		set opendream_unimplemented = TRUE

