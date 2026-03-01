/**
 * This file is part of the Succinct Filesystem project.
 * 
 * Copyright (c) 2026 Sebastian Brunnert <mail@sebastianbrunnert.de>
 * SPDX-License-Identifier: GPL-2.0-only
 */

// We use FUSE 3.17, which is the latest stable release as of February 2026.
#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 17)

#include <fuse3/fuse_lowlevel.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "fsm/file_system_manager.hpp"

FileSystemManager* file_system_manager = nullptr;

/**
 * This function is called when the FUSE session is being initialized. It can be used to set up any necessary state or resources for the filesystem.
 * 
 * @param userdata The user data passed to fuse_session_new()
 * @param conn Connection information about the FUSE session
 */
static void flouds_init(void *userdata, struct fuse_conn_info *conn) {
    const char *image_path = (const char*) userdata;    

    file_system_manager = new FileSystemManager();
    file_system_manager->mount(image_path);
}

/**
 * This function is called when the FUSE session is being destroyed, either due to unmounting or an error.
 * 
 * @param userdata The user data passed to fuse_session_new()
 */
static void flouds_destroy(void *userdata) {
    file_system_manager->unmount();
    delete file_system_manager;
}

/**
 * This function is called when a content of a directory is being looked up.
 * 
 * @param req The request handle that contains information about the lookup request and is used to send the response back to the kernel.
 * @param parent The inode number of the parent directory where the lookup is being performed.
 * @param name The name of the entry being looked up within the parent directory.
 */
static void flouds_lookup(fuse_req_t req, fuse_ino_t parent, const char *name) {
    // Convert FUSE inode to FLOUDS inode
    size_t parent_node = parent - 1;
    
    Flouds* flouds = file_system_manager->get_flouds();
    
    // Check if parent is a directory
    if (!flouds->is_folder(parent_node)) {
        fuse_reply_err(req, ENOTDIR);
        return;
    }
    
    // Search for the child with the given name
    size_t num_children = flouds->children_count(parent_node);
    for (size_t i = 0; i < num_children; i++) {
        size_t child_node = flouds->child(parent_node, i);
        if (flouds->get_name(child_node) == name) {
            // Found the child
            struct fuse_entry_param entry;
            memset(&entry, 0, sizeof(entry));
            
            entry.ino = child_node + 1;  // Convert back to FUSE inode
            entry.attr.st_ino = entry.ino;
            entry.attr.st_nlink = flouds->is_folder(child_node) ? 2 : 1;
            
            if (flouds->is_folder(child_node)) {
                entry.attr.st_mode = S_IFDIR | 0755;
            } else {
                entry.attr.st_mode = S_IFREG | 0644;
                entry.attr.st_size = file_system_manager->get_inode(child_node)->size;
            }
            
            entry.attr_timeout = 1.0;
            entry.entry_timeout = 1.0;
            
            fuse_reply_entry(req, &entry);
            return;
        }
    }
    
    // Child not found
    fuse_reply_err(req, ENOENT);
}

/**
 * This function is called when the attributes of a file or directory are being requested.
 * 
 * @param req The request handle that contains information about the getattr request and is used to send the response back to the kernel.
 * @param ino The inode number of the file or directory whose attributes are being requested.
 * @param fi Internal file information.
 */
static void flouds_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
    struct stat stbuf;
    memset(&stbuf, 0, sizeof(stbuf));
    
    Flouds* flouds = file_system_manager->get_flouds();
    stbuf.st_ino = ino;

    size_t node = ino - 1;
    Inode* inode = file_system_manager->get_inode(node);
    
    if (flouds->is_folder(node)) {
        stbuf.st_mode = S_IFDIR | inode->mode;
        stbuf.st_nlink = 2;
    } else if (flouds->is_file(node)) {
        stbuf.st_mode = S_IFREG | inode->mode;
        stbuf.st_nlink = 1;
        stbuf.st_size = inode->size;
    } else {
        fuse_reply_err(req, ENOENT);
        return;
    }

    stbuf.st_atime = inode->access_time;
    stbuf.st_mtime = inode->modification_time;
    stbuf.st_ctime = inode->creation_time;
    
    fuse_reply_attr(req, &stbuf, 1.0);
}

