#!/bin/sh

set -xe

gcc -ggdb -o tilemap_editor src/main.c src/tilemap.c -lm -lraylib
