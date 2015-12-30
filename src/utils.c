#include <ctype.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "utils.h"
#include "mpg123.h"

#define swrite(c, s)    write(c, s, strlen(s))


io_buffer_t *io_buf_init(io_buffer_t *buf)
{
    if(buf == NULL) {
        buf = malloc(sizeof(io_buffer_t));
    }
    buf->ni = 0;
    buf->oi = 0;
    buf->st = BUFFER_EMPTY;

    return buf;
}

io_buf_status io_buf_write(io_buffer_t *buf, char c)
{
    int ni = (buf->ni + 1) % IO_BUF_SIZE;

    if(ni == buf->oi) {
        buf->st = BUFFER_FULL;
        return BUFFER_FULL;

    }
    buf->data[buf->ni] = c;
    buf->ni = ni;
    buf->st = BUFFER_OK;

    return BUFFER_OK;
}

io_buf_status io_buf_read(io_buffer_t *buf, char *c)
{

    if(buf->oi == buf->ni) {
        buf->st = BUFFER_EMPTY;
        return BUFFER_EMPTY;
    }
    *c = buf->data[buf->oi];
    buf->oi = (buf->oi + 1) % IO_BUF_SIZE;
    buf->st = BUFFER_OK;

    return BUFFER_EMPTY;
}

io_buf_status io_buf_peek(io_buffer_t *buf, char *c)
{
    int ni = (buf->ni + 1) % IO_BUF_SIZE;

    if(buf->ni == buf->oi) {
        return BUFFER_EMPTY;
    }
    *c = buf->data[ni];

    return BUFFER_OK;
}

char ends_cmd(char c) {
    switch(c) {
        case '%':
        case EOF:
            return -1;
        case '\0':
        case '\r':
        case '\n':
            return 1;
    }
    return 0;
}



/* low resolution timeout counter */
char timeout(time_t *t, int lim)
{
    if(*t + lim > time(NULL)) {
        return 0;
    }
    return 1;
}

void timeout_update(time_t *t)
{
    time(t);
}


/* printable timestamp */
char *timestamp(char timer_buffer[TIME_BUFLEN])
{
    time_t timer;
    struct tm *timer_info;

    time(&timer);
    timer_info = localtime(&timer);
    strftime(timer_buffer, 26, "%Y:%m:%d %H:%M:%S", timer_info);

    return timer_buffer;
}

char *get_ip(char *buff)
{
    char c;
    int n, p[2];
    pipe(p);

    if(!fork()){
        close(p[0]);
        dup2(p[1], STDOUT_FILENO);
         /* soo portable */
        system("/sbin/ifconfig eth0 | grep 'inet addr:' | cut -d: -f2 | awk '{ print $1}'");
        close(p[1]);
        exit(EXIT_SUCCESS);
    } else {
        close(p[1]);
        n = 0;
        while(read(p[0], &c, 1) & (n + 1 < IP_BUFLEN)) {
            if(isalnum(c) | ispunct(c)) {
                buff[n++] = c;
            }
        }
        if(n == 0) {
            n = strlen("localhost");
            strncpy(buff, "localhost", n);
        }
        close(p[0]);
        buff[n] = '\0';
    }
    return buff;
}


char conf_pswd(char *pass, int count)
{
    char valid_pass[PASS_LENGTH];

    memset(valid_pass, '\0', PASS_LENGTH);
    strncpy(valid_pass, "PASS\0", PASS_LENGTH);

    if((memcmp(valid_pass, pass, PASS_LENGTH - 1)) || (count != PASS_LENGTH) ) {
#ifdef DEBUG
        int i;
        for(i=0; i < PASS_LENGTH; ++i) {
            printf("[%d] %c{%d} -  %c{%d}\n", i, pass[i], pass[i],
                                        valid_pass[i], valid_pass[i]);
        }
#endif
        printf("Client authentication failed: got %s [%d] expected %s [%d]\n",
                pass, count, valid_pass, PASS_LENGTH);
        return 0;
    }
    return 1;
}

/* get first free index of pointers array */
int getcid(client_t **buf, int max)
{
    int i;
    i = 0;
    for(i = 0; i < max; ++i) {
        if(buf[i] == NULL) {
            return i;
        }
    }
    return -1;
}

int printcl(client_t *cl, int i)
{
    if(cl == NULL) {
        printf("Cl %d empty \n", i);
        return -1;
    }
    printf("___\n|cfd=%d\n|cid=%d\n|tid=%d\n", cl->cfd, cl->cid, (int)cl->tid);
    return cl->cid;
}

void thread_sig_capture(int sig) {
    printf("\rThread intercepted signal %d\n", sig);
    st.exit = 1;
    shutdown(st.sfd, SHUT_RDWR);
}

void json_mp3(int cfd, const char *path)
{
    int meta;
    mpg123_handle* m;
    mpg123_id3v1 *v1;
    mpg123_id3v2 *v2;

    mpg123_init();
    m = mpg123_new(NULL, NULL);

    if(mpg123_open(m, path) != MPG123_OK) {
        fprintf(stderr, "Cannot open %s: %s\n", path, mpg123_strerror(m));
    }
    mpg123_scan(m);
    meta = mpg123_meta_check(m);
    if(meta & MPG123_ID3 && mpg123_id3(m, &v1, &v2) == MPG123_OK) {

        swrite(cfd, "{\"path\":\"");
        swrite(cfd, path);
        swrite(cfd, "\",\"title\":\"");
        if(v2->title != NULL && v2->title->fill) {
            write(cfd, v2->title->p, v2->title->fill);
        }
        swrite(cfd, "\",\"artist\":\"");
        if(v2->artist != NULL && v2->artist->fill) {
            write(cfd, v2->artist->p, v2->artist->fill);
        }
        swrite(cfd, "\",\"year\":\"");
        if(v2->year != NULL && v2->year->fill) {
            write(cfd, v2->year->p, v2->year->fill);
        }
        swrite(cfd, "\",\"genre\":\"");
        if(v2->genre != NULL && v2->genre->fill) {
            write(cfd, v2->genre->p, v2->genre->fill);
        }
        swrite(cfd, "\"}");
    }
}

/* list all mp3 files in directory */
void json_list_mp3s(int cfd, const char *path)
{
    char many = 0;
    char fullpath[COMMAND_MAX_LEN];
    int pos;
    char *fname;
    DIR *d;
    struct dirent *dir;

    char *starttoken = "{\"tracks\":[";
    char *endtoken = "]}";

    d = opendir(path);
    strncpy(fullpath, path, COMMAND_MAX_LEN);
    pos = strlen(path);

    swrite(cfd, starttoken);

    if(d) {
        while((dir = readdir(d)) != NULL) {
            if(dir->d_type == DT_REG) {
                fname = dir->d_name;
                if(strlen(fname) > 4 && !strcmp(fname + strlen(fname) - 4, ".mp3")) {
                    if(many) {
                        swrite(cfd, ",");
                    } else {
                        many = 1;
                    }
                    strncpy(fullpath + pos, fname, COMMAND_MAX_LEN - pos);
                    json_mp3(cfd, fullpath);
                }
            }
        }
        closedir(d);
    }
    swrite(cfd, endtoken);
    many = EOF;
    swrite(cfd, &many);
}

