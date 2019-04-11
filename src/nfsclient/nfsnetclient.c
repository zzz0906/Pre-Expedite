#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <assert.h>
#include "cJSON.h"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <arpa/inet.h>
#include "vfs.h"
#include "fsnetutils.h"
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

//向NFS服务端发送请求，检测NFS服务端是否正常运行
int nfsBlockServerLiveProbeHandler(char *target_url)
{

   
    cJSON *request_body = NULL;

    request_body = cJSON_CreateObject();

    char *nfsblockcreate_result = fsEasyClient(target_url, HTTP_METHOD_GET, NULL, cJSON_Print(request_body));
    cJSON *result_json = cJSON_Parse(nfsblockcreate_result);

    if (result_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "cJson解析出错，错误信息为%s\n", error_ptr);
        }
        goto error;
    }
    if (!cJSON_IsObject(result_json))
    {
        printf("HTTP返回错误,请检查服务URL！，其值为%s\n", cJSON_Print(result_json));
        goto error;
    }
    int result_code = cJSON_GetObjectItemCaseSensitive(result_json, "code")->valueint;
    if (result_code != 200)
    {
        printf("NFS中转服务运行不正常！");
        goto error;
    }
    goto end;
error:
    cJSON_Delete(request_body);
    return -1;
end:
    cJSON_Delete(request_body);
    return 0;
}

//向NFS服务端发送请求，进行NFS文件块创建
int nfsBlockCreateHandler(char *target_url, char *user, int uid, int gid, int vfssize, char *nfsblock)
{

    //TODO: 成功创建虚拟文件块后更新用户配额表

    assert(user);
    assert(nfsblock);
    cJSON *request_body = NULL;

    request_body = cJSON_CreateObject();
    if (cJSON_AddNumberToObject(request_body, USER_VFS_UID, (int)getuid()) == NULL)
    {
        goto error;
    }

    if (cJSON_AddNumberToObject(request_body, USER_VFS_GID, (int)getgid()) == NULL)
    {
        goto error;
    }

    if (cJSON_AddNumberToObject(request_body, USER_VFS_VSIZE, (int)vfssize) == NULL)
    {
        goto error;
    }
    if (cJSON_AddStringToObject(request_body, USER_VFS_UNAME, user) == NULL)
    {
        goto error;
    }
    if (cJSON_AddStringToObject(request_body, USER_VFS_NAME, nfsblock) == NULL)
    {
        goto error;
    }
    char *nfsblockcreate_result = fsEasyClient(target_url, HTTP_METHOD_POST, NULL, cJSON_Print(request_body));
    cJSON *result_json = cJSON_Parse(nfsblockcreate_result);

    if (result_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "cJson解析出错，错误信息为%s\n", error_ptr);
        }
        goto error;
    }
    if (!cJSON_IsObject(result_json))
    {
        printf("HTTP返回错误,请检查服务URL！，其值为%s\n", cJSON_Print(result_json));
        goto error;
    }
    int result_code = cJSON_GetObjectItemCaseSensitive(result_json, "code")->valueint;
    char *result_error = cJSON_GetObjectItemCaseSensitive(result_json, "error")->valuestring;
    if (result_code != 200)
    {
        printf("用户%s创建NFS文件块失败，方法返回值为%d,返回错误信息为:%s\n", user, result_code, result_error);
        goto error;
    }
    goto end;
error:
    cJSON_Delete(request_body);
    return -1;
end:
    cJSON_Delete(request_body);
    return 0;
}

//向NFS服务端发送请求，进行NFS文件块删除操作
int nfsBlockDeleteHandler(char *target_url, char *user, char *nfsblock, int uid, int gid)
{
    //TODO: 成功创建虚拟文件块后更新用户配额表

    assert(user);
    assert(nfsblock);
    cJSON *request_body = NULL;

    request_body = cJSON_CreateObject();

    if (cJSON_AddNumberToObject(request_body, USER_VFS_UID, (int)getuid()) == NULL)
    {
        goto error;
    }

    if (cJSON_AddNumberToObject(request_body, USER_VFS_GID, (int)getgid()) == NULL)
    {
        goto error;
    }
    if (cJSON_AddStringToObject(request_body, USER_VFS_UNAME, user) == NULL)
    {
        goto error;
    }

    if (cJSON_AddStringToObject(request_body, USER_VFS_NAME, nfsblock) == NULL)
    {
        goto error;
    }
    char *nfsblockdelete_result = fsEasyClient(target_url, HTTP_METHOD_DELETE, NULL, cJSON_Print(request_body));
    cJSON *result_json = cJSON_Parse(nfsblockdelete_result);
    if (result_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "cJson解析出错，错误信息为%s\n", error_ptr);
        }
        goto error;
    }
    if (!cJSON_IsObject(result_json))
    {
        printf("HTTP返回错误,请检查服务URL！，其值为%s\n", cJSON_Print(result_json));
        goto error;
    }
    int result_code = cJSON_GetObjectItemCaseSensitive(result_json, SERVER_RESP_CODE)->valueint;
    char *result_error = cJSON_GetObjectItemCaseSensitive(result_json, SERVER_RESP_ERROR)->valuestring;
    if (result_code != 200)
    {
        printf("删除用户%s虚拟文件%s失败，方法返回值为%d,返回错误信息为:%s\n", user, nfsblock, result_code, result_error);
        goto error;
    }
    cJSON *result_spec = cJSON_GetObjectItemCaseSensitive(result_json, SERVER_RESP_SPEC);

    if (result_spec == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "cJson解析出错，错误信息为%s\n", error_ptr);
        }
        goto error;
    }

    goto end;
