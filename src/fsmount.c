/*
mount or unmount filesystem to target dir
*/
#define _LARGEFILE64_SOURCE /* for large file handler*/
#include <stdio.h>
#include <sys/mount.h>
#include <errno.h> /* strerror */
#include <string.h>
#include <linux/loop.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <malloc.h>
#include <assert.h>
#include <sys/sysmacros.h>
#include <stdlib.h>
#include <mntent.h>
#include "vfs.h"

static const char LOOPDEV_PREFIX[] = "/dev/loop";
//static int LOOPDEV_PREFIX_LEN = sizeof(LOOPDEV_PREFIX) / sizeof(LOOPDEV_PREFIX[0]) - 1;
static const char MOUNT_TABINFO[] = "/etc/mtab";

int escalate()
{
    // printf("wwww-->%d\n",geteuid());
    if (seteuid(0) == -1 || geteuid() != 0) //检查是否为root权限
    {
        fprintf(stderr, "setuid提权失败，程序退出！ \n");
        return 1;
    }

    return 0;
}

int drop()
{
    if (seteuid(getuid()) == -1 || geteuid() != getuid())
    {
        fprintf(stderr, "Failed to drop privileges.\n");
        return 1;
    }

    return 0;
}

//apt autoremove --purge snapd
char *loopdev_find_unused()
{
    int control_fd = -1;
    int n = -1;
    // setuid(0);
    if (escalate())
        return NULL;
    errno = 0;
    if ((control_fd = open("/dev/loop-control", O_RDWR)) < 0)
    {
        fprintf(stderr, "Failed to open /dev/loop-control,错误信息为%s\n", strerror(errno));
        return NULL;
    }

    if (drop())
        return NULL;

    n = ioctl(control_fd, LOOP_CTL_GET_FREE);

    if (n < 0)
    {
        fprintf(stderr, "Failed to find a free loop device.\n");
        return NULL;
    }

    int l = strlen(LOOPDEV_PREFIX) + 1 + 1; /* 1 for first character, 1 for NULL */
    {
        int m = n;
        while (m /= 10)
        {
            ++l;
        }
    }

    char *loopdev = (char *)malloc(l * sizeof(char));
    assert(sprintf(loopdev, "%s%d", LOOPDEV_PREFIX, n) == l - 1);

    return loopdev;
}


/**
 * 
 * 创建loop device 设备
 */
//set loop device   refer to https://github.com/alexchamberlain/piimg/blob/master/src/piimg-mount.c
int loopdev_setup_device(const char *file, uint64_t offset, const char *device)
{
    setuid(0);
    int file_fd = open(file, O_RDWR);
    int device_fd = -1;

    struct loop_info64 info;

    if (file_fd < 0)
    {

        fprintf(stderr, "Failed to open backing file (%s).错误信息为:%s\n", file, strerror(errno));
        goto error;
    }

    if (escalate())
        goto error;
    errno = 0;
    if ((device_fd = open(device, O_RDWR)) < 0)
    {
        fprintf(stderr, "Failed to open device (%s),错误信息为%s\n", device, strerror(errno));
        goto error;
    }

    if (drop())
        goto error;

    if (ioctl(device_fd, LOOP_SET_FD, file_fd) < 0)
    {
        fprintf(stderr, "Failed to set fd.\n");
        goto error;
    }

    close(file_fd);
    file_fd = -1;

    memset(&info, 0, sizeof(struct loop_info64)); /* Is this necessary? */
    info.lo_offset = offset;
    /* info.lo_sizelimit = 0 => max available */
    /* info.lo_encrypt_type = 0 => none */
    if (ioctl(device_fd, LOOP_SET_STATUS64, &info))
    {
        fprintf(stderr, "Failed to set info.\n");
        goto error;
    }

    close(device_fd);
    device_fd = -1;

    return 0;

error:
    if (file_fd >= 0)
    {
        close(file_fd);
    }
    if (device_fd >= 0)
    {
        ioctl(device_fd, LOOP_CLR_FD, 0);
        close(device_fd);
    }
    return 1;
}

//检测当前目录是否为挂载点
/**
 * @dir 目录 
 * 
 */
int IsDirMountPoint(char *dir)
{
    struct mntent *fment;
    FILE *fp = setmntent(MOUNT_TABINFO, "r"); //read only
    if (fp == NULL)
    {
        printf("打开mount info文件 %s 失败,程序退出!\n", MOUNT_TABINFO);
        return -1;
    }
    while ((fment = getmntent(fp)) != NULL)
    {
        if (strcmp(dir, fment->mnt_dir) == 0)
        {
            return 0;
        }
    }
    endmntent(fp);
    return -1;
}

