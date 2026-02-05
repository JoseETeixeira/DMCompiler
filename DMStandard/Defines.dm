#define TRUE 1
#define FALSE 0

#define NORTH 1
#define SOUTH 2
#define EAST 4
#define WEST 8
#define UP 16
#define DOWN 32
#define NORTHEAST 5 // NORTH | EAST
#define SOUTHEAST 6 // SOUTH | EAST
#define SOUTHWEST 10 // SOUTH | WEST
#define NORTHWEST 9 // NORTH | WEST

#define FLOAT_LAYER -1
#define AREA_LAYER 1
#define TURF_LAYER 2
#define OBJ_LAYER 3
#define MOB_LAYER 4
#define FLY_LAYER 5
#define EFFECTS_LAYER 5000
#define TOPDOWN_LAYER 10000
#define BACKGROUND_LAYER 20000

#define FLOAT_PLANE -32767

#define FEMALE "female"
#define MALE "male"
#define NEUTER "neuter"
#define PLURAL "plural"

// animate() flags arg
#define ANIMATION_END_NOW 1
#define ANIMATION_LINEAR_TRANSFORM 2
#define ANIMATION_PARALLEL 4
#define ANIMATION_RELATIVE 256

// animate() easing arg
#define LINEAR_EASING 0
#define SINE_EASING 1
#define CIRCULAR_EASING 2
#define CUBIC_EASING 3
#define BOUNCE_EASING 4
#define ELASTIC_EASING 5
#define BACK_EASING 6
#define QUAD_EASING 7
#define JUMP_EASING 7
// flags applied to the animate() easing arg
#define EASE_IN 64
#define EASE_OUT 128

#define NO_STEPS 0
#define FORWARD_STEPS 1
#define SLIDE_STEPS 2
#define SYNC_STEPS 3

// world.system_type
#define UNIX "UNIX"
#define MS_WINDOWS "MS_WINDOWS"

// Icon blending functions
#define ICON_ADD 0
#define ICON_SUBTRACT 1
#define ICON_MULTIPLY 2
#define ICON_OVERLAY 3
#define ICON_AND 4
#define ICON_OR 5
#define ICON_UNDERLAY 6

// mob.sight
#define BLIND (1<<0)
#define SEEINVIS (1<<1)
#define SEE_MOBS (1<<2)
#define SEEMOBS (1<<2)
#define SEE_OBJS (1<<3)
#define SEEOBJS (1<<3)
#define SEE_TURFS (1<<4)
#define SEETURFS (1<<4)
#define SEE_SELF (1<<5)
#define SEE_INFRA (1<<6)
#define SEE_PIXELS (1<<8)
#define SEE_THRU (1<<9)
#define SEE_BLACKNESS (1<<10)

// client.perspective
#define MOB_PERSPECTIVE 0
#define EYE_PERSPECTIVE 1
#define EDGE_PERSPECTIVE 2

// regex
#define REGEX_QUOTE(a) regex((a), 1)
#define REGEX_QUOTE_REPLACEMENT(a) regex((a), 2)

#define ASSERT(expr) ((expr) ? null : CRASH("Assertion Failed: " + #expr))

// atom.blend_mode
#define BLEND_DEFAULT 0
#define BLEND_OVERLAY 1
#define BLEND_ADD 2
#define BLEND_SUBTRACT 3
#define BLEND_MULTIPLY 4
#define BLEND_INSET_OVERLAY 5

// sound.status
#define SOUND_MUTE (1<<0)
#define SOUND_PAUSED (1<<1)
#define SOUND_STREAM (1<<2)
#define SOUND_UPDATE (1<<4)

#define EXCEPTION(value) new/exception(value, __FILE__, __LINE__)

// Color spaces
#define COLORSPACE_RGB 0
#define COLORSPACE_HSV 1
#define COLORSPACE_HSL 2
#define COLORSPACE_HCY 3

// See color matrix filter: filter(type="color", ...)
#define FILTER_COLOR_RGB 0
#define FILTER_COLOR_HSV 1
#define FILTER_COLOR_HSL 2
#define FILTER_COLOR_HCY 3

