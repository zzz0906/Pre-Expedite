/**
 *NFS虚拟化文件系统工具
 *author liang.du@nscc-gz.cn
 *date 2018.11.15 
 *version 1.0.0
 * 
 */
#define _LARGEFILE64_SOURCE /* for large file handler*/
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <time.h>
#include <assert.h>
#include "confuse.h"
#include "fsperm.h"
#include "fsmk.h"
#include "fsmount.h"
#include "fsutils.h"
#include "fsclient.h"
#include "mongoose.h"
#include <fcntl.h> //Specified in man 2 open
static char *VFS_WRITE_READ_MODE = "rw";
static char *NFS_SERVER_SHARE_DIR = "HSS-SHARE-DIR/";
static char *NFS_BLOCK_DEFAULT_DIR = "HSS/";

static char *concat_nfsblock_location(char *nfsdefault_dir, char *nfsblock_name)
{
    //文件夹不存在，则创建该文件夹
    if (!opendir(nfsdefault_dir))
    {
        mkdir(nfsdefault_dir, S_IRWXU | S_IRWXG | S_IRWXO);
    }
    if (!opendir(NFS_SERVER_SHARE_DIR))
    {
        mkdir(NFS_SERVER_SHARE_DIR, S_IRWXU | S_IRWXG | S_IRWXO);
    }
    char *nfsblock_location = (char *)malloc(strlen(nfsdefault_dir) + strlen(nfsblock_name) + 1);
    sprintf(nfsblock_location, "%s%s", nfsdefault_dir, nfsblock_name);
    return nfsblock_location;
}
int main(int argc, char *argv[])
{
    char *vfs_name = NULL; //file name of the virtual filesystem i.e path of the file
    int64_t vfs_size = 0;  //file request sieze
    long vfs_user = 0l;
    long vfs_group =0l;

    if (argc != 5)
    {
        printf("程序参数错误，所需参数个数为argv[1]:文件名%s,argv[2]:文件大小%s，argv[3]文件所属用户ID:%s,argv[3]文件所属用户组ID:%s,【当前参数个数为%d】\n", argv[1], argv[2], argv[3], argv[4],argc);
        exit(EXIT_FAILURE);
    }


    char *vfs_file_name = argv[1]; //第一个参数为文件名

    vfs_size = ParseFileSizeParam(argv[2]); //第二个参数为文件大小
    if (vfs_size <= 0)                      //对传入的文件大小作严格验证，不允许传值为负数
    {
        printf("用户指定的要创建虚拟文件参数%ld不正确，程序退出 \n", vfs_size);
        exit(EXIT_FAILURE);
    }
    vfs_user = strtol(argv[3], NULL, 10); //第三个参数为用户名 以用户ID标识
    vfs_group = strtol(argv[4], NULL, 10); //第四个参数为用户组，以用户group ID标识
    char *vfs_blockname_location = concat_nfsblock_location(NFS_BLOCK_DEFAULT_DIR, vfs_file_name);
    if (vfs_size > 0) //只有在指定文件大小大于0时才判断为创建文件
    {
        if (NFS_SERVER_SHARE_DIR == NULL)
        {
            printf("执行文件挂载时必须指定文件挂载点，当前文件挂载点为：%s\n", NFS_SERVER_SHARE_DIR);
            goto error;
        }

        int nbc_res = VFSBlockCreate(vfs_blockname_location, vfs_size); //创建文件块，并格式化
        if (nbc_res != 0)
        {
            printf("创建NFS文件块失败，程序退出！\n");
            exit(EXIT_FAILURE);
        }

        setuid(0); //进行提权操作
        int nm_res = VFSMount(vfs_blockname_location, NFS_SERVER_SHARE_DIR, VFS_WRITE_READ_MODE);
        if (nm_res != 0)
        {
            printf("绑定文件%s到挂载点%s失败！程序退出,错误码为%d 错误信息为 %s\n", vfs_file_name, NFS_SERVER_SHARE_DIR, errno, strerror(errno));
            remove(vfs_blockname_location);
            exit(EXIT_FAILURE);
        }
        int npm_res = VFSDirPermission(NFS_SERVER_SHARE_DIR, vfs_user, vfs_group, NULL);
        if (npm_res != 0)
        {
            printf("修改文件夹%s所有者为%ld:%ld出错，程序退出[清理挂载信息和删除文件]！\n", NFS_SERVER_SHARE_DIR, vfs_user, vfs_group);
            VFSUMount(vfs_blockname_location, NFS_SERVER_SHARE_DIR, NULL);
            remove(vfs_blockname_location);
            exit(EXIT_FAILURE);
        }
        printf("当前创建的用户虚拟文件文件名为%s，其大小为%ldGB,挂载至%s文件夹，修改该文件夹权限为%ld:%ld\n", vfs_file_name, vfs_size, NFS_SERVER_SHARE_DIR, vfs_user, vfs_group);
    }

    free(vfs_name);

    return 0;

error:

  
    exit(EXIT_FAILURE);
    return -1;
}
