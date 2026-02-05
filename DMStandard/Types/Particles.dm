// particles type definition - particle system for visual effects
// See: https://www.byond.com/docs/ref/#/{notes}/particles
/particles
	parent_type = /datum

	// Particle vars that affect the entire set (generators are not allowed for these)
	var/width = 100   // Size of the particle image width
	var/height = 100  // Size of the particle image height
	var/count = 100   // Maximum number of particles
	var/spawning = 1  // Number of particles to spawn per tick
	var/bound1 = -1000  // Lower bound for particle positions (list or number)
	var/bound2 = 1000   // Upper bound for particle positions (list or number)
	var/gravity         // Gravity vector (list or number)
	var/list/gradient = null  // Color gradient for particles
	var/transform       // Transformation matrix

	// Vars that apply when a particle spawns
	var/lifespan        // Time particle exists (in ticks)
	var/fade            // Fade out time
	var/fadein          // Fade in time
	var/icon            // Icon or weighted list of icons
	var/icon_state      // Icon state or weighted list of states
	var/color           // Initial color
	var/color_change    // Color change per tick
	var/position        // Initial position (list or number)
	var/velocity        // Initial velocity (list or number)
	var/scale           // Initial scale (list or number)
	var/grow            // Scale change per tick (list or number)
	var/rotation        // Initial rotation
	var/spin            // Rotation change per tick
	var/friction        // Velocity friction per tick

	// Vars that are evaluated every tick
	var/drift           // Random drift per tick (list or number)
