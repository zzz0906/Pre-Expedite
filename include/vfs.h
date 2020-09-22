/*
MARCO FOR VFS ,USER DEFINED MARCO
*/
                  
#define VFS_TYPE "xfs"                             // VFS FILESYSTEM TYPE (Default:XFS)
#define VFS_MOUNT_TYPE MS_DIRSYNC | MS_SYNCHRONOUS // VFS MOUNT TYPE
#define VFS_UNMOUNT_FLAG MNT_DETACH
#define HTTP_METHOD_GET "GET"
#define HTTP_METHOD_POST "POST"
#define HTTP_METHOD_PATCH "PATCH"
#define HTTP_METHOD_PUT "PUT"
#define HTTP_METHOD_DELETE "DELETE"
#define USER_VFS_QUOTA_CHECK "quota_check"      //检查用户配额
#define USER_VFS_QUOTA_UPDATE "quota_update"    //更新用户配额
#define SERVER_RESP_CODE "code"
#define SERVER_RESP_ERROR "error"
#define SERVER_RESP_SPEC "spec"
#define SERVER_RESP_TOTAL "total"
#define USER_VFS_UID "uid"
#define USER_VFS_GID "gid"
#define USER_VFS_UNAME "uname"
#define USER_VFS_NAME "vfsname"
#define USER_VFS_VMPOINT "vmpoint"
#define USER_VFS_VMMODE "vmmode"
#define USER_VFS_VMPERM "vmperm"
#define USER_VFS_VSIZE "vfsize"
#define USER_VFS_VDAEMON_PID "vfspid"
#define USER_VFS_LOCATION "vfslocation"
#define USER_VFS_LIMIT "vfslimit"
#define USER_VFS_USED "vfsused"
#define USER_VFS_QUOTAS "vfsquotas"
