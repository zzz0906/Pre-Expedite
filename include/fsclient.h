#include <unistd.h>

int UserVFSQuotasCheckHandler(char *target_url, char *user, int64_t quota_size);
int UserVFSQuotasUpdateHandler(char *target_url, char *user, int64_t quota_size);
int UserVFSRegisterHandler(char *target_url, char *user,char *vfsname, char *vmpoint, const char *vmmode, char *vmperm,int vfsize);
int UserVFSUnRegisterHandler(char *target_url, char *user, char *vmpoint, char *vmmode, char *vmperm);
int UserVFSLiveProbesHandler(char *target_url, char *user, int64_t quota_size);
int UserVFSQuotasSearchHandler(char *target_url, char *user, int64_t quota_size);
int UserVFSMountSearchHandler(char *target_url, char *user, int64_t quota_size);
int UserVFSRemoveHandler(char *target_url, char *user, char *vfsname);
int UserVFSRWUniqueSearchHandler(char *target_url, char *user,  char *vfsname);
int UserVFSMountAndDaemonSearchHandler(char *target_url, char *user, char *mountpoint,int64_t quota_size);
int UserVFSRegisterWithDaemonHandler(char *target_url, char *user, char *vfsname, char *vmpoint, const char *vmmode, char *vmperm, int vfsize, int daemon_pid);
int UserVFSMountStatusSearchHandler(char *target_url, char *user, char *vfsname);
int UserVFSQuotaRevertHandler(char *target_url, char *user, char *vfsname);
char *UserNFSMountPointSearchHandler(char *target_url,char *user, char *vfsname);
int UserNFSMountPointRegisterHandler(char *target_url, char *user, char *vfsname, char *nfsmpoint);