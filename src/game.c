#include "game.h"
#include <sys/ioctl.h>
#include <stdbool.h>
#include <unistd.h>
#include "arguments.h"
Game_Settings settings;
Map map;
Game_Actions move_result;


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
    
    // Receive response and map from Host (no action for initial setup)
    if (receive_host_data(fd, NULL) != 0) {
        printf("Error: Failed to receive data from Host\n");
        return -1;
    }

    print_map();

    return 0; // Success
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
    if (strlen(move_result.action) > 0) {
        printf("Current Map State: %s\n\n", move_result.action);
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

void game_loop(int fd) {   
    // Main game loop
    move_result.is_game_over = 0;
    move_result.action[0] = '\0';

    char input[256];
    while (1) {
        // Get user input
        if (fgets(input, sizeof(input), stdin) != NULL) {
            // Remove newline
            input[strcspn(input, "\n")] = '\0';
            
            // Check for quit command
            if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
                printf("Exiting game...\n");
                break;
            }
            
            // Send move to host
            strcat(input, "\n");
            write(fd, input, strlen(input));
            
            // Receive response with action
            if (receive_host_data(fd, &move_result) != 0) {
                printf("Error: Failed to receive response from host\n");
                break;
            }
            
            // Update and print map (action will be displayed in print_map)
            print_map();
            
            // Check if game is over
            if (move_result.is_game_over) {
                printf("\n=== GAME OVER ===\n");
                printf("You destroyed all ships!\n");
                
                break;
            }
        } else {
            // Handle EOF or error
            break;
        }
    }

}

