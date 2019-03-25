/* Copyright © 2019 Noah Santer <personal@mail.mossy-tech.com>
 *
 * This file is part of xo.
 * 
 * xo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * xo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with xo.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

static const char * sockpath = "/usr/local/share/xo/sock";

static bool should_exit = false;
static bool got_sigpipe = false;

void sig_handler(int signo)
{
    if (signo == SIGINT) {
        should_exit = true;
    }
    if (signo == SIGPIPE) {
        got_sigpipe = true;
    }
}

#define HANDLE_OKAY     0
#define HANDLE_EOF      1
#define HANDLE_ERROR    2

int handle_connection(int peer)
{
    char * line = NULL;
    size_t linen = 0;
    for (;;) {
        printf(">> ");
        size_t l = getline(&line, &linen, stdin);
        if (!isatty(0)) {
            printf("%s", line);
        }
        if (should_exit || l == -1) {
            fprintf(stderr, "\nexiting.\n");
            close(peer);
            free(line);
            return HANDLE_EOF;
        }
        ssize_t n = send(peer, line, l, 0);
        if (n <= 0) {
            close(peer);
            if (got_sigpipe || should_exit || n == 0) {
                printf("disconnected.\n");
                free(line);
                return HANDLE_OKAY;
            } else {
                fprintf(stderr, "error sending\n");
                free(line);
                return HANDLE_ERROR;
            }
        }

        int response;
        do {
            ssize_t r = recv(peer, &response, sizeof(response), 0);
            if (r <= 0) {
                close(peer);
                if (got_sigpipe || should_exit || r == 0) {
                    printf("disconnected.\n");
                    free(line);
                    return HANDLE_OKAY;
                } else {
                    fprintf(stderr, "error receiving\n");
                    free(line);
                    return HANDLE_ERROR;
                }
            }
            if (response > 2) {
                printf("<< unknown error.\n");
            } else if (response == 2) {
                printf("<< OOM error.\n");
            } else if (response == 1) {
                printf("<< syntax error.\n");
            } else if (response == 0) {
                printf("<< ok.\n");
            } else {
                printf("<< %d,\n", -response - 1);
            }
        } while (response < 0);
    }
}

int main(int argc, char ** argv)
{
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        fprintf(stderr, "error creating socket\n");
        exit(1);
    }

    struct sockaddr_un address;
    address.sun_family = AF_UNIX;
    strcpy(&address.sun_path[0], sockpath);

    if (bind(sock, (struct sockaddr *)&address,
                offsetof(struct sockaddr_un, sun_path) +
                strlen(sockpath) + 1)) {
        fprintf(stderr, "error binding\n");
        close(sock);
        exit(1);
    }

    if (listen(sock, 1)) {
        fprintf(stderr ,"error listening\n");
        close(sock);
        unlink(sockpath);
        exit(1);
    }

    struct sigaction act;
    act = (struct sigaction) { .sa_handler = sig_handler };
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGPIPE, &act, NULL);
    
    for (;;) {
        if (should_exit) {
            close(sock);
            unlink(sockpath);
            exit(0);
        }
        printf("waiting for connection...\n");
        int peer = accept(sock, NULL, NULL);
        if (peer == -1) {
            close(sock);
            unlink(sockpath);
            if (should_exit) {
                printf("\nexiting.\n");
                exit(0);
            } else {
                fprintf(stderr, "error accepting\n");
                exit(1);
            }
        }
        printf("connected.\n");
       
        int result = handle_connection(peer);
        if (result == HANDLE_OKAY) {
        } else if (result == HANDLE_EOF) {
            close(sock);
            unlink(sockpath);
            exit(0);
        } else if (result == HANDLE_ERROR) {
            close(sock);
            unlink(sockpath);
            exit(1);
        }
    }
}

