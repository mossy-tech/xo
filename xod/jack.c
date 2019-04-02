#include "xo.h"
#include "lex.h"

#include <jack/jack.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ladspa.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#ifndef CLIENT_NAME
#define CLIENT_NAME "xo"
#endif

struct userdata {
    struct xo * xo;
    jack_port_t * pin_0, * pin_1;
    jack_port_t ** pout;
    output_type ** outbufs;
};

int on_process(jack_nframes_t nframes, void * arg) {
    struct userdata * userdata = arg;

    input_type * inbuf0 = jack_port_get_buffer(userdata->pin_0, nframes);
    input_type * inbuf1 = jack_port_get_buffer(userdata->pin_1, nframes);

    for (size_t i = 0; i < userdata->xo->n_chains; i++) {
        userdata->outbufs[i] = jack_port_get_buffer(userdata->pout[i], nframes);
    }
    xo_process_chain(userdata->xo, inbuf0, inbuf1, nframes, userdata->outbufs);

    return 0;
}

void on_jack_shutdown(void * arg) {
    struct userdata * userdata = arg;
    xo_free(userdata->xo);
    free(userdata->pout);
    free(userdata->outbufs);
    free(userdata);
    fprintf(stderr, "JACK shutdown\n");
    exit(1);
}

static int sock = 0;

void respond(int response)
{
    send(sock, &response, sizeof(response), 0);
}

void commit(struct xo * xo)
{
    // do nothing
}

static const char * sockpath = DEFAULT_SOCKPATH;

int connect_client()
{
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    
    if (sock == -1) {
        fprintf(stderr, "error creating socket\n");
        exit(1);
    }

    struct sockaddr_un address;
    address.sun_family = AF_UNIX;
    strcpy(&address.sun_path[0], sockpath);

    if (connect(sock, (struct sockaddr *)&address,
                offsetof(struct sockaddr_un, sun_path) +
                strlen(sockpath) + 1) == -1) {
        close(sock);
        return 0;
    }

    return sock;
}

static bool configure(struct xo * xo)
{
    fprintf(stderr, "connecting to %s,\n", sockpath);
    sock = connect_client();
    if (!sock) {
        fprintf(stderr, "failed to connect.\n");
        return false;
    } else {
        fprintf(stderr, "connected,\n");
    }

    static char buf[1024];
    for (;;) {
        ssize_t n = recv(sock, buf, sizeof(buf) - 1, 0);
        if (n < 0) {
            fprintf(stderr, "error receiving!\n");
            return false;
        } else if (n == 0) {
            fprintf(stderr, "closing connection.\n");
            return true;
        }
        buf[n] = 0;

        const char * s = buf;
        int c = yyparse(&xo, &s);

        if (!s) {
            fprintf(stderr, "connection closed.\n");
            close(sock);
            return true;
        }

        send(sock, &c, sizeof(c), 0);
    }
}


int main(int argc, char ** argv) {

    /*
    if (argc < 2) {
        fprintf(stderr, "Syntax: %s [-V] CONFIG [...]\n", argv[0]);
        return 1;
    }
    */

    if (argc > 1 && !strcmp(argv[1], "-V")) {
        fprintf(stderr, "%s\n", VERSION);
        exit(0);
    }

    struct xo * xo = xo_alloc();

    /*
    for (int i = 1; i < argc; i++) {
        FILE * f = fopen(argv[i], "r");
        if (!f) {
            fprintf(stderr, "Cannot open file %s\n", argv[i]);
            return 1;
        }
        xo = xo_config_load_file(xo, f);
        if (!xo) {
            fprintf(stderr, "Failed to parse file %s\n", argv[i]);
            return 1;
        }
    }
    */

    if (configure(xo)) {
        xo_correct(xo, 48000.);
    } else {
        fprintf(stderr, "error\n");
        exit(1);
    }

    struct userdata * userdata = malloc(sizeof(*userdata));
    userdata->xo = xo;
    userdata->outbufs = malloc(sizeof(*userdata->outbufs) * xo->n_chains);

    jack_status_t open_status = 0;
    jack_client_t * client = jack_client_open(CLIENT_NAME, JackNoStartServer, &open_status, NULL);
    if (!client) {
        fprintf(stderr, "Failed to create client\n");
        if (open_status & JackServerStarted) {
            fprintf(stderr, "Failed to connect to server\n");
        }
        return 1;
    }
    jack_on_shutdown(client, on_jack_shutdown, NULL);

    userdata->pin_0 = jack_port_register(client, "left_in", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    userdata->pin_1 = jack_port_register(client, "right_in", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    userdata->pout = malloc(sizeof(*userdata->pout) * xo->n_chains);
    char name[256];
    for (size_t i = 0; i < xo->n_chains; i++) {
        snprintf(name, 255, "chain_%zd_out", i);
        userdata->pout[i] = jack_port_register(client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    }
    jack_set_process_callback(client, on_process, userdata);

    if (jack_activate(client)) {
        fprintf(stderr, "Failed to activate\n");
        return 1;
    }

    fgetc(stdin);

    jack_client_close(client);

    fprintf(stderr, "Limiter report:\n");
#if LIMITER
    for (size_t i = 0;i < xo->n_chains; i++) {
        fprintf(stderr, "Chain %zu = %f\n", i, (double)xo->chains[i].limiter);
    }
#endif

    xo_free(xo);
    free(userdata->pout);
    free(userdata->outbufs);
    free(userdata);

    return 0;
}
