int nfsBlockCreateHandler(char *target_url, char *user, int uid, int gid, int vfssize, char *nfsblock);

int nfsBlockMountHandler(char *target_url, char *user, int uid, int gid, char *nfsblock,char *mountpoint,char* mount_mode);
int nfsBlockUmountHandler(char *target_url, char *user, int uid, int gid, char *mount_point);
int nfsBlockDeleteHandler(char *target_url, char *user, char *nfsblock, int uid, int gid);
int nfsBlockServerLiveProbeHandler(char *target_url);