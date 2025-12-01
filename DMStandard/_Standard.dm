//These procs should be in alphabetical order, as in DreamProcNativeRoot.cs
proc/abs(A) as num
proc/addtext(...) as text
proc/alert(Usr = usr, Message, Title, Button1 = "Ok", Button2, Button3) as text
proc/animate(Object, time, loop, easing, flags, delay, pixel_x, pixel_y, pixel_z, maptext, maptext_width, maptext_height, maptext_x, maptext_y, dir, alpha, transform, color, luminosity, infra_luminosity, layer, glide_size, icon, icon_state, invisibility, suffix) as null
proc/arglist(args) as /list
proc/arccos(X) as num
proc/arcsin(X) as num
proc/arctan(X, Y) as num
proc/ascii2text(N) as text
proc/ASSERT(condition)
proc/block(Start, End, StartX, StartY, StartZ, EndX=StartX, EndY=StartY, EndZ=StartZ) as /list
proc/bound_pixloc(Atom, Dir)
proc/bounds(Ref=src, Dist=0, x_offset, y_offset, extra_width=0, extra_height=0, x, y, width, height, z) as /list
proc/bounds_dist(Ref, Target) as num
proc/browse(Body, Options)
proc/browse_rsc(File, FileName)
proc/call(proc, ...)
proc/call_ext(LibName, FuncName)
proc/ceil(A) as num
proc/ckey(Key) as text|null
proc/ckeyEx(Text) as text|null
proc/clamp(Value, Low, High) as /list|num|null
proc/cmptext(...) as num
proc/cmptextEx(...) as num
proc/copytext(T, Start = 1, End = 0) as text|null
proc/copytext_char(T,Start=1,End=0) as text|null
proc/cos(X) as num
proc/CRASH(msg) as null
proc/del(Object)
proc/fcopy(Src, Dst) as num
proc/fcopy_rsc(File) as num|null
proc/fdel(File) as num
proc/EXCEPTION(message)
proc/fexists(File) as num
proc/file(Path)
proc/file2text(File) as text|null
proc/filter(type, ...)
proc/findtext(Haystack, Needle, Start = 1, End = 0) as num
proc/findtextEx(Haystack, Needle, Start = 1, End = 0) as num
proc/findlasttext(Haystack, Needle, Start = 0, End = 1) as num
proc/findlasttextEx(Haystack, Needle, Start = 0, End = 1) as num
proc/flick(Icon, Object)
proc/flist(Path) as /list
proc/floor(A) as num
proc/fract(n) as num
proc/ftp(File, Name)
proc/ftime(File, IsCreationTime = 0) as num
proc/generator(type, A, B, rand) as /generator
proc/get_dir(Loc1, Loc2) as num
proc/get_dist(Loc1, Loc2) as num
proc/get_step(Ref, Dir)
proc/get_step_away(Ref, Trg, Max=5)
proc/get_step_rand(Ref)
proc/get_step_to(Ref, Trg, Min=0)
proc/get_step_towards(Ref, Trg)
proc/get_steps_to(Ref, Trg, Min=0) as /list
proc/gradient(A, index)
proc/hascall(Object, ProcName) as num
proc/hearers(Depth = world.view, Center = usr) as /list
proc/html_decode(HtmlText) as text
proc/html_encode(PlainText) as text
proc/icon(icon, icon_state, dir, frame, moving)
proc/icon_states(Icon, mode = 0) as /list
proc/image(icon, loc, icon_state, layer, dir, pixel_x, pixel_y)
proc/initial(Var)
proc/input(Usr = usr, Message, Title, Default, Type)
proc/isarea(...) as num
proc/isfile(File) as num
proc/isicon(Icon) as num
proc/isinf(n) as num
proc/islist(Object) as num
proc/isloc(...) as num
proc/ismob(...) as num
proc/isobj(...) as num
proc/ismovable(...) as num
proc/isnan(n) as num
proc/isnull(Val) as num
proc/isnum(Val) as num
proc/ispath(Val, Type) as num
proc/ispointer(Value) as num
proc/issaved(Var) as num
proc/istext(Val) as num
proc/isturf(...) as num
proc/istype(Val, Type) as num
proc/json_decode(JSON)
proc/json_encode(Value, flags)
proc/length(E) as num
proc/length_char(E) as num
proc/lerp(A, B, factor)
proc/link(url)
proc/locate(Type, Container)
proc/list2params(List) as text
proc/load_ext(LibName, FuncName)
proc/load_resource(...)
proc/log(X, Base) as num
proc/lowertext(T) as text
proc/matrix(a, b, c, d, e, f)
proc/max(A) as num|text|null
proc/md5(T) as text|null
proc/min(A) as num|text|null
proc/nameof(x) as text
proc/noise_hash(...) as num
	set opendream_unimplemented = TRUE
	return 0.5
