/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

// We use FUSE 3.18, which is the latest stable release as of February 2026.
// If this causes issues, try lowering to 3.10 or 3.2
#define FUSE_USE_VERSION 31

#include <fuse3/fuse_lowlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

// FileSystemManager* fsm = nullptr;

/**
 * This function is called when the FUSE session is being initialized. It can be used to set up any necessary state or resources for the filesystem.
 * 
 * @param userdata The user data passed to fuse_session_new()
 * @param conn Connection information about the FUSE session
 */
static void flouds_init(void *userdata, struct fuse_conn_info *conn) {
    const char* image_path = (const char*) userdata;    
    printf("Loading filesystem from: %s\n", image_path);
}

/**
 * This function is called when the FUSE session is being destroyed, either due to unmounting or an error.
 * 
 * @param userdata The user data passed to fuse_session_new()
 */
static void flouds_destroy(void *userdata) {
    // Cleanup code will go here
}

/**
 * This function is called when a content of a directory is being looked up.
 * 
 * @param req The request handle that contains information about the lookup request and is used to send the response back to the kernel.
 * @param parent The inode number of the parent directory where the lookup is being performed.
 * @param name The name of the entry being looked up within the parent directory.
 */
static void flouds_lookup(fuse_req_t req, fuse_ino_t parent, const char *name) {
    fuse_reply_err(req, ENOSYS);
}

/**
 * This function is called when the attributes of a file or directory are being requested.
 * 
 * @param req The request handle that contains information about the getattr request and is used to send the response back to the kernel.
 * @param ino The inode number of the file or directory whose attributes are being requested.
 * @param fi Internal file information.
 */
static void flouds_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
    fuse_reply_err(req, ENOSYS);
}

/**
 * This function is called when a file is being opened.
 * 
 * @param req The request handle that contains information about the open request and is used to send the response back to the kernel.
 * @param ino The inode number of the file being opened.
 * @param fi Internal file information that can be used to store state about the open file.
 */
static void flouds_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
    fuse_reply_err(req, ENOSYS);
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
    fuse_reply_err(req, ENOSYS);
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
    fuse_reply_err(req, ENOSYS);
}

// This structure defines the operation that our FUSE filesystem supports.
static struct fuse_lowlevel_ops flouds_operations;

/**
 * This is the main entry point of the FUSE filesystem. The main loop is started here.
 * The initialization is analogous to this default FUSE example: https://libfuse.github.io/doxygen/example_2hello__ll_8c.html
 */
int main(int argc, char *argv[]) {
    printf("1");
    fflush(stdout);

    // Initialize the operations structure
    memset(&flouds_operations, 0, sizeof(flouds_operations));
    flouds_operations.init = flouds_init;
    flouds_operations.destroy = flouds_destroy;
    flouds_operations.lookup = flouds_lookup;
    flouds_operations.getattr = flouds_getattr;
    flouds_operations.open = flouds_open;
    flouds_operations.read = flouds_read;
    flouds_operations.readdir = flouds_readdir;

    printf("2\n");
    fflush(stdout);

    printf("3: Initializing fuse_args\n");
    fflush(stdout);
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    printf("4: fuse_args initialized\n");
    fflush(stdout);
    
    struct fuse_session *se = NULL;
    struct fuse_cmdline_opts opts;
    struct fuse_loop_config *config = NULL;
    const char *image_path = NULL;
    int ret = -1;

    memset(&opts, 0, sizeof(opts));

    printf("5: Starting arg extraction\n");
    fflush(stdout);
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

    printf("6: Calling fuse_parse_cmdline\n");
    fflush(stdout);
    // Parse the command line arguments. This will fill the opts structure with the relevant options for the FUSE session.
    if (fuse_parse_cmdline(&args, &opts) != 0) {
        return 1;
    }
    printf("7: fuse_parse_cmdline succeeded\n");
    fflush(stdout);

    printf("7.1: FUSE version check\n");
    printf("Compiled with FUSE version: %d\n", FUSE_USE_VERSION);
    fflush(stdout);

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

    printf("8: Creating fuse session (ops size=%zu)\n", sizeof(flouds_operations));
    fflush(stdout);
    // Create a new FUSE session with the parsed arguments and the defined operations.
    // Pass image_path as userdata so it is available in flouds_init
    se = fuse_session_new(&args, &flouds_operations, sizeof(flouds_operations), (void*) image_path);

    printf("9: fuse_session_new returned\n");
    fflush(stdout);
    
    if(se == NULL) {
        printf("ERROR: Session is NULL\n");
        // Session could not be created
        goto cleanup_args;
    }
    
    printf("10: Setting signal handlers\n");
    fflush(stdout);

    if (fuse_set_signal_handlers(se) != 0) {
        printf("ERROR: Failed to set signal handlers\n");
        // Failed to set signal handlers
        goto cleanup_session;
    }

    printf("11: Mounting filesystem\n");
    fflush(stdout);
    
    // Check if mountpoint exists and is a directory
    struct stat st;
    if (stat(opts.mountpoint, &st) != 0) {
        printf("ERROR: Mountpoint '%s' does not exist: %s\n", opts.mountpoint, strerror(errno));
        goto cleanup_signal_handlers;
    }
    if (!S_ISDIR(st.st_mode)) {
        printf("ERROR: Mountpoint '%s' is not a directory\n", opts.mountpoint);
        goto cleanup_signal_handlers;
    }
    
    printf("11.5: Mountpoint '%s' exists and is a directory\n", opts.mountpoint);
    fflush(stdout);
    
    printf("11.6: About to call fuse_session_mount (se=%p, mountpoint='%s')\n", (void*)se, opts.mountpoint);
    fflush(stdout);
    
    printf("11.7: Forcing foreground mode to debug\n");
    opts.foreground = 1;
    opts.singlethread = 1;
    fflush(stdout);
    
    int mount_ret = fuse_session_mount(se, opts.mountpoint);
    printf("11.8: fuse_session_mount returned %d\n", mount_ret);
    fflush(stdout);
    
    if (mount_ret != 0) {
        printf("ERROR: fuse_session_mount failed: %s\n", strerror(errno));
        // Failed to mount the filesystem
        goto cleanup_signal_handlers;
    }

    printf("12: Starting main loop\n");
    fflush(stdout);
    
    if (opts.singlethread) {
        ret = fuse_session_loop(se);
    } else {
        config = fuse_loop_cfg_create();
        if (config == NULL) {
            printf("ERROR: Failed to create loop config\n");
            goto cleanup_unmount;
        }
        fuse_loop_cfg_set_clone_fd(config, opts.clone_fd);
        fuse_loop_cfg_set_max_threads(config, opts.max_threads);
        ret = fuse_session_loop_mt(se, config);
        fuse_loop_cfg_destroy(config);
    }

cleanup_unmount:
    printf("DEBUG: cleanup_unmount\n");
    fflush(stdout);
    if (se) fuse_session_unmount(se);
cleanup_signal_handlers:
    printf("DEBUG: cleanup_signal_handlers\n");
    fflush(stdout);
    if (se) fuse_remove_signal_handlers(se);
cleanup_session:
    printf("DEBUG: cleanup_session\n");
    fflush(stdout);
    if (se) fuse_session_destroy(se);
cleanup_args:
    printf("DEBUG: cleanup_args\n");
    fflush(stdout);
    if (opts.mountpoint) free(opts.mountpoint);
    fuse_opt_free_args(&args);

    printf("DEBUG: Exiting\n");
    fflush(stdout);
    return ret ? 1 : 0;
}