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
#include "nfsnetclient.h"
static char *DEFAULT_VFS_DIR = NULL;   //默认虚拟文件存储地址
static char *DEFAULT_VFS_MOUNT = NULL; //默认虚拟文件挂载点
static char *VFS_MOUNT = NULL;         //挂载 mount
static char *VFS_UNMOUNT = NULL;       //卸载 unmount
static char *VFS_WRITE_READ_MODE = NULL;
static char *VFS_READ_ONLY_MODE = NULL;
static char *VFS_USER_QUOTA_CHECK_URL = NULL;
static char *VFS_USER_QUOTA_REVERT_URL = NULL;
static char *VFS_USER_QUOTA_SEARCH_URL = NULL;
static char *VFS_USER_MOUNT_SEARCH_URL = NULL;
static char *VFS_USER_QUOTA_UPDATE_URL = NULL;
static char *VFS_USER_MOUNT_REGISTER_URL = NULL;
static char *VFS_USER_MOUNT_UNREGISTER_URL = NULL;
static char *VFS_USER_MOUNT_LIVEPROBE_URL = NULL;
static char *VFS_USER_MOUNT_UNIQUE_URL = NULL;
static char *VFS_USER_MOUNT_STATUS_URL = NULL;
static char *NFS_SERVER_ENV_PARAM = NULL; //NFS服务参数在环境变量中名称
static char *NFS_SERVER = NULL;
static char *NFS_SERVER_SHARE_DIR = NULL;
static char *NFS_SERVER_HTTP_PORT = NULL;
static char *NFS_SERVER_CREATE_VFS_URL = NULL;
static char *NFS_SERVER_DELETE_VFS_URL = NULL;
static char *NFS_SERVER_MOUNT_VFS_URL = NULL;
static char *NFS_SERVER_UMOUNT_VFS_URL = NULL;
static char *NFS_SERVER_LIVE_PROBLE_URL = NULL;
static char *NFS_USER_MOUNT_REIGISTER_URL = NULL; //初次挂载NFS时注册URL
static char *NFS_USER_MOUNT_SEARCH_URL = NULL;    //二次挂载时执行查询NFS中转路径URL

static long int VFS_DAEMON_SLEEP_DUARATION = 30;
char *nfsblock_delete_url = NULL;
void Print_Help(char *program);
int VFSFileRemove(char *user, char *filename, char *delete_url);
int NFSBlockClean(char *nfs_delete_url, char *user, char *filename);
int init_config(char *conf)
{
    cfg_opt_t opts[] = {
        CFG_SIMPLE_STR("DEFAULT_VFS_DIR", &DEFAULT_VFS_DIR),
        CFG_SIMPLE_STR("DEFAULT_VFS_MOUNT", &DEFAULT_VFS_MOUNT),
        CFG_SIMPLE_STR("VFS_MOUNT", &VFS_MOUNT),
        CFG_SIMPLE_STR("VFS_UNMOUNT", &VFS_UNMOUNT),
        CFG_SIMPLE_STR("VFS_WRITE_READ_MODE", &VFS_WRITE_READ_MODE),
        CFG_SIMPLE_STR("VFS_READ_ONLY_MODE", &VFS_READ_ONLY_MODE),
        CFG_SIMPLE_STR("VFS_USER_QUOTA_CHECK_URL", &VFS_USER_QUOTA_CHECK_URL),
        CFG_SIMPLE_STR("VFS_USER_QUOTA_SEARCH_URL", &VFS_USER_QUOTA_SEARCH_URL),
        CFG_SIMPLE_STR("VFS_USER_QUOTA_UPDATE_URL", &VFS_USER_QUOTA_UPDATE_URL),
        CFG_SIMPLE_STR("VFS_USER_MOUNT_REGISTER_URL", &VFS_USER_MOUNT_REGISTER_URL),
        CFG_SIMPLE_STR("VFS_USER_MOUNT_UNREGISTER_URL", &VFS_USER_MOUNT_UNREGISTER_URL),
        CFG_SIMPLE_STR("VFS_USER_MOUNT_LIVEPROBE_URL", &VFS_USER_MOUNT_LIVEPROBE_URL),
        CFG_SIMPLE_STR("VFS_USER_MOUNT_SEARCH_URL", &VFS_USER_MOUNT_SEARCH_URL),
        CFG_SIMPLE_STR("VFS_USER_MOUNT_STATUS_URL", &VFS_USER_MOUNT_STATUS_URL),
        CFG_SIMPLE_STR("VFS_USER_QUOTA_REVERT_URL", &VFS_USER_QUOTA_REVERT_URL),
        CFG_SIMPLE_STR("VFS_USER_MOUNT_UNIQUE_URL", &VFS_USER_MOUNT_UNIQUE_URL),
        CFG_SIMPLE_INT("VFS_DAEMON_SLEEP_DUARATION", &VFS_DAEMON_SLEEP_DUARATION),
        CFG_SIMPLE_STR("NFS_SERVER_HTTP_PORT", &NFS_SERVER_HTTP_PORT),
        CFG_SIMPLE_STR("NFS_SERVER_SHARE_DIR", &NFS_SERVER_SHARE_DIR),
        CFG_SIMPLE_STR("NFS_SERVER_CREATE_VFS_URL", &NFS_SERVER_CREATE_VFS_URL),
        CFG_SIMPLE_STR("NFS_SERVER_DELETE_VFS_URL", &NFS_SERVER_DELETE_VFS_URL),
        CFG_SIMPLE_STR("NFS_SERVER_MOUNT_VFS_URL", &NFS_SERVER_MOUNT_VFS_URL),
        CFG_SIMPLE_STR("NFS_SERVER_UMOUNT_VFS_URL", &NFS_SERVER_UMOUNT_VFS_URL),
        CFG_SIMPLE_STR("NFS_SERVER_LIVE_PROBLE_URL", &NFS_SERVER_LIVE_PROBLE_URL),
        CFG_SIMPLE_STR("NFS_USER_MOUNT_REIGISTER_URL", &NFS_USER_MOUNT_REIGISTER_URL),
        CFG_SIMPLE_STR("NFS_USER_MOUNT_SEARCH_URL", &NFS_USER_MOUNT_SEARCH_URL),
        CFG_SIMPLE_STR("NFS_SERVER_ENV_PARAM", &NFS_SERVER_ENV_PARAM),
        CFG_END()};
    cfg_t *cfg;
#ifdef LC_MESSAGES
    setlocale(LC_MESSAGES, "");
    setlocale(LC_CTYPE, "");
#endif
    cfg = cfg_init(opts, 0);
    if (cfg_parse(cfg, conf) != 0)
    {
        return -1;
    }
    if (cfg_free(cfg) != 0)
    {
        return -1;
    }
    return 0;
}