proc/newlist(...) as /list
proc/nonspantext(Haystack, Needles, Start = 1) as num
proc/num2text(N, A, B) as text
proc/obounds(Ref, Dist=0, x_offset, y_offset, extra_width=0, extra_height=0) as /list
proc/orange(Dist = 5, Center = usr) as /list
proc/oview(Dist = 5, Center = usr) as /list
proc/oviewers(Depth = world.view, Center = usr) as /list
proc/ohearers(Depth = world.view, Center = usr) as /list
proc/output(Msg, Control)
proc/params2list(Params) as /list
proc/pick(...)
proc/pixloc(x, y, z)
proc/prob(P) as num
proc/rand(L, H) as num
proc/rand_seed(Seed) as null
proc/range(Dist = 5, Center = usr) as /list
proc/ref(Object) as text
proc/regex(pattern, flags)
proc/REGEX_QUOTE(text) as text
proc/replacetext(Haystack, Needle, Replacement, Start = 1, End = 0) as text|null
proc/replacetextEx(Haystack, Needle, Replacement, Start = 1, End = 0) as text|null
proc/rgb(R, G, B, A, space) as text|null
proc/rgb2num(color, space = COLORSPACE_RGB) as /list
proc/roll(ndice = 1, sides) as num
proc/round(A, B) as num
proc/run(file)
proc/sha1(input) as text|null
proc/shell(command)
proc/shutdown(Addr,Natural = 0)
proc/sign(A) as num
proc/sin(X) as num
proc/sleep(Delay)
proc/sorttext(...) as num
proc/spawn(Delay)
proc/sorttextEx(...) as num
proc/sound(file, repeat = 0, wait, channel, volume)
proc/spantext(Haystack,Needles,Start=1) as num
proc/spantext_char(Haystack,Needles,Start=1) as num
proc/splicetext(Text, Start = 1, End = 0, Insert = "") as text|null
proc/splicetext_char(Text, Start = 1, End = 0, Insert = "") as text|null
proc/splittext(Text, Delimiter) as /list
proc/sqrt(A) as num
proc/stat(Name, Value)
proc/statpanel(Panel, Name, Value)
proc/startup(Port, Addr)
proc/step(Ref, Dir, Speed=0) as num
proc/step_away(Ref, Trg, Max=5, Speed=0) as num
proc/step_rand(Ref, Speed=0) as num
proc/step_to(Ref, Trg, Min=0, Speed=0) as num
proc/step_towards(Ref, Trg, Speed=0) as num
proc/tan(X) as num
proc/text2ascii(T, pos = 1) as num
proc/text2ascii_char(T, pos = 1) as num
proc/text2file(Text, File)
proc/text2num(T, radix = 10) as num|null
proc/text2path(T)
proc/time2text(timestamp, format) as text
proc/trimtext(Text) as text
proc/trunc(n) as num
proc/turn(Dir, Angle)
proc/typesof(Item1) as /list
proc/uppertext(T) as text
proc/url_decode(UrlText) as text
proc/url_encode(PlainText, format = 0) as text
proc/vector(x, y, z)
proc/values_cut_over(Alist, Max, inclusive = 0) as num
proc/values_cut_under(Alist, Min, inclusive = 0) as num
proc/values_dot(A, B) as num
proc/values_product(Alist) as num
proc/values_sum(Alist) as num
proc/view(Dist = 5, Center = usr) as /list
proc/viewers(Depth = world.view, Center = usr) as /list
proc/walk(Ref, Dir, Lag = 0, Speed = 0)
proc/walk_away(Ref, Trg, Max=5, Lag=0, Speed=0)
proc/walk_rand(Ref, Lag = 0, Speed = 0)
proc/walk_to(Ref, Trg, Min = 0, Lag = 0, Speed = 0)
proc/walk_towards(Ref, Trg, Lag=0, Speed=0)
proc/winclone(player, window_name, clone_name)
proc/winexists(player, control_id) as num
proc/winget(player, control_id, params) as text
proc/winset(player, control_id, params)
proc/set_background(mode)