error:
    cJSON_Delete(request_body);
    return -1;
end:
    cJSON_Delete(request_body);
    return 0;
}

//向NFS服务端发送请求，进行NFS文件mount绑定操作
int nfsBlockMountHandler(char *target_url, char *user, int uid, int gid, char *nfsblock, char *mountpoint, char *mount_mode)
{

    //TODO: 成功创建虚拟文件块后更新用户配额表

    assert(user);
    assert(nfsblock);
    assert(mountpoint);
    cJSON *request_body = NULL;

    request_body = cJSON_CreateObject();
    if (cJSON_AddNumberToObject(request_body, USER_VFS_UID, (int)getuid()) == NULL)
    {
        goto error;
    }

    if (cJSON_AddNumberToObject(request_body, USER_VFS_GID, (int)getgid()) == NULL)
    {
        goto error;
    }
    if (cJSON_AddStringToObject(request_body, USER_VFS_UNAME, user) == NULL)
    {
        goto error;
    }
    if (cJSON_AddStringToObject(request_body, USER_VFS_NAME, nfsblock) == NULL)
    {
        goto error;
    }
    if (cJSON_AddStringToObject(request_body, USER_VFS_VMPOINT, mountpoint) == NULL)
    {
        goto error;
    }

    if (cJSON_AddStringToObject(request_body, USER_VFS_VMMODE, mount_mode) == NULL)
    {
        goto error;
    }

    char *nfsblockmount_result = fsEasyClient(target_url, HTTP_METHOD_POST, NULL, cJSON_Print(request_body));
    cJSON *result_json = cJSON_Parse(nfsblockmount_result);

    if (result_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "cJson解析出错，错误信息为%s\n", error_ptr);
        }
        goto error;
    }
    if (!cJSON_IsObject(result_json))
    {
        printf("HTTP返回错误,请检查服务URL！，其值为%s\n", cJSON_Print(result_json));
        goto error;
    }
    int result_code = cJSON_GetObjectItemCaseSensitive(result_json, "code")->valueint;
    char *result_error = cJSON_GetObjectItemCaseSensitive(result_json, "error")->valuestring;
    if (result_code != 200)
    {
        printf("用户%s挂载信息注册失败，方法返回值为%d,返回错误信息为:%s\n", user, result_code, result_error);
        goto error;
    }
    goto end;
error:
    cJSON_Delete(request_body);
    return -1;
end:
    cJSON_Delete(request_body);
    return 0;
}

//向NFS服务端发送请求，进行NFS文件块卸载操作
int nfsBlockUmountHandler(char *target_url, char *user, int uid, int gid, char *mount_point)
{

    assert(user);
    assert(mount_point);
    cJSON *request_body = NULL;

    request_body = cJSON_CreateObject();
    if (cJSON_AddNumberToObject(request_body, USER_VFS_UID, (int)getuid()) == NULL)
    {
        goto error;
    }

    if (cJSON_AddNumberToObject(request_body, USER_VFS_GID, (int)getgid()) == NULL)
    {
        goto error;
    }
    if (cJSON_AddStringToObject(request_body, USER_VFS_UNAME, user) == NULL)
    {
        goto error;
    }

    if (cJSON_AddStringToObject(request_body, USER_VFS_VMPOINT, mount_point) == NULL)
    {
        goto error;
    }
    // printf("target_url is %s,request body is %s\n",target_url,cJSON_Print(request_body));
    char *nfsblockumount_result = fsEasyClient(target_url, HTTP_METHOD_POST, NULL, cJSON_Print(request_body));
    cJSON *result_json = cJSON_Parse(nfsblockumount_result);

    if (result_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "cJson解析出错，错误信息为%s\n", error_ptr);
        }
        goto error;
    }
    if (!cJSON_IsObject(result_json))
    {
        printf("HTTP返回错误,请检查服务URL！，其值为%s\n", cJSON_Print(result_json));
        goto error;
    }
    int result_code = cJSON_GetObjectItemCaseSensitive(result_json, "code")->valueint;
    char *result_error = cJSON_GetObjectItemCaseSensitive(result_json, "error")->valuestring;
    if (result_code != 200)
    {
        printf("用户%s卸载挂载点%s失败，方法返回值为%d,返回错误信息为:%s\n", user, mount_point, result_code, result_error);
        goto error;
    }
    goto end;
error:
    cJSON_Delete(request_body);
    return -1;
end:
    cJSON_Delete(request_body);
    return 0;
}