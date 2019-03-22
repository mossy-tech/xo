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
FP ?= 64
LADSPA_UID ?= 9000
LADSPA_LABEL ?= xo
LADSPA_NAME ?= "XO (FP-$(FP))"
LIMITER ?= 1

## Generic
NDEBUG ?= 0
NSANITIZE ?= $(NDEBUG)
#################

#### Flags ####
ALLDEFS = -DFP=$(FP) -DLIMITER=$(LIMITER)
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

ALLFLAGS += $(SANITIZE) $(CFLAGS) $(LDFLAGS) -lm
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
#################

.PHONY: default
default: xo.so

.PHONY: all
all: xo.so xo-info xo-baker

xo-info: $(INFOC) $(INFOH)
	$(CC) -o $@ $(ALLFLAGS) $(INFOC)

xo-baker: $(BAKERC) $(BAKERH)
	$(CC) -o $@ $(ALLFLAGS) $(BAKERC)

xo.so: $(LADSPAC) $(LADSPAH)
	$(CC) -o $@ $(LADSPAFLAGS) $(LADSPAC)

gperf: cmd/keywords.txt cmd/mkgperf.sh
	mkdir -p cmd/gen
	./cmd/mkgperf.sh < cmd/keywords.txt > cmd/gen/keywords.gperf
	gperf cmd/gen/keywords.gperf > cmd/gen/keywords.h
	$(CC) -o cmd/test $(ALLFLAGS) cmd/test.c

#cmd-grammar: cmd/cmd.l cmd/cmd.y cmd/cmd.h xo.h
#	mkdir -p cmd/gen
#	bison --defines=cmd/gen/cmd.tab.h -o cmd/gen/cmd.tab.c cmd/cmd.y
#	flex -o cmd/gen/lex.yy.c cmd/cmd.l

.PHONY: install
install: xo.so
	#install -g audio -m775 -d /usr/local/share/xo
	install -d -m755 /usr/local/lib/ladspa
	install -m755 xo.so /usr/local/lib/ladspa/

.PHONY: clean
clean:
	rm -fv xo.so xo-info xo-baker

