#!/bin/sh

set -xe

gcc -o tilemap_editor src/main.c src/tilemap.c src/file_picker.c -lm -lraylib ./libimgui.a -lstdc++
