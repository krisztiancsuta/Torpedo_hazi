#include "game.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>

#define BUFLEN (1024)
#define PFDSLEN (2)

Game_Settings settings;
Map map;
Game_Actions action;


int game_init(int fd) {
    char buffer[128];
    // Initialize game state, allocate resources, etc.
    map.width = settings.map_width;
    map.height = settings.map_height;
    // Allocate memory for the map cells
    map.cells = (int **)malloc(map.height * sizeof(int *));
    if (map.cells == NULL) {
        printf("Memory allocation failed for rows\n");
        return -1;
    }
    for (int i = 0; i < map.height; i++) {
        map.cells[i] = (int *)malloc(map.width * sizeof(int));
        if (map.cells[i] == NULL) {
            printf("Memory allocation failed for row %d\n", i);
            // Free previously allocated rows
            for (int j = 0; j < i; j++) {
                free(map.cells[j]);
            }
            free(map.cells);
            return -1;
        }
    }
    // Initialize map cells to 0
    for (int i = 0; i < map.height; i++) {
        for (int j = 0; j < map.width; j++) {
            map.cells[i][j] = '~';
        }
    }


    // Send Start command to Host
    sprintf(buffer, "Start %d %d %d %d %d\n", map.width, map.height, settings.ship_count_1, settings.ship_count_2, settings.ship_count_3);
    write(fd, buffer, strlen(buffer));
    
    print_map();

    return 0; // Success
}


int is_exit_command(const char *input) {
    // Check if input contains an exit command: 'x', 'quit', or 'exit'
    // Input should already be null-terminated
    
    if (input == NULL || strlen(input) == 0) {
        return 0;
    }
    
    // Create a copy and strip trailing whitespace
    char buffer[BUFLEN];
    strncpy(buffer, input, BUFLEN - 1);
    buffer[BUFLEN - 1] = '\0';
    
    size_t len = strlen(buffer);
    while (len > 0 && (buffer[len-1] == '\n' || buffer[len-1] == '\r' || buffer[len-1] == ' ')) {
        buffer[--len] = '\0';
    }
    
    // Convert to lowercase for case-insensitive comparison
    char lower_buffer[BUFLEN];
    for (size_t i = 0; i < len && i < BUFLEN - 1; i++) {
        lower_buffer[i] = tolower(buffer[i]);
    }
    lower_buffer[len] = '\0';
    
    // Check if exit commands appear in the string
    if (strcmp(lower_buffer, "x") == 0 || 
        strstr(lower_buffer, "quit") != NULL || 
        strstr(lower_buffer, "exit") != NULL) {
        return 1;
    }
    
    return 0;
}


void game_loop(int serial_fd) {   
    bool exit_loop = false; 
    char linebuf[BUFLEN]; 
    static char receive_buffer[1024]; 
    static int buffer_pos = 0; 


    struct pollfd pfds[]={
        {
            .fd = STDIN_FILENO,
            .events = POLLIN
        },
        {
            .fd = serial_fd,
            .events = POLLIN
        }
    };

    int nfds = sizeof(pfds) / sizeof(pfds[0]);
    while (!exit_loop) {

        int ret = poll(pfds, nfds, -1);
        if (ret > 0) {
            if (pfds[0].revents & POLLIN) {
                ssize_t n = read(STDIN_FILENO, linebuf, BUFLEN);
                if (n <= 0) {
                    continue;
                }
                if (is_exit_command(linebuf)) {
                    // Send RESET command to board
                    write(serial_fd, "RESET\n", 6);
                    exit_loop = true;
                } else { 
                    // Validate move before sending
                    linebuf[n] = '\0'; // Null-terminate the string
                    if (move_is_valid(linebuf) != NULL) {
                        action.action[0] = '\0'; // Clear previous action
                        write(serial_fd, linebuf, n); // n karakter írása a soros portra
                    } else {
                        strcpy(action.action, "INVALID");
                        print_map();
                    }
                }
            } else if (pfds[1].revents & POLLIN) {
                ssize_t n = read(serial_fd, linebuf, BUFLEN - 1);
                if (n > 0) {
                    linebuf[n] = '\0';

                    // Write received data to file
                    FILE *log_file = fopen("received_data.txt", "a");
                    if (log_file != NULL) {
                        fprintf(log_file, "%s", linebuf);
                        fclose(log_file);
                    }
                    
                    // Accumulate data in buffer
                    if (buffer_pos + n < sizeof(receive_buffer) - 1) {
                        memcpy(receive_buffer + buffer_pos, linebuf, n);
                        buffer_pos += n;
                        receive_buffer[buffer_pos] = '\0';
                        
                        // Check if we received a complete message (ends with "END")
                        if (strstr(receive_buffer, "END") != NULL) {
                            process_received_data(receive_buffer);
                            buffer_pos = 0; // Reset buffer
                            receive_buffer[0] = '\0';
                            print_map();
                        }
                    } else {
         
                        buffer_pos = 0;
                        receive_buffer[0] = '\0';
                    }
                }
            }
        } else if (ret == 0) {
            // Timeout occurred, no data
            continue;
        } else {
            write(STDOUT_FILENO, "Error occurred\n", 6);
        }
    }

}

