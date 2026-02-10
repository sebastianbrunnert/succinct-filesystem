/*
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

// We use FUSE 3.18, which is the latest stable release as of February 2026.
#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 18)

#include <fuse3/fuse_lowlevel.h>

/*
 * This function is called when libfuse establishes a connection with the kernel.
 * 
 * Parameters:
 * - userdata: A pointer to user-defined data that can be passed to the FUSE operations
 * - conn: A pointer to a structure containing information about the connection
 */
static void *fuse_init(void *userdata, struct fuse_conn_info *conn) {
    return NULL;
}

// This structure defines the operation that our FUSE filesystem supports.
static const struct fuse_low_level_ops fuse_operations = {
    .init = fuse_init
};

/*
 * This is the main entry point of the FUSE filesystem. The main loop is started here.
 * The initialization is analogous to this default FUSE example: https://libfuse.github.io/doxygen/example_2hello__ll_8c.html
 */
int main(int argc, char *argv[]) {
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct fuse_session *se;
    struct fuse_cmdline_opts opts;
    struct fuse_loop_config *config;
    int ret = -1;

    // Parse the command line arguments. This will fill the opts structure with the relevant options for the FUSE session.
    if (fuse_parse_cmdline(&args, &opts) != 0) {
        return 1;
    }

    if (opts.show_help) {
        // If the user requested help information, print it and exit.
        printf("usage: %s [options] <mountpoint>\n\n", argv[0]);
        fuse_cmdline_help();
        fuse_lowlevel_help();
        ret = 0;
        goto cleanup_args;
    } else if (opts.show_version) {
        // If the user requested version information, print it and exit.
        fuse_cmdline_version();
        ret = 0;
        goto cleanup_args;
    }

    if (opts.mountpoint == NULL) {
        // If no mount point was provided, print usage information and exit.
        printf("usage: %s [options] <mountpoint>\n", argv[0]);
        printf("       %s --help\n", argv[0]);
        ret = 1;
        goto cleanup_args;
    }

    // Create a new FUSE session with the parsed arguments and the defined operations.
    se = fuse_session_new(&args, &fuse_operations, sizeof(fuse_operations), NULL);

    if(se == NULL) {
        // Session could not be created
        goto cleanup_args;
    }

    if (fuse_set_signal_handlers(se) != 0) {
        // Failed to set signal handlers
        goto cleanup_session;
    }

    if (fuse_session_mount(se, opts.mountpoint) != 0) {
        // Failed to mount the filesystem
        goto cleanup_signal_handlers;
    }

    // Daemonize the process if the --foreground option is not set.
    fuse_daemonize(opts.foreground);

    // Block until ctrl-c or fusermount -u is issued.
    if (opts.singlethread) {
        ret = fuse_session_loop(se);
    } else {
        config = fuse_loop_cfg_create();
        fuse_loop_cfg_set_clone_fd(config, opts.clone_fd);
        fuse_loop_cfg_set_max_threads(config, opts.max_threads);
        ret = fuse_session_loop_mt(se, config);
        fuse_loop_cfg_destroy(config);
        config = NULL;
    }

    fuse_session_unmount(se);
cleanup_signal_handlers:
    fuse_remove_signal_handlers(se);
cleanup_session:
    fuse_session_destroy(se);
cleanup_args:
    fuse_opt_free_args(&args);

    return ret ? 1 : 0;
}