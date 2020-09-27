/*
change user filesysyem perimission
refer to https://stackoverflow.com/questions/4568681/using-chmod-in-a-c-program
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define PERM_DEFAULT S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | __S_ISVTX //| S_ISVTX// rw-r-r-

struct UserPerm
{
    uid_t uid;
    uid_t gid;
} user_perm;

//perm_mode demo 0755 0644
int VFSPermission(char *vfs_path, char *perm_mode)
{

    struct UserPerm user_perm = {
        .uid = getuid(),
        .gid = getgid(),
    };                  //define user perm struct;
    struct stat vfstat; // file stat
    if (chown(vfs_path, user_perm.uid, user_perm.gid) != 0)
    {
        printf("chown file %s permission fail,error info is: %s\n", vfs_path, strerror(errno));
        return errno;
    }
    errno = 0; //reset errno
    if (stat(vfs_path, &vfstat) != 0)
    {
        printf("stat file %s fail ,error info is: %s\n", vfs_path, strerror(errno));
        return errno;
    }
    mode_t vfs_mode = vfstat.st_mode | PERM_DEFAULT;
    errno = 0;
    if (chmod(vfs_path, vfs_mode) != 0) //RW-------.
    {

        printf("set file %s permission fail,error info is: %s\n", vfs_path, strerror(errno));
        return errno;
    }

    return 0;
}

int VFSDirPermission(char *vfs_path, int user_id, int group_id, char *perm_mode) // change perm for mount point
{
    mode_t mode = S_IRWXU | S_IRWXG ;
    errno = 0;                      //reset the errno
    setuid(0);                      //修改文件夹权限时需要进行提权操作
    if (chmod(vfs_path, mode) != 0) //change mode 755.
    {
        printf("修改文件夹%s权限为755失败，错误信息为: %s\n", vfs_path, strerror(errno));
        return errno;
    }
    errno = 0;
    if (chown(vfs_path, user_id, group_id) != 0)
    {
        printf("修改文件夹 %s 所属用户[%d]和所属组[%d]权限失败，错误信息为: %s\n", vfs_path, user_id,group_id,strerror(errno));
        return errno;
    }

    return 0;
}