/**
 * This function is called when file or directory attributes need to be changed.
 * 
 * @param req The request handle that contains information about the setattr request and is used to send the response back to the kernel.
 * @param ino The inode number of the file or directory whose attributes are being changed.
 * @param attr The new attributes to set.
 * @param to_set Bitmask indicating which attributes should be changed.
 * @param fi Internal file information.
 */
static void flouds_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr, int to_set, struct fuse_file_info *fi) {    
    Flouds* flouds = file_system_manager->get_flouds();

    size_t node = ino - 1;
    Inode* inode = file_system_manager->get_inode(node);
    
    try {
        // Handle different attribute changes
        if (to_set & FUSE_SET_ATTR_MODE) {
            inode->mode = attr->st_mode;
        }
        
        if (to_set & FUSE_SET_ATTR_SIZE && flouds->is_file(node)) {
            file_system_manager->set_file_size(node, attr->st_size);
        }
        
        if (to_set & FUSE_SET_ATTR_ATIME) {
            inode->access_time = attr->st_atime;
        }
        
        if (to_set & FUSE_SET_ATTR_MTIME) {
            inode->modification_time = attr->st_mtime;
        }
        
        // Return the updated attributes
        struct stat stbuf;
        memset(&stbuf, 0, sizeof(stbuf));
        stbuf.st_ino = ino;
        
        if (flouds->is_folder(node)) {
            stbuf.st_mode = S_IFDIR | inode->mode;
            stbuf.st_nlink = 2;
        } else if (flouds->is_file(node)) {
            stbuf.st_mode = S_IFREG | inode->mode;
            stbuf.st_nlink = 1;
            stbuf.st_size = inode->size;
        }
        
        stbuf.st_atime = inode->access_time;
        stbuf.st_mtime = inode->modification_time;
        stbuf.st_ctime = inode->creation_time;
        
        fuse_reply_attr(req, &stbuf, 1.0);

        file_system_manager->save();
    } catch (...) {
        fuse_reply_err(req, EIO);
    }
}

/**
 * This function is called when a file is being opened.
 * 
 * @param req The request handle that contains information about the open request and is used to send the response back to the kernel.
 * @param ino The inode number of the file being opened.
 * @param fi Internal file information that can be used to store state about the open file.
 */
static void flouds_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
    size_t node = ino - 1;
    
    Flouds* flouds = file_system_manager->get_flouds();
    
    // Check if the node exists and is a file
    if (flouds->is_file(node)) {
        fuse_reply_open(req, fi);
    } else {
        fuse_reply_err(req, ENOENT);
    }
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
    size_t node = ino - 1;
    
    Flouds* flouds = file_system_manager->get_flouds();
    if (!flouds->is_file(node)) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    Inode* inode = file_system_manager->get_inode(node);
    
    // Don't read beyond file size
    if (off >= inode->size) {
        fuse_reply_buf(req, NULL, 0);
        return;
    }
    
    // Adjust size if reading beyond end of file
    if (off + size > inode->size) {
        size = inode->size - off;
    }

    char* buffer = new char[size];
    file_system_manager->read_file(node, buffer, size, off);
    
    fuse_reply_buf(req, buffer, size);
    delete[] buffer;
}

/**
 * This function is called when the contents of a file are being written.
 * 
 * @param req The request handle that contains information about the write request and is used to send the response back to the kernel.
 * @param ino The inode number of the file being written to.
 * @param buf The buffer containing the data to write to the file. Must be at least size bytes.
 * @param size The number of bytes to write from the buffer to the file.
 * @param off The offset within the file from which to start writing.
 * @param fi Internal file information that can be used to store state about the open file.
 */
