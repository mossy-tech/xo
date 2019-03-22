#include "xo.h"

#include <jack/jack.h>
#include <string.h>
#include <stdlib.h>

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

int main(int argc, char ** argv) {

    if (argc < 2) {
        fprintf(stderr, "Syntax: %s CONFIG [...]\n", argv[0]);
        return 1;
    }

    /*
    if (strcmp(argv[1], "-v") == 0) {
        fprintf(stderr, "FP%u\n", FP);
        return 1;
    }
    */

    struct xo * xo = NULL;

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
