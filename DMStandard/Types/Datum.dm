// datum type definition - base type for all DM objects
/datum
	var/tmp/type
	var/tmp/parent_type
	var/tmp/list/vars
	var/tag

	proc/New()

	proc/Del()

	proc/Topic(href, href_list)

	proc/Read(savefile/F)

	proc/Write(savefile/F)