int getNfsServer()
{
    NFS_SERVER = getenv(NFS_SERVER_ENV_PARAM);
    if (NFS_SERVER == NULL)
    {
        printf("获取NFS_SERVER失败！\n");
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[])
{

    if (init_config("./etc/nfs_client.conf") != 0)
    {
        printf("初始化项目配置文件失败，程序退出!\n");
        exit(EXIT_FAILURE);
    }
    char *vfs_name = NULL; //file name of the virtual filesystem i.e path of the file
    int64_t vfs_size = 0;  //file request sieze
    char *vfs_operation_mount = "unkown";
    char *vfs_operation_umount = "unkown";
    char *vfs_mpoint = NULL; // mount_path i.e user_home_dir $HOME
    char *vfs_mmode = NULL;  //mount_mode i.e read or write
    char *target_vfs = NULL; //要删除的虚拟文件
    char *vfs_file;
    char current_timestamp[20];
    int user_gid = getgid();
    int user_uid = getuid();
    char *user = GetUserName();

    char *current_user = (char *)malloc(sizeof(char) * (strlen(user) + 1));
    if (current_user != NULL)
    {
        memcpy(current_user, user, strlen(user) + 1);
    }
    if (current_user == NULL)
    {
        printf("当前用户为%s,其值为NULL，程序退出\n", current_user);
        exit(EXIT_FAILURE);
    }

    if (getNfsServer() != 0)
    {
        printf("获取NFS服务IP失败，程序退出！\n");
        exit(EXIT_FAILURE);
    }
    int opt = 0;
    static struct option long_options[] = {
        {"filename", required_argument, 0, 'f'},
        {"filesize", required_argument, 0, 's'},
        {"mount", required_argument, 0, 'm'},
        {"umount", required_argument, 0, 'u'},
        {"list", no_argument, 0, 'l'},
        {"quoata", no_argument, 0, 'q'},
        {"delete", no_argument, 0, 'l'},
        {"version", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0},
    };
    char *nfsblock_delete_url = malloc(strlen(NFS_SERVER) + strlen(":") + strlen(NFS_SERVER_HTTP_PORT) + strlen(NFS_SERVER_DELETE_VFS_URL) + 2);
    if (nfsblock_delete_url == NULL)
    {

        printf("分配NFS文件块删除操作URL时内存出错，程序退出!\n");
        exit(EXIT_FAILURE);
    }
    strcpy(nfsblock_delete_url, NFS_SERVER);
    strcat(nfsblock_delete_url, ":"); // 注意该处的冒号：
    strcat(nfsblock_delete_url, NFS_SERVER_HTTP_PORT);
    strcat(nfsblock_delete_url, NFS_SERVER_DELETE_VFS_URL);
    char *nfsblock_create_url = malloc(strlen(NFS_SERVER) + strlen(":") + strlen(NFS_SERVER_HTTP_PORT) + strlen(NFS_SERVER_CREATE_VFS_URL) + 2);
    if (nfsblock_create_url == NULL)
    {

        printf("分配NFS文件块创建操作URL时内存出错，程序退出!\n");
        exit(EXIT_FAILURE);
    }
    strcpy(nfsblock_create_url, NFS_SERVER);
    strcat(nfsblock_create_url, ":"); // 注意该处的冒号：
    strcat(nfsblock_create_url, NFS_SERVER_HTTP_PORT);
    strcat(nfsblock_create_url, NFS_SERVER_CREATE_VFS_URL);
    char *nfsblock_mount_url = malloc(strlen(NFS_SERVER) + strlen(":") + strlen(NFS_SERVER_HTTP_PORT) + strlen(NFS_SERVER_MOUNT_VFS_URL) + 2);
    if (nfsblock_mount_url == NULL)
    {
        {
            printf("分配NFS文件块mount操作URL时内存出错，程序退出!\n");
            exit(EXIT_FAILURE);
        }
    }
    strcpy(nfsblock_mount_url, NFS_SERVER);
    strcat(nfsblock_mount_url, ":"); // 注意该处的冒号：
    strcat(nfsblock_mount_url, NFS_SERVER_HTTP_PORT);
    strcat(nfsblock_mount_url, NFS_SERVER_MOUNT_VFS_URL);
    char *nfsblock_umount_url = malloc(strlen(NFS_SERVER) + strlen(":") + strlen(NFS_SERVER_HTTP_PORT) + strlen(NFS_SERVER_UMOUNT_VFS_URL) + 2);
    if (nfsblock_umount_url == NULL)
    {
        {
            printf("分配NFS文件块卸载操作URL时内存出错，程序退出!\n");
            exit(EXIT_FAILURE);
        }
    }
    strcpy(nfsblock_umount_url, NFS_SERVER);
    strcat(nfsblock_umount_url, ":"); // 注意该处的冒号：
    strcat(nfsblock_umount_url, NFS_SERVER_HTTP_PORT);
    strcat(nfsblock_umount_url, NFS_SERVER_UMOUNT_VFS_URL);
    char *nfsblock_liveprobe_url = malloc(strlen(NFS_SERVER) + strlen(":") + strlen(NFS_SERVER_HTTP_PORT) + strlen(NFS_SERVER_LIVE_PROBLE_URL) + 2);
    if (nfsblock_liveprobe_url == NULL)
    {
        printf("分配NFS中转服务探针URL内存出错，程序退出！\n");
        exit(EXIT_FAILURE);
    }
    strcpy(nfsblock_liveprobe_url, NFS_SERVER);
    strcat(nfsblock_liveprobe_url, ":");
    strcat(nfsblock_liveprobe_url, NFS_SERVER_HTTP_PORT);
    strcat(nfsblock_liveprobe_url, NFS_SERVER_LIVE_PROBLE_URL);
    int option_index = 0;
    if (argc <= 1) //如果参数为空时，直接打印出帮助信息
    {
        Print_Help(argv[0]);
        return 0;
    }
    while ((opt = getopt_long(argc, argv, "f:s:o:m:u:d:qlhv", long_options, &option_index)) != -1) //解析命令行参数
    {
        switch (opt)
        {
        case 'f':
            vfs_name = optarg;
            printf("用户指定的文件名为 %s\n", vfs_name);
            break;
        case 's':
            vfs_size = ParseFileSizeParam(optarg);
            if (vfs_size <= 0) //对传入的文件大小作严格验证，不允许传值为负数
            {
                printf("用户指定的要创建虚拟文件参数%ld不正确，程序退出 \n", vfs_size);
                exit(EXIT_FAILURE);
            }
            printf("用户指定要创建的虚拟文件大小为 %ldGB\n", vfs_size);
            break;
        case 'o':
            vfs_mmode = optarg;
            break;
        case 'm':
            vfs_operation_mount = "mount";
            vfs_mpoint = formartVfsMountPoint(optarg);
            if (0 != isDirAbsolute(vfs_mpoint))
            {
                printf("挂载点必须以绝对路径给出，当前给出的路径为%s\n", optarg);
                exit(EXIT_FAILURE);
            }
            if (IsDirBelongToUser(vfs_mpoint, current_user) != 0)
            {
                printf("当前用户%s指定的文件挂载点%s目录不属于该用户，程序退出!\n", current_user, vfs_mpoint);
                exit(EXIT_FAILURE);
            }
            printf("用户操作为虚拟文件挂载操作: %s,挂载路径为：%s\n", vfs_operation_mount, vfs_mpoint);
            break;
        case 'u':
            vfs_operation_umount = "umount";
            vfs_mpoint = formartVfsMountPoint(optarg);
            if (0 != isDirAbsolute(vfs_mpoint))
            {
                printf("挂载点必须以绝对路径给出，当前给出的路径为%s\n", optarg);
                exit(EXIT_FAILURE);
            }
            printf("用户操作为虚拟文件卸载操作: %s,挂载点路径为：%s\n", vfs_operation_umount, vfs_mpoint);
            break;
        case 'l':
            UserVFSMountSearchHandler(VFS_USER_MOUNT_SEARCH_URL, current_user, 0);
            exit(EXIT_SUCCESS);
        case 'd':
            target_vfs = optarg;
            if (0 != UserVFSMountStatusSearchHandler(VFS_USER_MOUNT_STATUS_URL, current_user, target_vfs))
            {
                printf("文件%s仍处于挂载状态，无法直接进行删除！请先卸载文件[执行-l查询 或-u 卸载]\n", target_vfs);
                exit(EXIT_FAILURE);
            }
            char *nfsmountpoint = UserNFSMountPointSearchHandler(NFS_USER_MOUNT_SEARCH_URL, current_user, target_vfs);
            if (strcmp(nfsmountpoint, "") != 0 || nfsmountpoint != NULL)
            {

                if (nfsBlockUmountHandler(nfsblock_umount_url, current_user, user_uid, user_gid, nfsmountpoint) != 0)
                {
                    printf("该文件%s在中转容器中执行卸载操作失败[在NFS容器中挂载点为%s]！无法继续进行删除\n", target_vfs, nfsmountpoint);
                    exit(EXIT_FAILURE);
                }
            }

            if (0 != UserVFSQuotaRevertHandler(VFS_USER_QUOTA_REVERT_URL, current_user, target_vfs))
            {

                exit(EXIT_FAILURE);
            }

            if (0 != VFSFileRemove(current_user, target_vfs, nfsblock_delete_url))
            {

                exit(EXIT_FAILURE);
            };
            free(nfsblock_delete_url);
            exit(EXIT_SUCCESS);
            break;
        case 'q':
            UserVFSQuotasSearchHandler(VFS_USER_QUOTA_UPDATE_URL, current_user, 0);
            exit(EXIT_SUCCESS);

        case 'v':
            printf("%s\tversion 1.0.0\n", argv[0]);
            exit(EXIT_SUCCESS);
        case 'h':
            Print_Help(argv[0]);
            exit(EXIT_SUCCESS);

        default:

            Print_Help(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (vfs_name == NULL)
    {
        vfs_file = current_user;
        if (UserVFSQuotasCheckHandler(VFS_USER_QUOTA_CHECK_URL, current_user, vfs_size) != 0)
        {
            printf("当前用户%s申请%ld大小虚拟文件失败，用户存储配额不足，程序退出!\n", current_user, vfs_size);
            return -1;
        }
    }
    else
    {
        vfs_file = vfs_name;
    }

    //TODO: 当前创建文件策略为如果不传递文件名则设置操作用户的用户名+操作时间为文件名
    sprintf(current_timestamp, "%d", (int)time(NULL)); //获取当前时间戳

    char *vfs_file_result = malloc(strlen(vfs_file) + strlen(current_timestamp) + 2); // 拼接创建的文件名称,2=1+1 在创建文件名时有"_"字符
    if (vfs_file_result == NULL)
    {
        printf("分配虚拟文件名内存时出错，程序退出!\n");
        exit(EXIT_FAILURE);
    }
    strcpy(vfs_file_result, vfs_file);
    strcat(vfs_file_result, "_");
    strcat(vfs_file_result, current_timestamp); //设置文件名

    unsigned long mount_type;
    // vfs_mount_type = MS_ACTIVE;
    // MS_ACTIVE | MS_BIND                                                            //reset errno to zero
    // TODO: 只有在初次创建文件时进行挂载时允许进行读写模式挂载，其余只能进行读模式挂载
    if (vfs_mmode == NULL || strcmp(vfs_mmode, "rw") == 0 || (strcmp(vfs_mmode, "RW") == 0))
    { //初次挂载时默认为读写模式
        //  VFS_MOUNT_TYPE
        mount_type = MS_DIRSYNC | MS_SYNCHRONOUS;
    }
    else if (strcmp(vfs_mmode, "r") == 0 || strcmp(vfs_mmode, "R") == 0)
    { //只读模式挂载
        mount_type = MS_RDONLY;
    }
    else
    {
        printf("未知挂载类型 %s,程序退出！\n", vfs_mmode);
        return -1;
    }
    if (strcmp(vfs_operation_mount, VFS_MOUNT) == 0)
    {
        if (vfs_size > 0) //只有在指定文件大小大于0时才判断为创建文件
        {
            if (vfs_mpoint == NULL)
            {
                printf("执行文件挂载时必须指定文件挂载点，当前文件挂载点为：%s\n", vfs_mpoint);
                goto error;
            }

            if (nfsBlockServerLiveProbeHandler(nfsblock_liveprobe_url) != 0)
            {
                printf("NFS中转服务运行不正常，程序退出！\n");
                goto error;
            }
            int nfs_ret = nfsBlockCreateHandler(nfsblock_create_url, current_user, user_uid, user_gid, vfs_size, vfs_file_result);
            if (nfs_ret != 0)
            {
                printf("调用NFS服务端创建NFS文件块失败，清理创建的文件后，程序退出！\n");

                if (nfsBlockDeleteHandler(nfsblock_delete_url, current_user, vfs_file_result, user_uid, user_gid) != 0)
                {
                    printf("清理用户文件%s失败\n", vfs_file_result);
                    goto error;
                }
                exit(EXIT_FAILURE);
            }

            printf("当前创建的用户虚拟文件文件名为%s，其大小为%ldGB\n", vfs_file_result, vfs_size);

            if (vfs_mmode == NULL || strcmp(vfs_mmode, VFS_WRITE_READ_MODE) == 0)
            {
                vfs_mmode = VFS_WRITE_READ_MODE;
            }
            else
            {
                printf("初次创建文件并挂载时，必须以读写模式进行文件挂载，当前挂载模式为：%s,因当前操作错误，进行文件清理操作\n", vfs_mmode);

                if (nfsBlockDeleteHandler(nfsblock_delete_url, current_user, vfs_file_result, user_uid, user_gid) != 0)
                {
                    printf("清理用户文件%s失败，程序退出！\n", vfs_file_result);
                    goto error;
                }
                exit(EXIT_FAILURE);
            }
            // TODO: 该步骤只是进行NFS服务器上挂载，还需在本地将NFS服务器中文件挂载至本地文件文件夹
            int nfs_mount_ret = nfsBlockMountHandler(nfsblock_mount_url, current_user, user_uid, user_gid, vfs_file_result, vfs_mpoint, vfs_mmode);
            if (nfs_mount_ret != 0)
            {
                printf("挂载文件 [%s] 到挂载点[%s] 出错，程序退出! \n", vfs_file_result, vfs_mpoint);

                if (nfsBlockDeleteHandler(nfsblock_delete_url, current_user, vfs_file_result, user_uid, user_gid) != 0)
                {
                    printf("清理用户文件%s失败\n", vfs_file_result);
                    goto error;
                }
                exit(EXIT_FAILURE);
            }

            char *nfs_server_file = malloc(strlen(NFS_SERVER) + strlen(NFS_SERVER_HTTP_PORT) + strlen(NFS_SERVER_SHARE_DIR) + strlen(vfs_file_result) + 2);
           // char *nfs_server_file = malloc(strlen(":") + strlen(NFS_SERVER_SHARE_DIR) + strlen(vfs_file_result) + 2);
            strcpy(nfs_server_file, NFS_SERVER);
            strcpy(nfs_server_file, ":");
            strcat(nfs_server_file, NFS_SERVER_SHARE_DIR);
            strcat(nfs_server_file, formartNFSMountPoint(vfs_mpoint));
            char *nfs_server_mount_addr = malloc(strlen(" nolock,addr=") + strlen(NFS_SERVER) + strlen(NFS_SERVER_HTTP_PORT) + 2);
            strcpy(nfs_server_mount_addr, "nolock,addr=");
            strcat(nfs_server_mount_addr, NFS_SERVER);
            errno = 0;
            setuid(0); //进行提权操作
            int res = mount(nfs_server_file, vfs_mpoint, "nfs", mount_type, nfs_server_mount_addr);
            if (res != 0)
            {
                printf("绑定NFS远程文件%s到本地挂载点%s失败！程序退出,错误码为%d 错误信息为 %s\n", nfs_server_file, vfs_mpoint, errno, strerror(errno));
                if (nfsBlockDeleteHandler(nfsblock_delete_url, current_user, vfs_file_result, user_uid, user_gid) != 0)
                {
                    printf("清理用户文件%s失败\n", vfs_file_result);
                    goto error;
                }
                exit(EXIT_FAILURE);
            }
            // 修改文件夹权限
            if (VFSDirPermission(vfs_mpoint, "") != 0)
            {
                printf("修改文件夹权限失败，程序退出!\n");
                if (nfsBlockDeleteHandler(nfsblock_delete_url, current_user, vfs_file_result, user_uid, user_gid) != 0)
                {
                    printf("清理用户文件%s失败\n", vfs_file_result);
                    goto error;
                }
                exit(EXIT_FAILURE);
            }

            skeleton_daemon();
            int daemon_pid = getpid();
            if (UserVFSRegisterWithDaemonHandler(VFS_USER_MOUNT_REGISTER_URL, current_user, vfs_file_result, vfs_mpoint, vfs_mmode, NULL, vfs_size, daemon_pid) != 0)
            {
                printf("向服务端注册用户%s文件挂载信息失败，程序挂载点为%s,挂载模式为%s,进入清理模式\n", current_user, vfs_mpoint, vfs_mmode);
                if (VFSUMount(vfs_file_result, formartVfsMountPoint(vfs_mpoint), NULL) != 0)
                {
                    printf("从文件挂载点 %s卸载文件出错[注意文件挂载点路径必须为绝对路径]，程序退出！\n", vfs_mpoint);
                    goto error;
                }
                if (nfsBlockUmountHandler(nfsblock_mount_url, current_user, user_uid, user_gid, vfs_mpoint) != 0)
                {
                    printf("在中转容器中卸载NFS文件%s挂载信息失败！\n", vfs_file_result);
                    goto error;
                }
                if (nfsBlockDeleteHandler(nfsblock_delete_url, current_user, vfs_file_result, user_uid, user_gid) != 0)
                {
                    printf("清理用户文件%s失败\n", vfs_file_result);
                    goto error;
                }
                exit(EXIT_FAILURE);
                goto error;
            }

            //TODO: 更新用户存储配额
            if (UserVFSQuotasUpdateHandler(VFS_USER_QUOTA_UPDATE_URL, current_user, vfs_size) != 0)
            {
                printf("更新用户%s存储配额信息失败，程序退出!\n", current_user);
                if (VFSUMount(vfs_file_result, formartVfsMountPoint(vfs_mpoint), NULL) != 0)
                {
                    printf("从文件挂载点 %s卸载文件出错[注意文件挂载点路径必须为绝对路径]，程序退出！\n", vfs_mpoint);
                    goto error;
                }
                if (nfsBlockUmountHandler(nfsblock_mount_url, current_user, user_uid, user_gid, vfs_mpoint) != 0)
                {
                    printf("在中转容器中卸载NFS文件%s挂载信息失败！\n", vfs_file_result);
                    goto error;
                }
                if (nfsBlockDeleteHandler(nfsblock_delete_url, current_user, vfs_file_result, user_uid, user_gid) != 0)
                {
                    printf("清理用户文件%s失败\n", vfs_file_result);
                }
                printf("删除用户%s文件%s成功\n", user, vfs_file_result);

                goto error;
            }
            //注册首次挂载时，文件名和对应的NFS中挂载路径需要向服务端注册
            if (UserNFSMountPointRegisterHandler(NFS_USER_MOUNT_REIGISTER_URL, current_user, vfs_file_result, vfs_mpoint) != 0)
            {
                printf("向服务端注册文件%s在NFS中转服务器中挂载路径%s时，失败！程序退出！\n", vfs_file_result, vfs_mpoint);
                //TODO: 执行清理工序
                goto error;
            }
            while (1)
            {
                sleep(VFS_DAEMON_SLEEP_DUARATION);
                UserVFSLiveProbesHandler(VFS_USER_MOUNT_LIVEPROBE_URL, current_user, vfs_size);
            }
        }
        else // 没有指定vfs_size 表明仅进行文件二次挂载，需要提供文件名，文件挂载点和挂载模式[默认只读挂载]
        {

            if (vfs_name == NULL || vfs_mpoint == NULL || vfs_mmode == NULL)
            {
                printf("在对以创建文件进行二次挂载时，必须指定文件名[%s],文件挂载点[%s],，此时默认挂载模式为只读模式,高级用户也可指定挂载模式\n", vfs_name, vfs_mpoint);
                goto error;
            }

            if (vfs_mmode == NULL || strcmp("r", vfs_mmode) == 0 || strcmp(vfs_mmode, "R") == 0 || strcmp(vfs_mmode, "") == 0)
            {
                vfs_mmode = (char *)VFS_READ_ONLY_MODE;
            }
            else if ((strcmp("rw", vfs_mmode) == 0) || strcmp("RW", vfs_mmode) == 0)
            {
                vfs_mmode = (char *)VFS_WRITE_READ_MODE;
            }

            else
            {
                printf("当前指定的挂载模式%s未识别，支持的挂载模式有[r|R|rw|RW],程序退出!\n", vfs_mmode);
                goto error;
            }
            //当采用NFS方案时文件可以进行多次读写模式挂载
            char *nfs_server_file = malloc(strlen(NFS_SERVER) + strlen(NFS_SERVER_SHARE_DIR) + strlen(vfs_name) + 2);
            if (nfs_server_file == NULL)
            {
                printf("分配文件存储内存出错，错误信息为：%s\n", strerror(errno));
                goto error;
            }
            strcpy(nfs_server_file, NFS_SERVER);
            strcat(nfs_server_file, ":");
            if (nfsBlockServerLiveProbeHandler(nfsblock_liveprobe_url) != 0)
            {
                printf("NFS中转服务运行不正常，程序退出！\n");
                goto error;
            }
            char *nfs_vfsname_mpoint = UserNFSMountPointSearchHandler(NFS_USER_MOUNT_SEARCH_URL, current_user, vfs_name);
            if (nfs_vfsname_mpoint == NULL || strcmp(nfs_vfsname_mpoint, "") == 0)
            {
                printf("文件%s在NFS中转服务器上没有找到挂载信息，程序退出！\n", vfs_name);
                goto error;
            }
            strcat(nfs_server_file, NFS_SERVER_SHARE_DIR);
            strcat(nfs_server_file, formartNFSMountPoint(nfs_vfsname_mpoint));
            char *nfs_server_mount_addr = malloc(strlen("nolock,addr=") + strlen(NFS_SERVER) + strlen(NFS_SERVER_HTTP_PORT) + 2);
            strcpy(nfs_server_mount_addr, "nolock,addr=");
            strcat(nfs_server_mount_addr, NFS_SERVER);
            errno = 0;
            int res = mount(nfs_server_file, vfs_mpoint, "nfs", mount_type, nfs_server_mount_addr);
            if (res != 0)
            {
                printf("绑定NFS远程文件%s到本地挂载点%s失败！程序退出！\n 错误码为%d 错误信息为 is %s\n", nfs_server_file, vfs_mpoint, errno, strerror(errno));
                goto error;
            }
            skeleton_daemon();
            int daemon_pid = getpid();
            if (UserVFSRegisterWithDaemonHandler(VFS_USER_MOUNT_REGISTER_URL, current_user, vfs_name, vfs_mpoint, vfs_mmode, NULL, 0, daemon_pid) != 0)
            {
                printf("向服务端注册用户%s文件挂载信息失败，程序挂载点为%s,挂载模式为%s,程序退出！\n", current_user, vfs_mpoint, vfs_mmode);
                if (VFSUMount(vfs_file_result, formartVfsMountPoint(vfs_mpoint), NULL) != 0)
                {
                    printf("从文件挂载点 %s卸载文件出错[注意文件挂载点路径必须为绝对路径]，程序退出！\n", vfs_mpoint);
                    goto error;
                }
                if (remove(vfs_file_result) == 0)
                {

                    printf("删除用户%s文件%s成功\n", user, vfs_file_result);
                }
                printf("用户创建文件失败后，清理服务完成");

                goto error;
            }
            while (1)
            {
                sleep(VFS_DAEMON_SLEEP_DUARATION);
                UserVFSLiveProbesHandler(VFS_USER_MOUNT_LIVEPROBE_URL, current_user, vfs_size);
            }
        }
    }

    else if (strcmp(vfs_operation_umount, VFS_UNMOUNT) == 0) //卸载文件，此时必须指定文件挂载点
    {

        if (vfs_mpoint == NULL)
        {
            printf("执行虚拟文件系统卸载操作时，必须指定虚拟文件挂载点 %s\n", vfs_mpoint);
            goto error;
        }

        //  注销时，返回该挂载点所对应的守护进程PID ，并注销该守护进程
        int mount_daemon_pid = UserVFSUnRegisterHandler(VFS_USER_MOUNT_UNREGISTER_URL, current_user, formartVfsMountPoint(vfs_mpoint), NULL, NULL);
        if (mount_daemon_pid <= 0)
        {
            printf("向服务端注销用户%s文件挂载信息失败，程序挂载点为%s,程序退出！\n", current_user, formartVfsMountPoint(vfs_mpoint));
            goto error;
        }
        else
        {
            kill_daemon(mount_daemon_pid);
        }

        if (VFSUMount(vfs_name, formartVfsMountPoint(vfs_mpoint), NULL) != 0)
        {
            printf("从文件挂载点 %s卸载文件出错[注意文件挂载点路径必须为绝对路径]，程序退出！\n", vfs_mpoint);

            goto error;
        }
    }

    free(DEFAULT_VFS_DIR);
    free(DEFAULT_VFS_MOUNT);
    free(VFS_WRITE_READ_MODE);
    free(VFS_MOUNT);
    free(VFS_UNMOUNT);
    free(VFS_READ_ONLY_MODE);
    free(VFS_USER_MOUNT_LIVEPROBE_URL);
    free(VFS_USER_MOUNT_REGISTER_URL);
    free(VFS_USER_QUOTA_UPDATE_URL);
    free(VFS_USER_QUOTA_CHECK_URL);
    free(VFS_USER_MOUNT_UNREGISTER_URL);
    free(VFS_USER_MOUNT_UNIQUE_URL);
    free(vfs_name);
    free(current_user);
    free(NFS_SERVER_ENV_PARAM);
    free(NFS_SERVER_CREATE_VFS_URL);
    free(NFS_SERVER_DELETE_VFS_URL);
    free(NFS_SERVER_MOUNT_VFS_URL);
    free(NFS_SERVER_UMOUNT_VFS_URL);
    free(NFS_SERVER_SHARE_DIR);
    free(NFS_SERVER_LIVE_PROBLE_URL);
    free(NFS_USER_MOUNT_REIGISTER_URL);
    free(NFS_USER_MOUNT_SEARCH_URL);
    return 0;

error:
    free(vfs_file_result);
    free(current_user);
    exit(EXIT_FAILURE);
    return -1;
}

//采用Init_Deamon() 方式或POSIX 中daemon函数进行守护进程的创建，在守护进程中发送心跳信息到服务端
/**
 * 
 * 程序文件帮助信息输出
 */
void Print_Help(char *program)
{
    printf("program \'%s\'  usage is:\n %s -f [filename] -s[g|G|t|T] -o[mount|unmout] -m mode[rw|r] -p [mount path] \n", program, program);
    printf("for example: %s -f vfsdata -s 100G -o mount -p /userhome/mountpoit/ \n", program);                                            //check mount path only user dir supported
    printf("for example: %s --filename vfsdata --size 100G --operation mount   --mode rw --mountpoint /userhome/mountpoit/ \n", program); //check mount path only user dir supported
    printf(" -f \t  --filename i.e the filename,defautlt [username_unixtimestamp]\n");
    printf(" -s \t  --size i.e size of the filename, unit spec by g|G|t|T\n");
    printf(" -o \t  --mode i.e mount or umount  mode [r|rw]\n");
    printf(" -m \t  --mount i.e mount the virtual fs\n");
    printf(" -u \t  --umount i.e umount virtual fs\n");
    printf(" -d \t  --delete i.e delete user virtual fs\n");
    printf(" -q \t  --quota i.e list user virtual fs quota\n");
    printf(" -p \t  --mountpoint i.e mount point of the vfs\n");
    printf(" -l \t  --list i.e list all the mounted mount point \n");
    printf(" -v \t  --version i.e program version:%s\n", "1.0.0");
    printf(" -h \t  --help i.e print help info\n");
}

int VFSFileRemove(char *user, char *filename, char *delete_url)
{
    printf("确定要删除用户%s文件%s 吗？ [y|n]\n", user, filename);
    char ch = getchar();
    if (tolower(ch) != 'y')
    {
        printf("请确认是否要删除该文件%s\n", filename);
        return -1;
    }

    int res = nfsBlockDeleteHandler(delete_url, user, filename, getuid(), getgid());
    if (res == 0)
    {

        printf("删除用户%s文件%s成功\n", user, filename);
    }
    else
    {
        printf("删除用户%s文件%s失败，错误信息为：%s", user, filename, strerror(errno));
        return -1;
    }
    return 0;
}

int NFSBlockClean(char *nfs_delete_url, char *user, char *filename)
{
    int res = nfsBlockDeleteHandler(nfs_delete_url, user, filename, getuid(), getgid());
    if (res == 0)
    {

        printf("清理用户%s文件%s成功\n", user, filename);
        return 0;
    }
    else
    {
        printf("清理用户%s文件%s失败，错误信息为：%s", user, filename, strerror(errno));
        return -1;
    }
    return 0;
}
