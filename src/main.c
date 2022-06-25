// SPDX-License-Identifier: MIT
// Copyright (C) 2020 Artem Senichev <artemsen@gmail.com>

#include "buildcfg.h"
#include "config.h"
#include "image.h"
#include "viewer.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// clang-format off
/** Command line options. */
static const struct option options[] = {
    { "fullscreen", no_argument,       NULL, 'f' },
    { "scale",      required_argument, NULL, 's' },
    { "background", required_argument, NULL, 'b' },
    { "geometry",   required_argument, NULL, 'g' },
    { "info",       no_argument,       NULL, 'i' },
    { "class",      required_argument, NULL, 'c' },
    { "no-sway",    no_argument,       NULL, 'n' },
    { "version",    no_argument,       NULL, 'v' },
    { "help",       no_argument,       NULL, 'h' },
    { NULL,         0,                 NULL,  0  }
};
// clang-format on

/**
 * Print help usage info.
 */
static void print_help(void)
{
    // clang-format off
    puts("Usage: " APP_NAME " [OPTION...] [FILE...]");
    puts("  -f, --fullscreen         Full screen mode");
    puts("  -s, --scale=TYPE         Set initial image scale: default, fit, or real");
    puts("  -b, --background=XXXXXX  Set background color as hex RGB");
    puts("  -g, --geometry=X,Y,W,H   Set window geometry");
    puts("  -i, --info               Show image properties");
    puts("  -c, --class              Set window class/app_id");
    puts("  -n, --no-sway            Disable integration with Sway WM");
    puts("  -v, --version            Print version info and exit");
    puts("  -h, --help               Print this help and exit");
    // clang-format on
}

/**
 * Parse command line options into configuration instance.
 * @param[in] argc number of arguments to parse
 * @param[in] argv arguments array
 * @param[out] cfg target configuration instance
 * @return index of the first non option argument, or -1 if error, or 0 to exit
 */
static int parse_cmdline(int argc, char* argv[], config_t* cfg)
{
    char short_opts[(sizeof(options) / sizeof(options[0])) * 2];
    char* short_opts_ptr = &short_opts[0];
    int opt;

    // compose short options string
    for (size_t i = 0; i < sizeof(options) / sizeof(options[0]); ++i) {
        const struct option* opt = &options[i];
        if (opt->val) {
            *short_opts_ptr++ = opt->val;
            if (opt->has_arg != no_argument) {
                *short_opts_ptr++ = ':';
            }
        }
    }
    *short_opts_ptr = 0; // last null

    opterr = 0; // prevent native error messages

    // parse arguments
    while ((opt = getopt_long(argc, argv, short_opts, options, NULL)) != -1) {
        switch (opt) {
            case 'f':
                cfg->fullscreen = true;
                cfg->sway_wm = false;
                break;
            case 's':
                if (!set_scale_config(cfg, optarg)) {
                    return -1;
                }
                break;
            case 'b':
                if (!set_background_config(cfg, optarg)) {
                    return -1;
                }
                break;
            case 'g':
                if (!set_geometry_config(cfg, optarg)) {
                    return -1;
                }
                break;
            case 'i':
                cfg->show_info = true;
                break;
            case 'c':
                if (!set_appid_config(cfg, optarg)) {
                    return -1;
                }
                break;
            case 'n':
                cfg->sway_wm = false;
                break;
            case 'v':
                puts(APP_NAME " version " APP_VERSION ".");
                printf("Supported formats: %s.\n", supported_formats());
                return 0;
            case 'h':
                print_help();
                return 0;
            default:
                fprintf(stderr, "Invalid argument: %s\n", argv[optind - 1]);
                return -1;
        }
    }

    check_config(cfg);

    return optind;
}

/**
 * Application entry point.
 */
int main(int argc, char* argv[])
{
    int rc;
    config_t* cfg = NULL;
    file_list_t* files = NULL;
    bool recursive = true;
    int num_files;
    int index;

    cfg = init_config();
    if (!cfg) {
        rc = EXIT_FAILURE;
        goto done;
    }

    index = parse_cmdline(argc, argv, cfg);
    if (index == 0) {
        rc = EXIT_SUCCESS;
        goto done;
    }
    if (index < 0) {
        rc = EXIT_FAILURE;
        goto done;
    }

    num_files = argc - index;
    if (num_files == 0) {
        // not input files specified, use current directory
        const char* curr_dir = ".";
        files = init_file_list(&curr_dir, 1, recursive);
        if (!files) {
            fprintf(stderr, "No image files found in the current directory\n");
            rc = EXIT_FAILURE;
            goto done;
        }
    } else if (num_files == 1 && strcmp(argv[index], "-") == 0) {
        // reading from pipe
        files = NULL;
    } else {
        files = init_file_list((const char**)&argv[index], (size_t)(num_files),
                               recursive);
        if (!files) {
            fprintf(stderr, "Unable to compose file list from input args\n");
            rc = EXIT_FAILURE;
            goto done;
        }
    }

    rc = run_viewer(cfg, files) ? EXIT_SUCCESS : EXIT_FAILURE;

done:
    free_config(cfg);
    free_file_list(files);

    return rc;
}
