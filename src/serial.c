#include "serial.h"


extern const char g_serfile[];
extern int32_t g_serspeed;

int serial_init() {
    int fd = open(g_serfile, O_RDWR);
    if (fd < 0) {
        printf("Unable to open serial device: %s\n", g_serfile);
        exit(EXIT_FAILURE);
    }

    // Configure the serial port settings
    struct termios seropts;
    tcgetattr(fd, &seropts);

    seropts.c_cflag = CS8 | CREAD | CLOCAL;
    seropts.c_cc[VMIN] = 1;
    seropts.c_cc[VTIME] = 5;
    speed_t speed = baudrate(g_serspeed);
    cfsetospeed(&seropts, speed);
    cfsetispeed(&seropts, speed);

    return fd; // Success
}

void serial_close(int fd) {
    close(fd);
}