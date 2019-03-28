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
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <getopt.h>

#include "config.h"
#include "color.h"

#include <readline/readline.h>
#include <readline/history.h>

static bool quiet;

bool col_err, col_out;

static const char * sockpath = DEFAULT_SOCKPATH;

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

int handle_connection(int peer, FILE * src)
{
    rl_bind_key('\t', rl_insert);
    char * line = NULL;
    size_t linen = 0;
    for (;;) {
        ssize_t l;
        if (src == stdin) {
            line = readline(">> ");
            l = line ? strlen(line) : -1;
            add_history(line);
        } else {
            l = getline(&line, &linen, src);
        }

        if (l >= 1024) {
            PRINT(stderr, "%sline length exceeds maximum (1024)%s\n",
                    c_err(), c_off());
        }

        if (should_exit || l == -1) {
            if (!quiet) {
                PRINT(stderr, "\n%sexiting%s.\n",
                        c_info(), c_off());
            }
            close(peer);
            free(line);
            return HANDLE_EOF;
        }
        ssize_t n = send(peer, line, l, 0);
        free(line);
        line = NULL;
        if (n <= 0) {
            close(peer);
            if (got_sigpipe || should_exit || n == 0) {
                if (!quiet) {
                    PRINT(stdout, "%sdisconnected%s.\n",
                            c_info(), c_off());
                }
                return HANDLE_OKAY;
            } else {
                PRINT(stderr, "%serror sending%s!\n",
                        c_err(), c_off());
                return HANDLE_ERROR;
            }
        }

        int response;
        do {
            ssize_t r = recv(peer, &response, sizeof(response), 0);
            if (r <= 0) {
                close(peer);
                if (got_sigpipe || should_exit || r == 0) {
                    if (!quiet) {
                        PRINT(stderr, "%sdisconnected%s.\n",
                                c_info(), c_off());
                    }
                    free(line);
                    return HANDLE_OKAY;
                } else {
                    PRINT(stderr, "%serror receiving%s!\n",
                            c_info(), c_off());
                    free(line);
                    return HANDLE_ERROR;
                }
            }
            if (response > 2) {
                PRINT(stdout, "<< %sunknown error %d%s!\n",
                        c_err(), response, c_off());
            } else if (response == 2) {
                PRINT(stdout, "<< %sOOM error%s!\n",
                        c_err(), c_off());
            } else if (response == 1) {
                PRINT(stdout, "<< %ssyntax error%s!\n",
                        c_err(), c_off());
            } else if (!quiet && response == 0) {
                PRINT(stdout, "<< %sok%s.\n",
                        c_ok(), c_off());
            } else if (!quiet) {
                PRINT(stdout, "<< %d,\n", -response - 1);
            }
        } while (response < 0);
    }
}

int main(int argc, char ** argv)
{
    int listen_mode = 'd';
    int show_version = 0;

    struct option long_options[] = {
        { "version", no_argument, 0, 'V' },

        { "quiet", no_argument, 0, 'q' },

        { "single", no_argument, 0, 's' },
        { "no-single", no_argument, 0, 'l' },

        { "file", required_argument, 0, 'f' },

        { "path", required_argument, 0, 'p' },

        { 0, 0, 0, 0 }
    };

    FILE * src = NULL;

    int option_index = 0;
    char c;
    while ((c = getopt_long(argc, argv, "Vqslf:",
                    long_options, &option_index)) != -1)
    {
        switch (c) {
            case 'V':
                show_version = 1;
                break;
            case 'q':
                quiet = true;
                break;
            case 's':
            case 'l':
                listen_mode = c;
                break;
            case 'f':
                if (src) {
                    fclose(src);
                }
                src = fopen(optarg, "r");
                break;

            case 'p':
                sockpath = optarg;
                break;
            case '?':
                exit(1);
            default:
                abort();
        }
    }

    if (show_version) {
        PRINT(stderr, "%s\n", VERSION);
        exit(0);
    }

    if (listen_mode == 'd') {
        if (isatty(0)) {
            listen_mode = 'l';
        } else {
            listen_mode = 's';
        }
    }
    
    if (src == NULL) {
        src = stdin;
    }

    if (isatty(2)) {
        col_err = true;
    }
    if (isatty(1)) {
        col_out = true;
    }

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        PRINT(stderr, "%serror creating socket%s!\n",
                c_err(), c_off());
        exit(1);
    }

    struct sockaddr_un address;
    address.sun_family = AF_UNIX;
    strcpy(&address.sun_path[0], sockpath);

    if (bind(sock, (struct sockaddr *)&address,
                offsetof(struct sockaddr_un, sun_path) +
                strlen(sockpath) + 1)) {
        PRINT(stderr, "%serror binding %s!%s\n",
                c_err(), sockpath, c_off());
        close(sock);
        exit(1);
    }

    PRINT(stderr, "%sbound %s%s,\n",
            c_info(), sockpath, c_off());

    if (listen(sock, 1)) {
        PRINT(stderr ,"%serror listening%s!\n",
                c_err(), c_off());
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
        if (!quiet) {
            PRINT(stderr, "%swaiting for connection%s.\n",
                    c_info(), c_off());
        }
        int peer = accept(sock, NULL, NULL);
        if (peer == -1) {
            close(sock);
            unlink(sockpath);
            if (should_exit) {
                if (!quiet) {
                    PRINT(stderr, "\n%sexiting%s.\n",
                            c_info(), c_off());
                }
                exit(0);
            } else {
                PRINT(stderr, "%serror accepting%s!\n",
                        c_err(), c_off());
                exit(1);
            }
        }
        if (!quiet) {
            PRINT(stderr, "%sconnected%s.\n",
                    c_info(), c_off());
        }
       
        if (src != stdin && listen_mode == 'l') {
            fseek(src, 0, SEEK_SET);
        }
        int result = handle_connection(peer, src);
        if (result == HANDLE_EOF) {
            close(sock);
            unlink(sockpath);
            exit(0);
        } else if (result == HANDLE_ERROR) {
            close(sock);
            unlink(sockpath);
            exit(1);
        } else { //ok
        }

        if (listen_mode == 's') {
            close(sock);
            unlink(sockpath);
            exit(0);
        }
    }
}