static void flouds_write(fuse_req_t req, fuse_ino_t ino, const char *buf, size_t size, off_t off, struct fuse_file_info *fi) {
    size_t node = ino - 1;
    
    Flouds* flouds = file_system_manager->get_flouds();
    if (!flouds->is_file(node)) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    try {
        Inode* inode = file_system_manager->get_inode(node);
        file_system_manager->write_file(node, buf, size, off);
        fuse_reply_write(req, size);
        file_system_manager->save();
    } catch (...) {
        fuse_reply_err(req, EIO);
    }
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
    size_t node = ino - 1;
    
    Flouds* flouds = file_system_manager->get_flouds();
    if (!flouds->is_folder(node)) {
        fuse_reply_err(req, ENOTDIR);
        return;
    }

    struct stat stbuf;
    memset(&stbuf, 0, sizeof(stbuf));
    
    char buf[4096];
    size_t bufsize = sizeof(buf);
    char *p = buf;
    size_t rem = bufsize;
    
    off_t current_off = 0;
    
    // Add "." entry
    if (off <= current_off) {
        stbuf.st_ino = ino;
        stbuf.st_mode = S_IFDIR;
        size_t entsize = fuse_add_direntry(req, p, rem, ".", &stbuf, current_off + 1);
        if (entsize > rem) {
            fuse_reply_buf(req, buf, bufsize - rem);
            return;
        }
        p += entsize;
        rem -= entsize;
    }
    current_off++;
    
    // Add ".." entry
    if (off <= current_off) {
        stbuf.st_ino = ino;
        stbuf.st_mode = S_IFDIR;
        size_t entsize = fuse_add_direntry(req, p, rem, "..", &stbuf, current_off + 1);
        if (entsize > rem) {
            fuse_reply_buf(req, buf, bufsize - rem);
            return;
        }
        p += entsize;
        rem -= entsize;
    }
    current_off++;
    
    // Add directory entries from FLOUDS
    size_t num_children = flouds->children_count(node);
    for (size_t i = 0; i < num_children; i++) {
        if (off <= current_off) {
            size_t child_node = flouds->child(node, i);
            std::string child_name = flouds->get_name(child_node);
            
            stbuf.st_ino = child_node + 1;
            if (flouds->is_folder(child_node)) {
                stbuf.st_mode = S_IFDIR;
            } else {
                stbuf.st_mode = S_IFREG;
            }
            
            size_t entsize = fuse_add_direntry(req, p, rem, child_name.c_str(), &stbuf, current_off + 1);
            if (entsize > rem) {
                fuse_reply_buf(req, buf, bufsize - rem);
                return;
            }
            p += entsize;
            rem -= entsize;
        }
        current_off++;
    }
    
    // End of directory
    fuse_reply_buf(req, buf, bufsize - rem);
}

/**
 * This function is called when a new directory is being created.
 * 
 * @param req The request handle that contains information about the mkdir request and is used to send the response back to the kernel.
 * @param parent The inode number of the parent directory where the new directory should be created.
 * @param name The name of the new directory.
 * @param mode The permissions for the new directory.
 */
static void flouds_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode) {
    size_t parent_node = parent - 1;
    
    try {
        // Create the new directory
        size_t new_node = file_system_manager->add_node(parent_node, name, true, mode);
    
        struct fuse_entry_param entry;
        memset(&entry, 0, sizeof(entry));
        
        // Convert to FUSE inode
        entry.ino = new_node + 1;
        entry.attr.st_ino = entry.ino;
        entry.attr.st_mode = S_IFDIR | mode;
        entry.attr.st_nlink = 2;

        fuse_reply_entry(req, &entry);

        file_system_manager->save();
    } catch (...) {
        fuse_reply_err(req, EIO);
    }
}

/**
 * This function is called when a new file is being created.
 * 
 * @param req The request handle that contains information about the create request and is used to send the response back to the kernel.
 * @param parent The inode number of the parent directory where the new file should be created.
 * @param name The name of the new file.
 * @param mode The permissions for the new file.
 * @param fi File information structure that can be used to store state about the open file.
 */
