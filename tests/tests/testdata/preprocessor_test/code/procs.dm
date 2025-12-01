// Procedures file
#ifdef DEBUG
	proc/DebugPrint(msg)
		world << "DEBUG: [msg]"
#endif

proc/Hello()
	world << "Hello from DM!"
