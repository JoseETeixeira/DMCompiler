// world type definition - global game state singleton
/world
	var/list/contents = null
	var/list/vars

	var/log = null

	var/area = /area as /area
	var/turf = /turf as /turf
	var/mob = /mob as /mob

	var/name = "BYONDOT World"
	var/time
	var/timezone = 0
	var/timeofday
	var/realtime
	var/tick_lag = 1
	var/cpu = 0
	var/fps = 10
	var/tick_usage
	var/loop_checks = 0

	var/maxx = null as num|null
	var/maxy = null as num|null
	var/maxz = null as num|null
	var/icon_size = 32 as num
	var/view = 5 as text|num
	var/movement_mode = LEGACY_MOVEMENT_MODE

	var/byond_version = DM_VERSION
	var/byond_build = DM_BUILD

	var/version = 0

	var/address
	var/port = 0
	var/internet_address = "127.0.0.1"
	var/url
	var/visibility = 0
	var/status = ""
	var/process
	var/list/params = null

	var/sleep_offline = 0

	var/const/system_type

	var/map_cpu = 0
	var/hub = ""
	var/hub_password = ""
	var/reachable
	var/game_state = 0
	var/host = ""
	var/map_format = TOPDOWN_MAP
	var/cache_lifespan = 30
	var/executor = ""

	proc/New()
	proc/Del()

	proc/Profile(command, type, format)
	proc/GetConfig(config_set,param)
	proc/SetConfig(config_set,param,value)
	proc/OpenPort(port)

	proc/IsSubscribed(player, type)
		return FALSE

	proc/IsBanned(key,address,computer_id,type)
		return FALSE

	proc/Error(exception)

	proc/Reboot()

	proc/Repop()

	proc/Export(Addr, File, Persist, Clients)
	proc/Import()
	proc/Topic(T,Addr,Master,Keys)

	proc/Tick()

	proc/SetScores()
		return null

	proc/GetScores()
		return null

	proc/GetMedal()
		return null

	proc/SetMedal()
		return null

	proc/ClearMedal()
		return null

	proc/AddCredits(player, credits, note)
		return 0

	proc/GetCredits(player)
		return null

	proc/PayCredits(player, credits, note)
		return 0
