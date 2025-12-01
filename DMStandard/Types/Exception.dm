// exception type definition
/exception
	parent_type = /datum

	var/name
	var/desc
	var/file
	var/line

	New(name, file, line)
		src.name = name
		src.file = file
		src.line = line

proc/EXCEPTION(message)
	var/exception/E = new(message)
	E.desc = message
	return E


