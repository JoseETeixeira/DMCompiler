// Objects file
/obj
	var/health = MAX_HEALTH
	
	proc/TakeDamage(amount)
		health -= amount
