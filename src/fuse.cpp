/*
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

// We use FUSE 3.18, which is the latest stable release as of February 2026.
#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 18)

#include <fuse3/fuse_lowlevel.h>
#include <stdio.h>
#include <stddef.h>

/**
 * This function is called when the FUSE session is being initialized. It can be used to set up any necessary state or resources for the filesystem.
 * 
 * @param userdata The user data passed to fuse_session_new()
 * @param conn Connection information about the FUSE session
 */
static void flouds_init(void *userdata, struct fuse_conn_info *conn) {
    const char *image_path = (const char*) userdata;    
    printf("Loading filesystem from: %s\n", image_path);
}

/**
 * This function is called when the FUSE session is being destroyed, either due to unmounting or an error.
 * 
 * @param userdata The user data passed to fuse_session_new()
 */
static void flouds_destroy(void *userdata) {
}

/**
 * This function is called when a content of a directory is being looked up.
 * 
 * @param req The request handle that contains information about the lookup request and is used to send the response back to the kernel.
 * @param parent The inode number of the parent directory where the lookup is being performed.
 * @param name The name of the entry being looked up within the parent directory.
 */
static void flouds_lookup(fuse_req_t req, fuse_ino_t parent, const char *name) {
}

/**
 * This function is called when the attributes of a file or directory are being requested.
 * 
 * @param req The request handle that contains information about the getattr request and is used to send the response back to the kernel.
 * @param ino The inode number of the file or directory whose attributes are being requested.
 * @param fi Internal file information.
 */
static void flouds_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
}

/**
 * This function is called when a file is being opened.
 * 
 * @param req The request handle that contains information about the open request and is used to send the response back to the kernel.
 * @param ino The inode number of the file being opened.
 * @param fi Internal file information that can be used to store state about the open file.
 */
static void flouds_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
}

/**
 * This function is called when the contents of a file are being read.
 * 
 * @param req The request handle that contains information about the read request and is used to send the response back to the kernel.
 * @param ino The inode number of the file being read.
 * @param size The size of the buffer provided for reading the file contents.
 * @param off The offset within the file from which to start reading.
 * @param fi Internal file information that can be used to store state about the open file.
 */
static void flouds_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi) {
}

/**
 * This function is called when the contents of a directory are being read.
 * 
 * @param req The request handle that contains information about the readdir request and is used to send the response back to the kernel.
 * @param ino The inode number of the directory whose contents are being read.
 * @param size The size of the buffer provided for reading the directory entries.
 * @param off The offset within the directory entries from which to start reading.
 * @param fi Internal file information.
 */
static void flouds_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi) {
}

// This structure defines the operation that our FUSE filesystem supports.
static const struct fuse_lowlevel_ops flouds_operations = {
    .init = flouds_init,
    .destroy = flouds_destroy,
    .lookup = flouds_lookup,
    .getattr = flouds_getattr,
    .open = flouds_open,
    .read = flouds_read,
    .readdir = flouds_readdir
};

/**
 * This is the main entry point of the FUSE filesystem. The main loop is started here.
 * The initialization is analogous to this default FUSE example: https://libfuse.github.io/doxygen/example_2hello__ll_8c.html
 */
int main(int argc, char *argv[]) {
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct fuse_session *se;
    struct fuse_cmdline_opts opts;
    struct fuse_loop_config *config;
    const char *image_path = NULL;
    int ret = -1;

    // Extract the image path (first non-option argument) before parsing cmdline
    for (int i = 1; i < args.argc; i++) {
        if (args.argv[i][0] != '-') {
            image_path = args.argv[i];
            // Remove this argument from args so fuse_parse_cmdline gets the mountpoint
            for (int j = i; j < args.argc - 1; j++) {
                args.argv[j] = args.argv[j + 1];
            }
            args.argc--;
            break;
        }
    }

    // Parse the command line arguments. This will fill the opts structure with the relevant options for the FUSE session.
    if (fuse_parse_cmdline(&args, &opts) != 0) {
        return 1;
    }

    if (opts.show_help) {
        // If the user requested help information, print it and exit.
        printf("usage: %s [options] <image> <mountpoint>\n\n", argv[0]);
        fuse_cmdline_help();
        fuse_lowlevel_help();
        ret = 0;
        goto cleanup_args;
    } else if (opts.show_version) {
        // If the user requested version information, print it and exit.
        fuse_lowlevel_version();
        ret = 0;
        goto cleanup_args;
    }

    if (image_path == NULL || opts.mountpoint == NULL) {
        // If image or mount point was not provided, print usage information and exit.
        printf("usage: %s [options] <image> <mountpoint>\n", argv[0]);
        printf("       %s --help\n", argv[0]);
        ret = 1;
        goto cleanup_args;
    }

    // Create a new FUSE session with the parsed arguments and the defined operations.
    // Pass image_path as userdata so it is available in flouds_init
    se = fuse_session_new(&args, &flouds_operations, sizeof(flouds_operations), image_path);

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