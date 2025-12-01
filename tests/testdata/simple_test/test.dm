// Simple DM test file

/mob
    var/health = 100
    var/name = "Unknown"
    
    proc/take_damage(amount)
        health -= amount
        if(health <= 0)
            die()
    
    proc/die()
        world << "[name] has died!"
        del(src)

/mob/player
    name = "Player"
    
    New()
        ..()
        world << "Player created with [health] health"
    
    proc/heal(amount)
        health += amount
        if(health > 100)
            health = 100
        world << "[name] healed to [health] health"

/obj/item
    var/name = "item"
    var/weight = 1.0
    
    proc/pickup(mob/M)
        M << "You picked up [name]"

/obj/item/weapon
    name = "sword"
    weight = 5.0
    var/damage = 10

proc/global_test()
    var/x = 10
    var/y = 20
    return x + y
