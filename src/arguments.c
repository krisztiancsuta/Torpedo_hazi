#include "arguments.h"

extern Game_Settings settings;

char *const token[] = {
    [DEVICE_OPT] = "dev",
    [SPEED_OPT] = "speed",
    [MAP_WIDTH] = "x",
    [MAP_HEIGHT] = "y",
    [SHIP_COUNT_1] = "ship_cnt_1",
    [SHIP_COUNT_2] = "ship_cnt_2",
    [SHIP_COUNT_3] = "ship_cnt_3",
    NULL
};

void arguments_handle(int argc, char **argv){
    int opt;
    char *subopts;
    char *value;
    int errfnd = 0;

#if DEBUG
    printf("Number of arguments: %d\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("%d: %s \n", i, argv[i]);
    }
#endif

    while((opt = getopt(argc, argv, "hi:s:g:?")) != -1) {
        switch(opt) {
            case 'h':
                print_help();
                break;
            case '?':
                print_help();
                break;
            case 'i':
#if DEBUG
                printf("Cmd line par -i received\n");
#endif
                if(strlen(optarg) < CFGSTR_SIZE) {
                    strcpy(g_infile, optarg);
#if DEBUG
                    printf("Input file: %s\n", g_infile);
#endif
                }
                else {
                    printf("Input file name is to long (max size is %d), exiting...\n",CFGSTR_SIZE);
                    exit(EXIT_FAILURE);
                }
                break;
            case 's':
#if DEBUG
                printf("-s is handled with subopt\n");
#endif
                subopts = optarg;
                while((*subopts != '\0') && !errfnd) {
                    switch(getsubopt(&subopts, token, &value)) {
                        case DEVICE_OPT:
                            if(strlen(value) < CFGSTR_SIZE) {
                                strcpy(g_serfile, value);
#if DEBUG
                                printf("Serial device: %s\n", g_serfile);
#endif
                            }
                            else {
                                printf("Serial file name is to long (max size is %d), exiting...\n",CFGSTR_SIZE);
                                exit(EXIT_FAILURE);
                            }
                            break;
                        case SPEED_OPT:
                            g_serspeed = atoi(value);
#if DEBUG
                            printf("Serial speed: %d\n", g_serspeed);
#endif
                            break;
                    }
                }
                break;
            case 'g':
#if DEBUG
                printf("-g is handled with subopt\n");
#endif
                subopts = optarg;
                while((*subopts != '\0') && !errfnd) {
                    switch(getsubopt(&subopts, token, &value)) {
                        case MAP_WIDTH:
                            settings.map_width = atoi(value);
                            if(settings.map_width <= 0) {
                                printf("Map width must be greater than 0, exiting...\n");
                                exit(EXIT_FAILURE);
                            }
#if DEBUG
                            else {
                                printf("Map width: %d\n", settings.map_width);
                            }
#endif
                            break;
                        case MAP_HEIGHT:
                            settings.map_height = atoi(value);
                            if(settings.map_height <= 0) {
                                printf("Map height must be greater than 0, exiting...\n");
                                exit(EXIT_FAILURE);
                            }
#if DEBUG
                            else {
                                printf("Map height: %d\n", settings.map_height);
                            }
#endif
                            break;
                        case SHIP_COUNT_1:
                            settings.ship_count_1 = atoi(value);
#if DEBUG
                            printf("Ship count 1: %d\n", settings.ship_count_1);
#endif
                            break;
                        case SHIP_COUNT_2:
                            settings.ship_count_2 = atoi(value);
#if DEBUG
                            printf("Ship count 2: %d\n", settings.ship_count_2);
#endif
                            break;
                        case SHIP_COUNT_3:
                            settings.ship_count_3 = atoi(value);
#if DEBUG
                            printf("Ship count 3: %d\n", settings.ship_count_3);
#endif
                            break;
                    }
                }
                break;
            default:
                printf("Unknown cmd line parameter is received\n");
                break;
        }
    }
}

void print_help(){
    printf("Help for BLP example\n");
    printf("Command line parameters:\n");
    printf("   -h                           : Print help\n");
    printf("   -i file                      : Read from file\n");
    printf("   -s dev=devf,speed=baud_rate  : Set serial port device and speed\n");
    printf("   -g x=map_width,y=map_height,ship_cnt_1=5,ship_cnt_2=4,ship_cnt_3=3: Set game parameters\n");
}
