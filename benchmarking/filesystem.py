import os

# Base class for a filesystem that defines its lifecycle
class FileSystem:
    def __init__(self):
        pass

    # Setup the filesystem (e.g., create image, mount) at the folder "tmp"
    def setup(self):
        raise NotImplementedError()
    
    # Teardown the filesystem (e.g., unmount, clean up)
    def teardown(self):
        raise NotImplementedError()
    
    # Get the used space in bytes after running a workload
    def used_space(self):
        statvfs = os.statvfs("tmp")
        used = (statvfs.f_blocks - statvfs.f_bfree) * statvfs.f_frsize
        return used

# Ext4 filesystem implementation
class Ext4FileSystem(FileSystem):
    def setup(self):
        os.system("truncate -s 100G ext4.img")
        os.system("/usr/sbin/mkfs.ext4 -F ext4.img")
        os.system("mkdir tmp")
        os.system("sudo mount -o loop ext4.img tmp")
    
    def teardown(self):
        os.system("sudo umount tmp")
        os.system("rm -rf tmp ext4.img")

# Ext4 filesystem with FUSE mirror
class Ext4FuseFileSystem(FileSystem):
    def setup(self):
        os.system("truncate -s 100G ext4.img")
        os.system("/usr/sbin/mkfs.ext4 -F ext4.img")
        os.system("mkdir tmp_direct")
        os.system("sudo mount -o loop ext4.img tmp_direct")
        os.system("mkdir tmp")
        os.system("sudo bindfs tmp_direct tmp")
    
    def teardown(self):
        os.system("sudo umount tmp")
        os.system("sudo umount tmp_direct")
        os.system("rm -rf tmp tmp_direct ext4.img")

# FLOUDS filesystem implementation
class FloudsFileSystem(FileSystem):
    def setup(self):
        os.system("sudo cp ../build/succinct_filesystem ./succinct_filesystem")
        os.system("mkdir tmp")
        os.system("sudo ./succinct_filesystem $(pwd)/flouds.img tmp")

    def teardown(self):
        os.system("fusermount -u tmp")
        os.system("rm -rf tmp flouds.img succinct_filesystem")