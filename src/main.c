#include <stdlib.h>
#include <unistd.h>
#include "serial.h"
#include "game.h"
#include "arguments.h"

//Global variables
char g_infile[CFGSTR_SIZE] = "stdin";
char g_serfile[CFGSTR_SIZE] = "";
int32_t g_serspeed = 0;


int main(int argc, char *argv[]) {

    // Handle command line arguments
    arguments_handle(argc, argv);

    // Initialize serial port
    int ser_fd = serial_init();
    // Create thread to read from serial port
    game_init(ser_fd);
    // Run the game loop
    game_loop(ser_fd);

    game_cleanup();
    // Close the serial port
    serial_close(ser_fd);

    exit(EXIT_SUCCESS);
}
