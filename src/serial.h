#ifndef SERIAL_H
#define SERIAL_H

#include <termios.h>
#include <fcntl.h>
#include "baudrate.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int serial_init();
void serial_close(int fd);

#endif
