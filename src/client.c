#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

/* #include "dbg.h" */
#include "controls.h"
#include "utils.h"

/* This client is intended only for server testing */

char read_on;
int conn_fd;
pthread_t main_th, rd_th;
io_buffer_t sockrd;


void safe_exit(int sig);


void *rdthread(void *buf)
{
    io_buffer_t *rdbuf = io_buf_init(buf);

    char c = '\0';

    while(read_on) {
        read(conn_fd, &c, 1);
        if(c == EOF) {
            read_on = 0;
            printf("Got EOF from server");
            break;
        }
        printf("GOT{%c} [%d]\n", c, c);
        io_buf_write(rdbuf, c);
        while(rdbuf->st == BUFFER_FULL && read_on) {
            sleep(1);
        }
    }

    if(rdbuf != buf) {
        free(buf);
    }
    safe_exit(0);
    return 0;
}

void conn_close(int conn_fd)
{
    char cmd;
    cmd = EOF;
    write(conn_fd, &cmd, 1);
    close(conn_fd);
}

void safe_exit(int sig)
{
    read_on = 0;
    shutdown(conn_fd, SHUT_RDWR);
    conn_close(conn_fd);
    if(sig) {
        printf("\rGot SIG%d, closing test client...\n", sig);
    } else {
        printf("\rClosing test client...\n");
    }
    if(pthread_self() == main_th) {
        pthread_join(rd_th, NULL);
    } else {
       /* pthread_exit(NULL); */
        puts("exit from rdthread");
    }
     exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    int fd, con, n, port;
    unsigned int m;
    char cmd_buf[COMMAND_MAX_LEN];
    struct sockaddr_in serv_addr;
    struct hostent *host_addr;
    time_t tmr;

    signal(SIGINT, safe_exit);
    signal(SIGQUIT, safe_exit);
    read_on = 1;
    main_th = pthread_self();
    memset(cmd_buf, '\0', COMMAND_MAX_LEN);

    if(argc < 3) {
        printf("USE %s [IP_ADDR] [PORT]\n", argv[0]);
        return 1;
    }
    port = atoi(argv[2]);
    if(port < 0) {
        printf("Invalid port: %d\n", port);
        return 1;
    }
    host_addr = gethostbyname(argv[1]);

    fd = socket(PF_INET, SOCK_STREAM, 0);
    if(fd < 0) {
        puts("Failed to open socket");
        return 1;
    }

    serv_addr.sin_family = PF_INET;
    serv_addr.sin_port = htons(port);

    memcpy(&serv_addr.sin_addr.s_addr, host_addr->h_addr, host_addr->h_length);


    con = connect(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if(con != 0) {
        perror("Problem with connecting to server");
        return 1;
    }
    n = write(fd, "PASS", PASS_LENGTH);
    if(n != PASS_LENGTH) {
        perror("Problem with authentication to server");
    }

    conn_fd = fd;

    n = 0;

    if(argc > 3) {
        for(m = 3; (int)m < argc; m++) {
            write(fd, argv[m], strlen(argv[m]));
            write(STDOUT_FILENO, argv[m], strlen(argv[m]));
        }
    } else {
        printf("<REPL mode>\n");
        timeout_update(&tmr);

        pthread_create(&rd_th, NULL, rdthread, &sockrd);

        while(!timeout(&tmr, CLIENT_CMD_TIMEOUT_SEC) && read_on) {
            m = sizeof(n);
            con = getsockopt(fd, SOL_SOCKET, SO_ERROR, &n, &m);
            if(con) {
                perror("Connection possibly lost");
                exit(EXIT_SUCCESS);
            }
            read(STDIN_FILENO, cmd_buf, 1);
            write(fd, cmd_buf, 1);
            timeout_update(&tmr);
/*            n = 0;
            memset(cmd_buf, '\0', COMMAND_MAX_LEN);
            for(n = 0; n < COMMAND_MAX_LEN; n++) {
                read(STDIN_FILENO, cmd_buf, 1);
                write(fd, cmd_buf, 1);
            }  */
        }
    }
    safe_exit(0);

    return 0;
}

