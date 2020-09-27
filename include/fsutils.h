#include <stdint.h>
int isdigitstr(char *str);
int64_t ParseFileSizeParam(char *char_size);
char *GetUserName();
int Init_Daemon();
int system_alternative(char *pgm, char *argv[]);
int VFSFileDelete(char *user, char *filename);
char *StringConcat(char* s1, char* s2);
int IsDirBelongToUser(char *dir, char *user);
 void skeleton_daemon();
 void kill_daemon(int pid);
char *formartVfsMountPoint(char *mountpoint);
char *formartNFSMountPoint(char *mountpoint);
int isDirAbsolute(char *dir);
 int do_mkdir(const char *path, mode_t mode);
int mkpath(const char *path, mode_t mode);