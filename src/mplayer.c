#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include  "controls.h"
#include  "mplayer.h"
#include  "utils.h"

#undef SYSTEM_MPG123_INSTALL
#define LIBPATH         "../lib/bin/"

#ifdef SYSTEM_MPG123_INSTALL
  #define PLAYER_PATH     "/usr/bin/mpg123p"
#else
  #define PLAYER_PATH     LIBPATH"mpg123p"
#endif
#define MUSIC_ROOT      "../mp3s/"
#define MUSIC_SAMPLE    "Tracy.mp3"
#define MUSIC_PATH      MUSIC_ROOT MUSIC_SAMPLE
#define COMMAND         "-C"


int close_mp3(int *mp3_pid)
{
    int i = -1;
    if(mp3_pid != 0) {
        i = kill(*mp3_pid, SIGKILL);
        mp3_pid = 0;
    }
    return i;
}

int play_path(char *path, status_t *st)
{
    int i;
    close_mp3(&st->mp3_pid);

    if((st->mp3_pid = fork()) == 0) {
        // launch mpg123 in separate process binded by two pipes
        dup2(st->to_music_player[0], STDIN_FILENO);
        dup2(st->from_music_player[1], STDERR_FILENO);

        printf("[CALLING]%s %s %s\n", PLAYER_PATH, COMMAND, path);
        i = execlp(PLAYER_PATH, PLAYER_PATH, COMMAND, path, NULL);
        if(i) {
            perror("Invalid calling of mp3 player");
        }
        return i;
    } else {
        if(st->verbose){
            printf("[%s] Now playing:%s\n", time_printable(st->tmr_buf),
                   path
            );
        }
        return 0;
    }
}

int play_locally(char *path, status_t *st)
{
    char full_path[COMMAND_MAX_LEN];
    if (access(full_path, F_OK) != -1) {
        play_path(full_path, st);
    } else {
        //file doesn't exist, something went wrong
    }
    return 0;
}

/* write string to slave music player through pipe */
int swrite_mplayer(int pfd, char *s, int n)
{
    int i = 0;
//    i = write(st->to_music_player[1], s, n);

    return i;
}

/* write string to slave music player through pipe */
int cwrite_mplayer(char c)
{
    int i = 0;
    char cmd[2] = { '\0' };
    cmd[0] = c;

//    i = write(*st->to_music_player[1], cmd, 1);

    return i;
}

/* command slave mp3 player */
void mplayer_pause(void)
{
    cwrite_mplayer(MPG123_PAUSE_KEY);
}

void mplayer_louder(void)
{
    cwrite_mplayer(MPG123_VOL_UP_KEY);
}



