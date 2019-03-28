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
FP ?= 64
LADSPA_UID ?= 9000
LADSPA_LABEL ?= xo
LADSPA_NAME ?= "XO (FP-$(FP))"
LIMITER ?= 1

## Generic
NDEBUG ?= 0
NSANITIZE ?= $(NDEBUG)
VERSION ?= "$(shell git describe)$(shell git diff --quiet || echo -n '-alpha')"
#################

#### Flags ####
ALLDEFS = -DFP=$(FP) -DLIMITER=$(LIMITER) -DVERSION='$(VERSION)'
LADSPADEFS = -DNAME='$(LADSPA_NAME)'
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

LADSPAFLAGS = -shared -fPIC $(LADSPADEFS) $(ALLFLAGS) -fno-sanitize=all
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

.PHONY: default
.SILENT: default
default: xo.so xo-jack xod

.PHONY: all
all: xo.so xo-info xo-baker xo-jack xod-all

xo-info: $(INFOC) $(INFOH)
	$(CC) -o $@ $(ALLFLAGS) $(INFOC)

xo-baker: $(BAKERC) $(BAKERH)
	$(CC) -o $@ $(ALLFLAGS) $(BAKERC)

xo.so: $(LADSPAC) $(LADSPAH)
	$(CC) -o $@ $(LADSPAFLAGS) $(LADSPAC)

xo-jack: $(JACKC) $(JACKH)
	$(CC) -o $@ $(ALLFLAGS) $(JACKC) -ljack

xod:
	$(MAKE) -C xod

xod-all:
	$(MAKE) -C xod all

.PHONY: install
install: all
	install -d -m755 $(DESTDIR)$(PREFIX)/share/xo
	install -d -m755 $(DESTDIR)$(PREFIX)/lib/ladspa
	install -d -m755 $(DESTDIR)$(PREFIX)/bin
	install -d -m755 $(DESTDIR)$(PREFIX)/lib/xo/bin
	install -m755 xo.so $(DESTDIR)$(PREFIX)/lib/ladspa/
	install -m755 xod/xo-xod.so $(DESTDIR)$(PREFIX)/lib/ladspa/
	install -m755 xod/xod-cli $(DESTDIR)$(PREFIX)/bin/xod
	install -m755 xo-jack $(DESTDIR)$(PREFIX)/bin/
	install -m755 xod/xo-dummy $(DESTDIR)$(PREFIX)/lib/xo/bin/
	install -m755 xo-info $(DESTDIR)$(PREFIX)/lib/xo/bin/
	install -m755 xo-baker $(DESTDIR)$(PREFIX)/lib/xo/bin/

.PHONY: clean
clean:
	$(MAKE) -C xod clean
	rm -fv xo.so xo-info xo-baker xo-jack

