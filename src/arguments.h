#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include "game.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include "serial.h"

#define CFGSTR_SIZE 128
#define DEBUG false

extern char g_infile[];
extern char g_serfile[];
extern int32_t g_serspeed;

enum {
    DEVICE_OPT = 0,
    SPEED_OPT,
    MAP_WIDTH,
    MAP_HEIGHT,
    SHIP_COUNT_1,
    SHIP_COUNT_2,
    SHIP_COUNT_3
};


void print_help();
void arguments_handle(int argc, char *argv[]);

#endif
