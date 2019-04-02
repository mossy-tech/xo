# Copyright © 2019 Noah Santer <personal@mail.mossy-tech.com>
#
# This file is part of xo.
#
# xo is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# xo is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with xo.  If not, see <https://www.gnu.org/licenses/>.

#### OPTIONS ####
## Project
PREFIX ?= /usr/local
SOCKPATH ?= $(PREFIX)/share/xo/xod.sock
#SOCKPATH ?= /tmp/xod.sock
READLINE ?= 1
FP ?= 64
LADSPA_UID ?= 9001
LADSPA_LABEL ?= xo-xod
LADSPA_NAME ?= "XO +xod (FP-$(FP))"
LIMITER ?= 1

## Generic
NDEBUG ?= 0
NSANITIZE ?= $(NDEBUG)
VERSION ?= "$(shell git describe)$(shell git diff --quiet || echo -n '-alpha')"
#################

#### Flags ####
ALLDEFS  = -DFP=$(FP) 
ALLDEFS += -DLIMITER=$(LIMITER)
ALLDEFS += -DDEFAULT_SOCKPATH='"$(SOCKPATH)"'
ALLDEFS += -DVERSION='$(VERSION)'
LADSPADEFS  = -DNAME='$(LADSPA_NAME)'
LADSPADEFS += -DUNIQUE_ID="$(LADSPA_UID)"
LADSPADEFS += -DLABEL='"$(LADSPA_LABEL)"'

ALLFLAGS = -Wall $(ALLDEFS)

ifneq ($(NDEBUG),0)
    ALLFLAGS += -Werror -DNDEBUG -O2 -flto
else
    ALLFLAGS += -g -Og -flto
endif

ifneq ($(NSANITIZE),0)
	SANITIZE := -fno-sanitize=all
else
	SANITIZE := -fsanitize=address,undefined
endif

ALLFLAGS += $(SANITIZE) $(CFLAGS) $(LDFLAGS)
ALLFLAGS := $(strip $(ALLFLAGS))

ifneq ($(READLINE),0)
    RLFLAGS = -DREADLINE=$(READLINE) -lreadline
else
    RLFLAGS = -DREADLINE=0
endif

LADSPAFLAGS = -shared -fPIC $(LADSPADEFS) $(ALLFLAGS)
###############

#### Sources ####
BACKENDH = xo.h
BACKENDC = xo.c config_reader.c sv.c

LADSPAH = $(BACKENDH)
LADSPAC = $(BACKENDC) ladspa_frontend.c

INFOH = $(BACKENDH) xo_describe.h
INFOC = $(BACKENDC) info_frontend.c xo_describe.c

BAKERH = $(BACKENDH)
BAKERC = $(BACKENDC) baker_frontend.c

JACKH = $(BACKENDH)
JACKC = $(BACKENDC) jack_frontend.c
#################

GPERF ?= gperf

.PHONY: default
default: xo-xod.so xod-cli

.PHONY: all
all: default xo-dummy

xod.gen.h: xod.y
	bison -Wall --defines=xod.gen.h -o xod.gen.c xod.y

keywords.gen.h: keywords.gperf xod.gen.h
	$(GPERF) keywords.gperf > $@

xo-dummy: keywords.gen.h lex.c lex.h dummy.c ../xo.c ../xo_describe.c ../config_reader.c ../sv.c ../xo.h ../xo_describe.h impl.h
	$(CC) -o $@ $(ALLFLAGS) xod.gen.c lex.c dummy.c ../xo.c ../xo_describe.c ../config_reader.c ../sv.c -lm

xo-xod.so: keywords.gen.h lex.c lex.h ladspa.c ../xo.c ../xo_describe.c ../config_reader.c ../sv.c ../xo.h ../xo_describe.h impl.h
	$(CC) -o $@ $(LADSPAFLAGS) -I.. ladspa.c xod.gen.c lex.c ../xo.c ../xo_describe.c ../config_reader.c ../sv.c

xo-jack: keywords.gen.h lex.c lex.h jack.c ../xo.c ../xo_describe.c ../config_reader.c ../sv.c ../xo.h ../xo_describe.h impl.h
	$(CC) -o $@ $(ALLFLAGS) -I.. jack.c xod.gen.c lex.c ../xo.c ../xo_describe.c ../config_reader.c ../sv.c -ljack -lm


xod-cli: cli.c
	$(CC) -o $@ $(RLFLAGS) $(ALLFLAGS) cli.c

.PHONY: clean
clean:
	rm -fv xo-dummy xo-xod.so xod-cli *.gen.*