static void flouds_create(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode, struct fuse_file_info *fi) {
    size_t parent_node = parent - 1;
    
    try {
        // Create the new file
        size_t new_node = file_system_manager->add_node(parent_node, name, false, mode);
        
        struct fuse_entry_param entry;
        memset(&entry, 0, sizeof(entry));

        // Convert to FUSE inode
        entry.ino = new_node + 1;
        entry.attr.st_ino = entry.ino;
        entry.attr.st_mode = S_IFREG | mode;
        entry.attr.st_nlink = 1;
        entry.attr.st_size = 0;
        
        fuse_reply_create(req, &entry, fi);

        file_system_manager->save();
    } catch (...) {
        fuse_reply_err(req, EIO);
    }
}

/**
 * This function is called when a file is being deleted.
 * 
 * @param req The request handle that contains information about the unlink request and is used to send the response back to the kernel.
 * @param parent The inode number of the parent directory containing the file to be deleted.
 * @param name The name of the file to be deleted.
 */
static void flouds_unlink(fuse_req_t req, fuse_ino_t parent, const char *name) {
    size_t parent_node = parent - 1;
    
    Flouds* flouds = file_system_manager->get_flouds();
    
    // Find the child node with the given name
    size_t num_children = flouds->children_count(parent_node);
    for (size_t i = 0; i < num_children; i++) {
        size_t child_node = flouds->child(parent_node, i);
        if (flouds->get_name(child_node) == name) {
            // Check if it's a file (not a directory)
            if (flouds->is_folder(child_node)) {
                fuse_reply_err(req, EISDIR);
                return;
            }
            
            try {
                file_system_manager->remove_node(child_node);
                fuse_reply_err(req, 0);
                file_system_manager->save();
            } catch (...) {
                fuse_reply_err(req, EIO);
            }
            return;
        }
    }
    
    // File not found
    fuse_reply_err(req, ENOENT);
}

/**
 * This function is called when a directory is being deleted.
 * 
 * @param req The request handle that contains information about the rmdir request and is used to send the response back to the kernel.
 * @param parent The inode number of the parent directory containing the directory to be deleted.
 * @param name The name of the directory to be deleted.
 */
static void flouds_rmdir(fuse_req_t req, fuse_ino_t parent, const char *name) {
    size_t parent_node = parent - 1;
    
    Flouds* flouds = file_system_manager->get_flouds();
    
    // Find the child node with the given name
    size_t num_children = flouds->children_count(parent_node);
    for (size_t i = 0; i < num_children; i++) {
        size_t child_node = flouds->child(parent_node, i);
        if (flouds->get_name(child_node) == name) {
            // Check if it's a directory (not a file)
            if (!flouds->is_folder(child_node)) {
                fuse_reply_err(req, ENOTDIR);
                return;
            }
            
            // Check if directory is empty
            if (!flouds->is_empty_folder(child_node)) {
                fuse_reply_err(req, ENOTEMPTY);
                return;
            }
            
            try {
                // Remove the directory
                file_system_manager->remove_node(child_node);
                fuse_reply_err(req, 0);
                file_system_manager->save();
            } catch (...) {
                fuse_reply_err(req, EIO);
            }
            return;
        }
    }
    
    // Directory not found
    fuse_reply_err(req, ENOENT);
}

// This structure defines the operation that our FUSE filesystem supports.
static const struct fuse_lowlevel_ops flouds_operations = {
    .init = flouds_init,
    .destroy = flouds_destroy,
    .lookup = flouds_lookup,
    .getattr = flouds_getattr,
    .setattr = flouds_setattr,
    .mkdir = flouds_mkdir,
    .unlink = flouds_unlink,
    .rmdir = flouds_rmdir,
    .open = flouds_open,
    .read = flouds_read,
    .write = flouds_write,
    .readdir = flouds_readdir,
    .create = flouds_create
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
    se = fuse_session_new(&args, &flouds_operations, sizeof(flouds_operations), (void*) image_path);

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