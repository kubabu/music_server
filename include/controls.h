/* */


/* Commands for backends */
/* MPG123, straight from its source  */
#define MPG123_STOP_KEY 's'  /* space bar is alias for that */
#define MPG123_PAUSE_KEY    'p'
#define MPG123_QUIT_KEY 'q'
#define MPG123_VOL_UP_KEY '+'
#define MPG123_VOL_DOWN_KEY '-'
#define MPG123_REWIND_KEY   ','
#define MPG123_FORWARD_KEY  '.'
/* This is convenient on QWERTZ-keyboards. */
#define MPG123_FAST_REWIND_KEY ';'
#define MPG123_FAST_FORWARD_KEY ':'
#define MPG123_FINE_REWIND_KEY '<'
#define MPG123_FINE_FORWARD_KEY '>'
/* You probably want to use the following bindings instead
 * on a standard QWERTY-keyboard:
 */
/* #define MPG123_FAST_REWIND_KEY '<' */
/* #define MPG123_FAST_FORWARD_KEY '>' */
/* #define MPG123_FINE_REWIND_KEY ';' */
/* #define MPG123_FINE_FORWARD_KEY ':' */

/* MPV, not yet implemented */
#define MPV_PAUSE_KEY   ' '
#define MPV_QUIT_KEY    'q'
/* space bar is alias for that */
#define MPV_VOL_UP_KEY '0'
#define MPV_VOL_DOWN_KEY '9'
#define MPV_REWIND_KEY   '<-'
#define MPV_FORWARD_KEY  '->'


//Serious business
#define PASS_LENGTH 5
#define PASS 'root'

#define COMMAND_MAX_LEN 512
