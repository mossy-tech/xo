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
#include "xo.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <ladspa.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#ifndef DEFAULT_SOCKPATH
#error "must #define DEFAULT_SOCKPATH"
#endif

#include "../xo.h"
#include "lex.h"
#include "impl.h"

#ifndef LABEL
#error "plugin missing LABEL"
#endif
#ifndef UNIQUE_ID
#error "plugin missing UNIQUE_ID"
#endif
#ifndef NAME
#error "plugin missing NAME"
#endif

#if BAKED_CHAINS
struct xo * xo_add_baked_chains();
#endif

// max chains
#ifndef MAX_OUTPUTS
#define MAX_OUTPUTS ((size_t)4)
#endif

#define CONTROL_PORTS 1

#define DEFAULT_FC (float_type)(70.)

// number of non-output ports
#define OFFSET (size_t)(2 + CONTROL_PORTS)

struct userdata {
    struct xo * xo;
    float * data[MAX_OUTPUTS + OFFSET];
    float_type control_current[CONTROL_PORTS];
    float_type sample_rate;
};

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

static LADSPA_Handle instantiate(
        const LADSPA_Descriptor * desc,
        unsigned long sample_rate)
{
    (void)desc;

    fprintf(stderr, LABEL ": LADSPA sample_rate = %lu\n", sample_rate);

    struct xo * xo = xo_alloc();

#if BAKED_CHAINS
    fprintf(stderr,
            LABEL ": load baked chains.\n");
    xo = xo_add_baked_chains(NULL);
#else

    if (configure(xo)) {
        fprintf(stderr, LABEL ": use chains from daemon.\n");

        xo_correct(xo, sample_rate);
    }

    if (xo->n_chains == 0) {
        fprintf(stderr, LABEL ": fall back to static chains.\n");

        FILE * f = xo_config_find(NULL);
        if (!f) {
            return NULL;
        }
        xo = xo_config_load_file(NULL, f);
        if (!xo) {
            return NULL;
        }
    }
#endif

    if (xo->n_chains > MAX_OUTPUTS) {
        fprintf(stderr,
                "xo: error, number of chains (%zu) exceeds MAX_OUPUTS (%zu)\n",
                xo->n_chains, MAX_OUTPUTS);
        return NULL;
    }

    struct userdata * userdata = malloc(sizeof(*userdata));
    *userdata = (struct userdata) {
        .xo = xo,
        .control_current = { 0. },
        .sample_rate = sample_rate
    };

    // required ???
    for (size_t i = 0; i < OFFSET + MAX_OUTPUTS; i++) {
        userdata->data[i] = NULL;
    }

    /*
    for (size_t i = 0; i < CONTROL_PORTS; i++) {
        userdata->control_data[i] = NULL;
    }
    */

    fprintf(stderr, LABEL ": instantiated.\n");

    return (LADSPA_Handle)userdata;
}

static void connect_port(LADSPA_Handle instance,
    unsigned long Port,
    float * location)
{
    struct userdata * userdata = instance;
    userdata->data[Port] = location;
}

/*
static void activate(
    LADSPA_Handle instance)
{
    fprintf(stderr, LABEL ": activated.\n");
    // do nothing
}

static void deactivate(LADSPA_Handle instance)
{
    fprintf(stderr, LABEL ": deactivated.\n");
    xo_reset(((struct userdata *)instance)->xo);
}
*/

static void run(LADSPA_Handle instance,
    unsigned long sample_count)
{
    struct userdata * userdata = instance;

    /*
    if (userdata->data[2]) {
        if (userdata->data[2][0] != userdata->control_current[0]) {
            userdata->control_current[0] = userdata->data[2][0];
            calculate(userdata->xo, userdata->control_current[0], userdata->sample_rate);
        }
    }
    */

    xo_process_chain(userdata->xo,
        userdata->data[0], userdata->data[1],
        sample_count,
        &userdata->data[OFFSET]);
}

void cleanup(LADSPA_Handle instance)
{
    struct userdata * userdata = instance;
    xo_free(userdata->xo);
    free(userdata);
    fprintf(stderr, LABEL ": cleaned up.\n");
}

extern const LADSPA_Descriptor * ladspa_descriptor(unsigned long index)
{
    //    (void)index;
    if (index > 0) return NULL;

    LADSPA_Descriptor * desc = malloc(sizeof(*desc));
    *desc = (LADSPA_Descriptor) {
        .UniqueID = UNIQUE_ID,
        .Label = strdup(LABEL),

        .Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE,

        .Name = strdup(NAME),
        .Maker = strdup("Noah Santer"),
        .Copyright = strdup("All Rights Reserved"),

        .PortCount = OFFSET + MAX_OUTPUTS,
        .PortDescriptors = NULL,
        .PortNames = NULL,
        .PortRangeHints = NULL,

        .ImplementationData = NULL,

        .instantiate = &instantiate,
        .connect_port = &connect_port,
        .activate = NULL,
        .run = &run,
        .run_adding = NULL,
        .set_run_adding_gain = NULL,
        .deactivate = NULL,
        .cleanup = &cleanup
    };

    LADSPA_PortDescriptor * pdesc =
        calloc(OFFSET + MAX_OUTPUTS, sizeof(desc->PortDescriptors));

    const char ** ndesc =
        malloc((OFFSET + MAX_OUTPUTS) * sizeof(desc->PortNames));

    LADSPA_PortRangeHint * phint =
        calloc(OFFSET + MAX_OUTPUTS, sizeof(desc->PortRangeHints));

    pdesc[0] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
    pdesc[1] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
    pdesc[2] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;

    ndesc[0] = strdup("left_in");
    ndesc[1] = strdup("right_in");
    ndesc[2] = strdup("fc");

    char name[256];
    for (size_t i = 0; i < MAX_OUTPUTS; i++) {
        pdesc[OFFSET + i] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
        snprintf(name, 255, "chain_%zu_out", i);
        ndesc[OFFSET + i] = strdup(name);
    }

    desc->PortDescriptors = pdesc;
    desc->PortNames = ndesc;
    desc->PortRangeHints = phint;

    return desc;
}

__attribute__((destructor)) void bye()
{
    fprintf(stderr, LABEL ": bye\n");
}