void process_received_data(const char *data) {
    // Parse the received message in format:
    // ACTION\r\n
    // MAP\r\n
    // row1\r\n
    // row2\r\n
    // row3\r\n
    // END\r\n
    
    // Work directly on the data string using pointer arithmetic
    const char *line_start = data;
    const char *line_end;
    int line_num = 0;
    int map_row = 0;
    
    while (*line_start != '\0') {
        // Find the end of the current line
        line_end = line_start;
        while (*line_end != '\0' && *line_end != '\n' && *line_end != '\r') {
            line_end++;
        }
        
        // Calculate line length
        size_t len = line_end - line_start;
        
        if (len == 0) {
            // Empty line, skip it
            if (*line_end == '\r') line_end++;
            if (*line_end == '\n') line_end++;
            line_start = line_end;
            continue;
        }
        
        // First line is the action
        if (line_num == 0) {
            size_t copy_len = len < sizeof(action.action) - 1 ? len : sizeof(action.action) - 1;
            memcpy(action.action, line_start, copy_len);
            action.action[copy_len] = '\0';
            
            // Check for game over
            if (strstr(action.action, "GAME OVER") != NULL || strstr(action.action, "WIN") != NULL) {
                action.is_game_over = 1;
            } else {
                action.is_game_over = 0;
            }
        }
        // Second line should be "MAP"
        else if (line_num == 1) {
            if (len != 3 || strncmp(line_start, "MAP", 3) != 0) {
                // Invalid format
                break;
            }
        }
        // Lines until "END" are map rows
        else if (len == 3 && strncmp(line_start, "END", 3) == 0) {
            break;
        }
        else if (map_row < map.height) {
            // Parse map row by going through each character
            // Format: "x x ~ " or "~ ~ ~ " (characters separated by spaces)
            int col = 0;
            for (size_t i = 0; i < len && col < map.width; i++) {
                char ch = line_start[i];
                // Skip spaces and tabs
                if (ch != ' ' && ch != '\t') {
                    map.cells[map_row][col] = ch;
                    col++;
                }
            }
            map_row++;
        }
        
        // Move to next line
        if (*line_end == '\r') line_end++;
        if (*line_end == '\n') line_end++;
        line_start = line_end;
        line_num++;
    }
}

char *move_is_valid(const char *move) {

    size_t len = strlen(move);
    
    // Remove trailing newline/carriage return if present
    char move_copy[32];
    strncpy(move_copy, move, sizeof(move_copy) - 1);
    move_copy[sizeof(move_copy) - 1] = '\0';
    
    len = strlen(move_copy);
    while (len > 0 && (move_copy[len-1] == '\n' || move_copy[len-1] == '\r')) {
        move_copy[--len] = '\0';
    }
    
    if (len < 2) {
        return NULL;
    }
    
    char row_char = move_copy[0];
    
    // Convert lowercase to uppercase
    if (row_char >= 'a' && row_char <= 'z') {
        row_char = row_char - 'a' + 'A';
    }
    
    // Check if first character is a letter
    if (row_char < 'A' || row_char > 'Z') {
        return NULL;
    }
    
    int row = row_char - 'A';
    int col = atoi(&move_copy[1]) - 1;

    if (row < 0 || row >= map.height || col < 0 || col >= map.width) {
        return NULL;
    }
    return (char *)move;
}

void game_cleanup() {
    // Free allocated resources
    for (int i = 0; i < map.height; i++) {
        free(map.cells[i]);
    }
    free(map.cells);
}

void print_map() {
    static int first_print = 1;
    
    // Get terminal size
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    int term_width = w.ws_col;
    int term_height = w.ws_row;
    
    // Calculate map width (each cell is 3 chars, plus 2 for row label, plus label column)
    int map_display_width = 2 + (map.width * 3);
    int left_padding = (term_width - map_display_width) / 2;
    if (left_padding < 0) left_padding = 0;
    
    // Calculate vertical centering
    int total_lines = 3 + map.height + 1;  // title + blank + header + rows + "Enter move"
    int top_padding = (term_height - total_lines) / 2;
    if (top_padding < 0) top_padding = 0;
    
    // If not the first print, move cursor up to overwrite previous map
    if (!first_print) {
        printf("\033[%dA", total_lines + top_padding);  // Move cursor up
        printf("\r");  // Move to start of line
        printf("\033[J");  // Clear from cursor to end of screen
    }
    first_print = 0;
    
    // Add vertical padding
    for (int i = 0; i < top_padding; i++) {
        printf("\n");
    }
    
    // Print title with padding
    printf("%*s", left_padding, "");
    if (strlen(action.action) > 0) {
        printf("Current Map State: %s\n\n", action.action);
    } else {
        printf("Current Map State:\n\n");
    }
    
    // Print column numbers with padding
    printf("%*s", left_padding, "");
    printf("  ");
    for (int j = 0; j < map.width; j++) {
        printf("%3d", j + 1);
    }
    printf("\n");
    
    // Print rows with row letters and padding
    for (int i = 0; i < map.height; i++) {
        printf("%*s", left_padding, "");
        printf("%2c ", 'A' + i);
        for (int j = 0; j < map.width; j++) {
            printf("%2c ", map.cells[i][j]);
        }
        printf("\n");
    }
    
    printf("%*s", left_padding, "");
    printf("Enter move: ");
    fflush(stdout);
}
