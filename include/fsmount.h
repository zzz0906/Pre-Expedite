/*
mount or unmount filesystem to target dir

*/
char * loopdev_find_unused();
int loopdev_setup_device(const char * file, uint64_t offset, const char * device);
int VFSMount(char *vfs_name, char *vfs_mpoint, const char *vfs_mount_type);
int VFSUMount(char *vfs_name, char *vfs_mpoint,char *vfs_unmount_flag);
int IsDirMountPoint(char *dir);
int VFSMountPointList();    
