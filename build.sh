#!/bin/sh

set -xe

gcc -o tilemap_editor src/main.c src/tilemap.c -lm -lraylib ./libimgui.a -lstdc++