//检测当前目录是否为挂载点
/**
 * @dir 目录 
 * 
 */
int VFSMountPointList()
{
    struct mntent *fment;
    FILE *fp = setmntent(MOUNT_TABINFO, "r"); //read only
    if (fp == NULL)
    {
        printf("打开mount info文件 %s 失败,程序退出!\n", MOUNT_TABINFO);
        return -1;
    }
    while ((fment = getmntent(fp)) != NULL)
    {

        if (strcmp(fment->mnt_type, "xfs") == 0)
            printf("虚拟文件挂载路径为:%s,挂载参数为%s\n", fment->mnt_dir, fment->mnt_opts);
    }
    endmntent(fp);
    return -1;
}

// mount file to mount point
/**
 *根据指定操作，选择挂载指定文件到目标挂载点
 * @opt  操作
 * @*vfs_name   虚拟文件 
 * char *
 * char *
 */
int VFSMount(char *vfs_name, char *vfs_mpoint, const char *vfs_mount_type)
{
    assert(vfs_mount_type);
    char *unused_loop_dev = loopdev_find_unused(); //first find unused loop dev
    if (unused_loop_dev == NULL)
    {
        return -1;
    }
    
    //printf("-unused_loop_device  is %s,min_char--->%s,min is %d\n", unused_loop_dev, min_char, min);
    //TODO: 在client端并不会使用到该方法
     if (loopdev_setup_device(vfs_name, 0, unused_loop_dev) != 0)
    {
        printf("挂载文件 %s 到loop 设备  %s 失败,程序退出!\n", vfs_name, unused_loop_dev);
        return -1;
    }


    errno = 0;
    unsigned long mount_type;
    // vfs_mount_type = MS_ACTIVE;
    // MS_ACTIVE | MS_BIND   
    //printf("------->%s\n",vfs_mount_type);                                                         //reset errno to zero
    if (strcmp(vfs_mount_type,"")==0 || strcmp(vfs_mount_type, "rw") == 0 || (strcmp(vfs_mount_type, "RW") == 0))
    { //初次挂载时默认为读写模式

        mount_type = MS_DIRSYNC | MS_SYNCHRONOUS;
    }
    else if (strcmp(vfs_mount_type, "r") == 0 || strcmp(vfs_mount_type, "R") == 0)
    { //只读模式挂载
        mount_type = MS_RDONLY;
    }
    else
    {
        printf("未知挂载类型 %s,程序退出！\n", vfs_mount_type);
        return -1;
    }
    errno = 0;
    setuid(0); //挂载文件时需要用root权限挂载
    if (mount(unused_loop_dev, vfs_mpoint, VFS_TYPE, mount_type, "nouuid") != 0)
    // sysnc dir path | no uuid -->针对xfs多次挂载时duplicate UUID problem问题
    //参考 https://linux-tips.com/t/xfs-filesystem-has-duplicate-uuid-problem/181
    {
        printf("挂载错误码为: %d,错误信息为: %s\n", errno, strerror(errno));
        return -2;
    }

    return 0;
}

// mount file to mount point
/**
 *根据指定操作，选择挂载指定文件到目标挂载点
 * @opt  操作
 * @*vfs_name   虚拟文件 
 * @*vfs_mpoint 挂载点
 * @*vfs_unmount_type 文件卸载模式
 * 
 */
//TODO: 多种卸载模式，需要在vfs_unmount_flags中指定
int VFSUMount(char *vfs_name, char *vfs_mpoint, char *vfs_unmount_flag)
{

    // mountflasg --> MNT_DETACH | MNT_EXPIRE
    //TODO: Need to check filesystem is mounted or not  refer to https://stackoverflow.com/questions/22218054/how-to-check-the-given-folder-is-a-mount-point
    // https://stackoverflow.com/questions/10410513/function-or-a-systemcall-similar-to-mountpoint-command-in-linux
    unsigned long unmount_flag;
    if (vfs_unmount_flag == NULL || strcmp(vfs_unmount_flag, "") == 0)
    {
        unmount_flag = MNT_DETACH; //设置默认卸载模式
    }
    if (IsDirMountPoint(vfs_mpoint) == 0)
    {
        errno = 0;
        if (umount2(vfs_mpoint, unmount_flag) == 0)
        {
            printf("在文件挂载点 %s上卸载文件成功! \n", vfs_mpoint);
            return 0;
        }
        else
        {
            printf("文件卸载失败！错误码为:%d,错误信息为: %s\n", errno, strerror(errno));
            return errno;
        }
    }
    else
    {
        printf("目录 %s 不是文件挂载点,卸载操作出错！\n", vfs_mpoint);
        return -2;
    }

    return 0;
}
