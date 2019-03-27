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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "../xo.h"
#include "lex.h"

static int sock = 0;

void respond(int response)
{
    send(sock, &response, sizeof(response), 0);
}

static FILE * commit_target;

void commit(struct xo * xo)
{
    xo_config_write_file(xo, commit_target);
}

static const char * sockpath = "/usr/local/share/xo/sock";

int connect_client()
{
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    
    if (sock == -1) {
        fprintf(stderr, "error creating socket\n");
        exit(1);
    }

//    size_t point = cleanup(close, (void*)(size_t)sock);

    struct sockaddr_un address;
    address.sun_family = AF_UNIX;
    strcpy(&address.sun_path[0], sockpath);

    if (connect(sock, (struct sockaddr *)&address,
                offsetof(struct sockaddr_un, sun_path) +
                strlen(sockpath) + 1) == -1) {
        close(sock);
//        clean(point);
        return 0;
    }

    return sock;
}

int main(int argc, char ** argv)
{

    if (argc <= 1) {
        commit_target = stdout;
    } else {
        commit_target = fopen(argv[1], "w");
        if (!commit_target) {
            fprintf(stderr, "failed to open %s for writing, exiting.\n", argv[1]);
            exit(1);
        }
    }

//    atexit(cleanall);
    fprintf(stderr, "connecting to %s...\n", sockpath);
    sock = connect_client();
    if (!sock) {
        fprintf(stderr, "failed, exiting.\n");
        exit(1);
    } else {
        fprintf(stderr, "connected.\n");
    }

    struct xo * xo = xo_alloc();

    char buf[1024];
    for (;;) {
        ssize_t n = recv(sock, buf, sizeof(buf) - 1, 0);
        if (n < 0) {
            fprintf(stderr, "error receiving\n");
            exit(1);
        } else if (n == 0) {
            printf("bye.\n");
            exit(0);
        }
        buf[n] = 0;

        const char * s = buf;
        int c = yyparse(&xo, &s);

        if (!s) {
            fprintf(stderr, "disconnecting.\n");
            close(sock);
            exit(0);
        }

        send(sock, &c, sizeof(c), 0);
    }
}

