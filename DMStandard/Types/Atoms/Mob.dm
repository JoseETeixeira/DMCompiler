// mob type definition - player-controlled or AI-controlled entities
/mob
	parent_type = /atom/movable

	var/client/client
	var/key as text|null
	var/tmp/ckey as text|null

	var/tmp/list/group

	var/see_invisible = 0
	var/see_infrared = 0
	var/sight = 0
	var/see_in_dark = 2

	density = TRUE
	layer = MOB_LAYER

	proc/Login()

	proc/Logout()