int receive_host_data(int fd, Game_Actions *game_actions) {
    // Calculate expected bytes for complete response
    // Could be: "OK\r\nMAP\r\n..." for initial setup
    // Or: "HIT!\r\nMAP\r\n..." or "MISS!\r\nMAP\r\n..." for game moves
    
    int bytes_per_row = (2 * map.width) + 2;  // Each cell is "x " (2 chars), plus "\r\n"
    int base_size = 50 + (bytes_per_row * map.height) + 10;  // Response line + MAP + rows + END
    
    char *buffer = (char *)malloc(base_size + 100); // Extra buffer for safety
    if (buffer == NULL) {
        printf("Memory allocation failed for receive buffer\n");
        return -1;
    }
    
    int total_read = 0;
    int bytes_read;
    int max_attempts = 50;  // Maximum read attempts
    int attempts = 0;
    
#if DEBUG
    printf("Starting to receive data from host...\n");
#endif
    
    // Read data in chunks until we have enough
    while (attempts < max_attempts) {
        bytes_read = read(fd, buffer + total_read, base_size - total_read);
        if (bytes_read > 0) {
            total_read += bytes_read;
            buffer[total_read] = '\0';
            
            // Check if we have received "END" marker
            if (strstr(buffer, "END") != NULL) {
                break;  // We have all the data
            }
        } else if (bytes_read < 0) {
            perror("Error reading from serial");
            free(buffer);
            return -1;
        }
        
        attempts++;
        usleep(20000); // 20ms delay between reads
    }
    
    buffer[total_read] = '\0';
    
#if DEBUG
    printf("Received %d bytes in %d attempts\n", total_read, attempts);
    printf("Raw data:\n%s\n---END RAW---\n", buffer);
#endif
    
    // Parse the received data
    char *ptr = buffer;
    char *line_start;
    char *line_end;
    
    // 1. Read first line - could be "OK", "HIT!", "MISS!", "INVALID!", "ALREADY TRIED!", or "GAME OVER"
    line_start = ptr;
    line_end = strchr(line_start, '\n');
    if (line_end == NULL) {
        printf("Error: Malformed response (no first line)\n");
        free(buffer);
        return -1;
    }
    *line_end = '\0';
    
    // Trim line
    while (*line_start == ' ' || *line_start == '\r') line_start++;
    char *trim_end = line_end - 1;
    while (trim_end > line_start && (*trim_end == ' ' || *trim_end == '\r' || *trim_end == '!')) {
        if (*trim_end != '!') *trim_end = '\0';
        trim_end--;
    }
    
#if DEBUG
    printf("First line: '%s'\n", line_start);
#endif
    
    // Check if first line is already "MAP" (no action line sent)
    bool map_on_first_line = false;
    if (strcmp(line_start, "MAP") == 0) {
        map_on_first_line = true;
        // Store empty action or default message
        if (game_actions != NULL) {
            strcpy(game_actions->action, "");
            game_actions->is_game_over = 0;
        }
#ifdef DEBUG
        printf("MAP found on first line, no action\n");
#endif
    } else {
        // Store action if provided
        if (game_actions != NULL) {
            strncpy(game_actions->action, line_start, sizeof(game_actions->action) - 1);
            game_actions->action[sizeof(game_actions->action) - 1] = '\0';
            game_actions->is_game_over = 0;
            
            // Check if game is over
            if (strcmp(line_start, "GAME OVER") == 0) {
                game_actions->is_game_over = 1;
            }
            
#ifdef DEBUG
            printf("Action stored: '%s', Game Over: %d\n", game_actions->action, game_actions->is_game_over);
#endif
        }
        
        // Validate first line (OK for setup, or action for game move)
        if (strcmp(line_start, "OK") != 0 && 
            strcmp(line_start, "HIT!") != 0 && 
            strcmp(line_start, "MISS!") != 0 && 
            strcmp(line_start, "INVALID!") != 0 && 
            strcmp(line_start, "ALREADY TRIED!") != 0 &&
            strcmp(line_start, "GAME OVER") != 0) {
            printf("Error: Unexpected response '%s'\n", line_start);
            free(buffer);
            return -1;
        }
    }
    
    ptr = line_end + 1;
    
    // 2. Check for "MAP" (skip if already found on first line)
    if (map_on_first_line) {
        // Already at MAP, continue to parse rows
#ifdef DEBUG
        printf("Skipping MAP line check (already on first line)\n");
#endif
    } else {
        // Need to find MAP line
        line_start = ptr;
        line_end = strchr(line_start, '\n');
        if (line_end == NULL) {
            printf("Error: Malformed response (no MAP line)\n");
            free(buffer);
            return -1;
        }
        *line_end = '\0';
        
        while (*line_start == ' ' || *line_start == '\r') line_start++;
        trim_end = line_end - 1;
        while (trim_end > line_start && (*trim_end == ' ' || *trim_end == '\r')) {
            *trim_end = '\0';
            trim_end--;
        }
        
        if (strcmp(line_start, "MAP") != 0) {
            printf("Error: Expected MAP, got '%s'\n", line_start);
            free(buffer);
            return -1;
        }
        
#ifdef DEBUG
        printf("Received MAP marker\n");
#endif
        
        ptr = line_end + 1;
    }
    
    // 3. Parse map rows
    for (int row = 0; row < map.height; row++) {
        line_start = ptr;
        line_end = strchr(line_start, '\n');
        if (line_end == NULL) {
            printf("Error: Malformed map row %d\n", row);
            free(buffer);
            return -1;
        }
        *line_end = '\0';
        
        // Parse row data
        int col = 0;
        char *token = line_start;
        while (*token != '\0' && col < map.width) {
            if (*token != ' ' && *token != '\r') {
                map.cells[row][col] = *token;
                col++;
            }
            token++;
        }
        
#if DEBUG
        printf("Parsed row %d: %d cells\n", row, col);
#endif
        
        ptr = line_end + 1;
    }
    
    // 4. Check for "END"
    line_start = ptr;
    line_end = strchr(line_start, '\n');
    if (line_end != NULL) {
        *line_end = '\0';
    }
    
    while (*line_start == ' ' || *line_start == '\r') line_start++;
    if (line_end) {
        trim_end = line_end - 1;
        while (trim_end > line_start && (*trim_end == ' ' || *trim_end == '\r')) {
            *trim_end = '\0';
            trim_end--;
        }
    }
    
    if (strcmp(line_start, "END") != 0) {
        printf("Warning: Expected END, got '%s'\n", line_start);
    }
    
#if DEBUG
    printf("Map reception completed\n");
#endif
    
    free(buffer);
    return 0;
}