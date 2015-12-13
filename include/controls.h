/* */
#define MPLAYER_CMD_MODE 0
#define MPLAYER_COMMAND  1

#define MPLAYER_MODE_SET 's'
#define MPLAYER_MODE_LIST 'l'
#define MPLAYER_MODE_PLAY 'p'

#define MPLAYER_PLAY_PODCAST 'p'
#define MPLAYER_PLAY_LOCAL  'l'

#define MPLAYER_SET_PAUSE 'p'
#define MPLAYER_SET_STOP 's'
#define MPLAYER_SET_VOL_UP '+'
#define MPLAYER_SET_VOL_DOWN '-'
#define MPLAYER_SET_FORWARD '>'
#define MPLAYER_SET_REWIND '<'

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


#define MAX_CLIENT_COUNT 512

//Serious business
#define PASS_LENGTH 5
#define PASS 'root'