#include "Defines.dm"
#include "_Globals.dm"
#include "Types/AList.dm"
#include "Types/Callee.dm"
#include "Types/Client.dm"
#include "Types/Database.dm"
#include "Types/Datum.dm"
#include "Types/Exception.dm"
#include "Types/Filter.dm"
#include "Types/Generator.dm"
#include "Types/Icon.dm"
#include "Types/Image.dm"
#include "Types/List.dm"
#include "Types/Matrix.dm"
#include "Types/Mutable_Appearance.dm"
#include "Types/Particles.dm"
#include "Types/PixLoc.dm"
#include "Types/Regex.dm"
#include "Types/Savefile.dm"
#include "Types/Sound.dm"
#include "Types/Vector.dm"
#include "Types/World.dm"
#include "Types/Atoms/_Atom.dm"
#include "Types/Atoms/Area.dm"
#include "Types/Atoms/Mob.dm"
#include "Types/Atoms/Movable.dm"
#include "Types/Atoms/Obj.dm"
#include "Types/Atoms/Turf.dm"
#include "UnsortedAdditions.dm"

proc/replacetextEx_char(Haystack, Needle, Replacement, Start = 1, End = 0) as text
	set opendream_unimplemented = TRUE
	return Haystack

// Spatial/Movement proc implementations

proc/get_dir(Loc1, Loc2) as num
	set opendream_unimplemented = TRUE
	return 0

proc/get_dist(Loc1, Loc2) as num
	if (isnull(Loc1) || isnull(Loc2)) return 127
	if (Loc1 == Loc2) return -1

	var/distX = Loc2.x - Loc1.x
	var/distY = Loc2.y - Loc1.y
	return round(sqrt(distX ** 2 + distY ** 2))

proc/get_step(Ref, Dir)
	set opendream_unimplemented = TRUE
	return null

proc/get_step_away(Ref, Trg, Max = 5)
	var/dir = turn(get_dir(Ref, Trg), 180)
	return get_step(Ref, dir)

proc/get_step_rand(Ref)
	// BYOND's implementation seems to be heavily weighted in favor of Ref's dir.
	var/dir = pick(NORTH, SOUTH, EAST, WEST, NORTHEAST, SOUTHEAST, SOUTHWEST, NORTHWEST)
	return get_step(Ref, dir)

proc/get_step_towards(Ref, Trg)
	var/dir = get_dir(Ref, Trg)
	return get_step(Ref, dir)

proc/step(Ref, Dir, Speed=0) as num
	//TODO: Speed = step_size if Speed is 0
	return Ref.Move(get_step(Ref, Dir), Dir)

proc/step_away(Ref, Trg, Max=5, Speed=0) as num
	return Ref.Move(get_step_away(Ref, Trg, Max), turn(get_dir(Ref, Trg), 180))

proc/step_rand(Ref, Speed=0) as num
	var/target = get_step_rand(Ref)
	return Ref.Move(target, get_dir(Ref, target))

proc/step_to(Ref, Trg, Min = 0, Speed = 0) as num
	//TODO: Consider obstacles
	var/dist = get_dist(Ref, Trg)
	if (dist <= Min) return 0
	var/step_dir = get_dir(Ref, Trg)
	return step(Ref, step_dir, Speed)

proc/step_towards(Ref, Trg, Speed=0) as num
	return Ref.Move(get_step_towards(Ref, Trg), get_dir(Ref, Trg))

/proc/walk_away(Ref, Trg, Max=5, Lag=0, Speed=0)
	set opendream_unimplemented = TRUE
	CRASH("/walk_away() is not implemented")

proc/jointext(List, Glue, Start = 1, End = 0) as text
	if(islist(List))
		return List.Join(Glue, Start, End)
	if(istext(List))
		return List
	CRASH("jointext was passed a non-list, non-text value")

proc/lentext(T) as num
	return length(T)

proc/winshow(player, window, show=1)
	winset(player, window, "is-visible=[show ? "true" : "false"]")

proc/winclone(player, window_name, clone_name)
	set opendream_unimplemented = TRUE

proc/winexists(player, control_id) as num
	set opendream_unimplemented = TRUE
	return 0

proc/winget(player, control_id, params) as text
	set opendream_unimplemented = TRUE
	return ""

proc/winset(player, control_id, params)
	set opendream_unimplemented = TRUE

proc/set_background(mode)
	set opendream_unimplemented = TRUE

proc/refcount(var/Object) as num
	// woah that's a lot of refs
	// i wonder if it's true??
	return 100
	// (it's not)
