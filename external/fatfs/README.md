# FatFs

Unmodified subset of ChaN's FatFs R0.16, from
https://elm-chan.org/fsw/ff/arc/ff16.zip
(SHA-256 99f7dc1f7e095356e4a9e3dbe29959090d8b948afe2bbc5441e52fdf4b85449e).

Included from `source/`: `ff.c`, `ff.h`, `diskio.h`, `ffsystem.c`,
`ffunicode.c`, `00readme.txt`, and `00history.txt`. Not included:
`ffconf.h`, whose HomeCore configuration is `src/subsystems/fs/fat/ffconf.h`,
and the example `diskio.c`, replaced by `src/subsystems/fs/fat/diskio.c`.

FatFs is distributed under a one-clause BSD-style license; the full text is at
the top of `ff.c`, `ff.h`, and `ffunicode.c` and must be kept with the source.
