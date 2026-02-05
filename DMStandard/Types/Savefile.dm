// savefile type definition - persistent data storage
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
	proc/Lock(timeout)
	proc/Unlock()
