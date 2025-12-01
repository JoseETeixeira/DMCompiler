// vector type definition
/vector
	var/len = null as num|null
	var/size = null as num|null
	var/x = 0 as num
	var/y = 0 as num
	var/z = 0 as num

	proc/New(x, y, z)
	
	proc/Cross(vector/B)
		return new /vector(y * B.z - z * B.y, z * B.x - x * B.z, x * B.y - y * B.x)
	
	proc/Dot(vector/B)
		return x * B.x + y * B.y + z * B.z
	
	proc/Interpolate(vector/B, t)
		return src + (B-src) * t
	
	proc/Normalize()
		src.size = 1
		return src
	
	proc/Turn(angle)
		var/s = sin(angle)
		var/c = cos(angle)
		return new /vector(x * c - y * s, x * s + y * c, z)

/proc/vector(x, y, z)
	return new /vector(x, y, z)