// atom.appearance_flags
#define LONG_GLIDE (1<<0)
#define RESET_COLOR (1<<1)
#define RESET_ALPHA (1<<2)
#define RESET_TRANSFORM (1<<3)
#define NO_CLIENT_COLOR (1<<4)
#define KEEP_TOGETHER (1<<5)
#define KEEP_APART (1<<6)
#define PLANE_MASTER (1<<7)
#define TILE_BOUND (1<<8)
#define PIXEL_SCALE (1<<9)
#define PASS_MOUSE (1<<10)

// Matrix operations
#define MATRIX_COPY 0
#define MATRIX_MULTIPLY 1
#define MATRIX_ADD 2
#define MATRIX_SUBTRACT 3
#define MATRIX_INVERT 4
#define MATRIX_INTERPOLATE 8
#define MATRIX_ROTATE 5
#define MATRIX_SCALE 6
#define MATRIX_TRANSLATE 7
#define MATRIX_MODIFY 128

// world/Profile() arg
#define PROFILE_STOP 1
#define PROFILE_CLEAR 2
#define PROFILE_AVERAGE 4
#define PROFILE_START 0
#define PROFILE_REFRESH 0
#define PROFILE_RESTART PROFILE_CLEAR

// filter(type="alpha", ...) flags arg
#define MASK_INVERSE (1<<0)
#define MASK_SWAP (1<<1)

// filter(type="layer", ...) flags arg
#define FILTER_OVERLAY 0
#define FILTER_UNDERLAY 1

// filter(type="outline", ...) flags arg
#define OUTLINE_SHARP (1<<0)
#define OUTLINE_SQUARE (1<<1)

// filter(type="wave", ...) flags arg
#define WAVE_SIDEWAYS (1<<0)
#define WAVE_BOUNDED (1<<1)

// mouse handling
#define MOUSE_INACTIVE_POINTER 0
#define MOUSE_ACTIVE_POINTER 1
#define MOUSE_DRAG_POINTER 3
#define MOUSE_DROP_POINTER 4
#define MOUSE_ARROW_POINTER 5
#define MOUSE_CROSSHAIRS_POINTER 6
#define MOUSE_HAND_POINTER 7

// mouse buttons and keys
#define MOUSE_LEFT_BUTTON 1
#define MOUSE_RIGHT_BUTTON 2
#define MOUSE_MIDDLE_BUTTON 4
#define MOUSE_CTRL_KEY 8
#define MOUSE_SHIFT_KEY 16
#define MOUSE_ALT_KEY 32

// client.control_freak
#define CONTROL_FREAK_ALL (1<<0)
#define CONTROL_FREAK_SKIN (1<<1)
#define CONTROL_FREAK_MACROS (1<<2)

// Database operations
#define DATABASE_OPEN 0
#define DATABASE_CLOSE 1
#define DATABASE_ERROR_CODE 2
#define DATABASE_ERROR 3
#define DATABASE_QUERY_CLEAR 4
#define DATABASE_QUERY_ADD 5
#define DATABASE_QUERY_EXEC 8
#define DATABASE_QUERY_NEXT 9
#define DATABASE_QUERY_ABORT 10
#define DATABASE_QUERY_RESET 11
#define DATABASE_QUERY_ROWS_AFFECTED 12
#define DATABASE_ROW_COLUMN_NAMES 16
#define DATABASE_ROW_COLUMN_VALUE 17
#define DATABASE_ROW_LIST 18

// atom.vis_flags
#define VIS_INHERIT_ICON 1
#define VIS_INHERIT_ICON_STATE 2
#define VIS_INHERIT_DIR 4
#define VIS_INHERIT_LAYER 8
#define VIS_INHERIT_PLANE 16
#define VIS_INHERIT_ID 32
#define VIS_UNDERLAY 64
#define VIS_HIDE 128

// world.map_format
#define TOPDOWN_MAP 0
#define ISOMETRIC_MAP 1
#define SIDE_MAP 2
#define TILED_ICON_MAP 32768

// world.movement_mode
#define LEGACY_MOVEMENT_MODE 0
#define TILE_MOVEMENT_MODE 1
#define PIXEL_MOVEMENT_MODE 2

// generator() distributions
#define UNIFORM_RAND 0
#define NORMAL_RAND 1
#define LINEAR_RAND 2
#define SQUARE_RAND 3

// json_encode() flags
#define JSON_PRETTY_PRINT 1
