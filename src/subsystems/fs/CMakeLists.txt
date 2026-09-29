# MIT License
#
# Copyright (c) 2026 Grzegorz Grzęda
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
#
# Filesystems are compiled only when the device description mounts one.
if("fat" IN_LIST HOMECORE_DT_FILESYSTEMS)
    set(HOMECORE_FATFS_DIR ${HOMECORE_ROOT}/external/fatfs)
    set(HOMECORE_FATFS_SOURCES
        ${HOMECORE_FATFS_DIR}/ff.c
        ${HOMECORE_FATFS_DIR}/ffsystem.c
        ${HOMECORE_FATFS_DIR}/ffunicode.c
    )
    target_sources(${PROJECT_NAME} PRIVATE fat/fat.c fat/diskio.c ${HOMECORE_FATFS_SOURCES})
    # fat/ provides ffconf.h, which ff.h includes by name.
    target_include_directories(${PROJECT_NAME} PRIVATE fat ${HOMECORE_FATFS_DIR})
endif()
