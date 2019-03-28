# `xo`
*xo* is an audio digital signal processing (DSP) tool designed
to operate real-time on a digital audio source (e.g. a computer)
and distribute the signals onto multiple parallel playback channels.

As such, it is most useful in conjunction with a host system that is
acting as both a source and sink.
There are at present two supported host types, both geared towards use
on Linux:
 - LADSPA-capable hosts (e.g. ALSA)
 - JACK hosts

Of these, LADSPA is preferable; it is most "plug and play" and likely
ALSA is used to back JACK regardless.
It is also likely that pulseaudio supports LADSPA plugins but this has
been neither tested nor explored.
Other operating systems and pro-audio tools may or may not support
standard LADSPA plugins as well.

*xo* is dynamically configured at runtime through use of a configuration
file. This file is found by searching fixed locations, configured at
compile-time but typical of most configuration files. The LADSPA plugin
can also be built with support for a range of identifiers with the offset
into the range selecting between sequentially-numbered configuration
files.

Additionally, *xo* has the option of listening for configuration
instructions via a built-in server. This server could then be controlled
by a seperate program to reconfigure the plugin at runtime.
A project, tentatively *xo-gui* is planned for a graphical client of
this sort. See Configuration/Daemon below.



## Building

*xo.so* and *xo-jack* are the primary outputs.

```
make NDEBUG=1 && sudo make install
```

*xo* is built with make. Two additional tools, *xo-info* and *xo-baker*
are not built by default but through their own or the special `all` target.
Building always requires a C compiler that implements the floating point
types and functions from ISO/IEC TS 18661-3:2015.
This is satisfied by GCC beginning with major version 7.

A few build options are provided, in addition to respecting standard
`CC`, `CFLAGS`, and `LDFLAGS` environment variables:

 - `FP` can be set to 32, 64, 64x, or 128 to choose the encoding
   used for internal calculation and storage. x86\_64 hosts support
   all four (with 64x selecting the 80-bit real type.) ARM hosts
   support 32 and 64. The 128 target does not require *libquadmath*.

   The default is 64.

 - `LIMITER` enables automatic post-processing of outputs to reduce
   gain when clipping is detected. The gain reduction persists
   for the lifetime of the plugin.

   The default is 1 (enabled.)

 - `LADSPA_UID`, `LADSPA_LABEL`, `LADSPA_NAME` configure the reported
   UID, label, and name of the plugin. They are only used with the
   LADSPA frontend.

   The default is 9000, xo, and "XO (FP-*xxx*)" where *xxx* is the
   value of FP.

 - `NDEBUG` disables debug information
   and builds with `-O2` instead of `-Og`.

   The default is 0. For builds not part of active development,
   `NDEBUG=1` should be specified.

 - A related `NSANITIZE` flag exists and is controlled by default
   with the value of `NDEBUG`. It may be specified directly for
   debug builds on systems lacking *usan* and *asan* support
   (e.g. Windows) but is redundant if `NDEBUG=1` is also given.

## Configuration
There are currently two configuration file formats: raw .xo files
and xod session files. The first format is the older, historical
form and is used with the plugin frontends that have no daemon support:

 - *xo.so* searches for a *.xo* file in a predetermined list of directories
   (see `config_reader.c`.) The full name is determined by `LADSPA_LABEL`,
   e.g. *xo.xo* by default.
 - *xo-jack* reads one or more configuration files passed as command line
   arguments. *xo-baker* and *xo-info* follow this form as well.

The syntax for the file will be detailed in `doc/RAW.md` in the future.
For now, consult `config_reader.c`; it is a very simple, rigid format.

### Daemon
The second configuration type is used by plugin clients supporting
the *xod* setup. TODO: document. (See subdirectory for now.)

---
    Copyright (C) 2019 Noah Santer <personal@mail.mossy-tech.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

