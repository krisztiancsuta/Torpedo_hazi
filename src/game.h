#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


typedef struct Game_Settings {
    int map_width;
    int map_height;
    int ship_count_1;
    int ship_count_2;
    int ship_count_3;
} Game_Settings;

typedef struct Map {
    int width;
    int height;
    int **cells; // 2D array representing the map
} Map;

typedef struct Game_Actions{
    char action[32];      // "HIT", "MISS", "INVALID", "ALREADY TRIED", "GAME OVER"
    int is_game_over;     // 1 if game is over, 0 otherwise
} Game_Actions;


int game_init(int fd);
void game_loop(int fd);
void game_cleanup();
void print_map();
int receive_host_data(int fd, Game_Actions *actions);
#endif
