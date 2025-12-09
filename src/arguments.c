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
                            if(settings.map_width <= 0 || settings.map_width > 26) {
                                printf("Map width must be greater than 0 and less than or equal to 26, exiting...\n");
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
                            if(settings.map_height <= 0 || settings.map_height > 26) {
                                printf("Map height must be greater than 0 and less than or equal to 26, exiting...\n");
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

    // Before starting, allow interactive configuration (overrides args)
    interactive_configure();
}

void print_help(){
    printf("Help for BLP example\n");
    printf("Command line parameters:\n");
    printf("   -h                           : Print help\n");
    printf("   -i file                      : Read from file\n");
    printf("   -s dev=devf,speed=baud_rate  : Set serial port device and speed\n");
    printf("   -g x=map_width,y=map_height,ship_cnt_1=5,ship_cnt_2=4,ship_cnt_3=3: Set game parameters\n");
}

// Helper to read a line from stdin with a prompt and default
static void prompt_line(const char *prompt, char *out, size_t out_sz, const char *def) {
    if (def && def[0] != '\0') {
        printf("%s [%s]: ", prompt, def);
    } else {
        printf("%s: ", prompt);
    }
    fflush(stdout);
    if (fgets(out, out_sz, stdin) == NULL) {
        // EOF or error, keep default if provided
        if (def) {
            strncpy(out, def, out_sz-1);
            out[out_sz-1] = '\0';
        } else {
            out[0] = '\0';
        }
        return;
    }
    // strip newline
    size_t l = strlen(out);
    if (l > 0 && out[l-1] == '\n') out[l-1] = '\0';
    if (out[0] == '\0' && def) {
        strncpy(out, def, out_sz-1);
        out[out_sz-1] = '\0';
    }
}

void interactive_configure() {
    char buf[CFGSTR_SIZE];
    char numbuf[32];

    printf("\n--- Interactive Game Configuration ---\n");
    printf("Press Enter to accept the value in [] or type a new one.\n\n");

    // Serial device
    prompt_line("Serial device (dev)", buf, sizeof(buf), g_serfile[0] != '\0' ? g_serfile : "");
    if (strlen(buf) > 0) {
        strncpy(g_serfile, buf, CFGSTR_SIZE-1);
        g_serfile[CFGSTR_SIZE-1] = '\0';
    }

    // Serial speed
    snprintf(numbuf, sizeof(numbuf), "%d", g_serspeed);
    prompt_line("Serial speed (baud)", buf, sizeof(buf), g_serspeed != 0 ? numbuf : "9600");
    if (strlen(buf) > 0) {
        g_serspeed = atoi(buf);
    }

    // Map width
    snprintf(numbuf, sizeof(numbuf), "%d", settings.map_width);
    prompt_line("Map width (x) - 1..26", buf, sizeof(buf), settings.map_width > 0 ? numbuf : "10");
    int val = atoi(buf);
    if (val <= 0 || val > 26) {
        printf("Invalid width, using default 10\n");
        settings.map_width = settings.map_width > 0 ? settings.map_width : 10;
    } else {
        settings.map_width = val;
    }

    // Map height
    snprintf(numbuf, sizeof(numbuf), "%d", settings.map_height);
    prompt_line("Map height (y) - 1..26", buf, sizeof(buf), settings.map_height > 0 ? numbuf : "10");
    val = atoi(buf);
    if (val <= 0 || val > 26) {
        printf("Invalid height, using default 10\n");
        settings.map_height = settings.map_height > 0 ? settings.map_height : 10;
    } else {
        settings.map_height = val;
    }

    // Ship counts
    snprintf(numbuf, sizeof(numbuf), "%d", settings.ship_count_1);
    prompt_line("Ship count size 1 (ship_cnt_1)", buf, sizeof(buf), settings.ship_count_1 >= 0 ? numbuf : "5");
    val = atoi(buf);
    if (val < 0) val = 0;
    settings.ship_count_1 = val;

    snprintf(numbuf, sizeof(numbuf), "%d", settings.ship_count_2);
    prompt_line("Ship count size 2 (ship_cnt_2)", buf, sizeof(buf), settings.ship_count_2 >= 0 ? numbuf : "4");
    val = atoi(buf);
    if (val < 0) val = 0;
    settings.ship_count_2 = val;

    snprintf(numbuf, sizeof(numbuf), "%d", settings.ship_count_3);
    prompt_line("Ship count size 3 (ship_cnt_3)", buf, sizeof(buf), settings.ship_count_3 >= 0 ? numbuf : "3");
    val = atoi(buf);
    if (val < 0) val = 0;
    settings.ship_count_3 = val;

    printf("\nConfiguration complete. Map: %dx%d, ships: %d x1, %d x2, %d x3\n",
           settings.map_width, settings.map_height, settings.ship_count_1, settings.ship_count_2, settings.ship_count_3);
    printf("Serial: %s @ %d\n\n", g_serfile[0] != '\0' ? g_serfile : "(none)", g_serspeed);
}
