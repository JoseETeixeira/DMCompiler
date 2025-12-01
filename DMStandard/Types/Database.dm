// database type definition
/database
	parent_type = /datum
	proc/Close()
	proc/Error()
	proc/ErrorMsg()
	New(filename)
		if (filename)
			Open(filename)
	proc/Open(filename)

/database/query
	var/_binobj as opendream_unimplemented
	proc/Add(text, ...)
	proc/Clear()
	Close()
	proc/Columns(column)
	Error()
	ErrorMsg()
	proc/Execute(database)
	proc/GetColumn(column)
	proc/GetRowData()
	New(Query, Cursor)
	proc/NextRow()
	proc/RowsAffected()

	proc/Reset()

