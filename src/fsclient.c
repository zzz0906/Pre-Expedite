/**
 *  client 端与server端进行服务交互函数
 *  后续可考虑采用传入函数作为参数
 */
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
#define USER_VFS_VDAEMON_PID "vfspid"
#define USER_VFS_LOCATION "vfslocation"
#define USER_VFS_LIMIT "vfslimit"
#define USER_VFS_USED "vfsused"
#define USER_VFS_QUOTAS "vfsquotas"
#define USER_NFS_NAME "nfsname"
int UserVFSQuotasCheckHandler(char *target_url, char *user, int64_t quota_size)
{

    assert(user);
    const cJSON *result_spec = NULL;
    int target_url_size = strlen(target_url);
    char url_localtion[target_url_size];

    sprintf(url_localtion, target_url, user); //拼接url字符串

    char *quota_result = fsEasyClient(url_localtion, HTTP_METHOD_GET, NULL, NULL);

    cJSON *result_json = cJSON_Parse(quota_result);

    if (result_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        goto end;
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
        printf("获取用户%s配额信息失败，方法返回值为%d,返回错误信息为:%s\n", user, result_code, result_error);
        goto error;
    }

    //char *result_spec = cJSON_GetOBjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(result_json,"spec"),"vfslimit");

    result_spec = cJSON_GetObjectItemCaseSensitive(result_json, "spec");
    // printf("%d-->",cJSON_IsObject(result_spec));
    if (!cJSON_IsObject(result_spec))
    {
        printf("返回值错误，经解析，其值不为json对象");
        goto error;
    }

    const cJSON *spec_data_vfsuname = cJSON_GetObjectItemCaseSensitive(result_spec, USER_VFS_UNAME);
    const cJSON *spec_data_vfslimit = cJSON_GetObjectItemCaseSensitive(result_spec, USER_VFS_LIMIT);
    const cJSON *spec_data_vfsused = cJSON_GetObjectItemCaseSensitive(result_spec, USER_VFS_USED);
    int vfslimit = spec_data_vfslimit->valueint;
    int vfsused = spec_data_vfsused->valueint;
    char *uname = spec_data_vfsuname->valuestring;
    if (strcmp(uname, user) != 0)
    {
        printf("当前查询出的配额信息为用户%s,与操作用户%s不符\n", uname, user);
        goto error;
    }
    if (((int)(quota_size) + vfsused) >= vfslimit)
    {
        printf("当前用户剩余配额为%d,申请配额为%ld,总可用配额为%d,申请配额过大\n", (vfslimit - vfsused), quota_size, (vfslimit - vfsused));
        goto error;
    }
    goto end;

end:
    cJSON_Delete(result_json);
    return 0;
error:
    cJSON_Delete(result_json);
    return -1;
}

//用户配额信息处理函数
int UserVFSQuotasUpdateHandler(char *target_url, char *user, int64_t quota_size)
{

    //TODO: 成功创建虚拟文件块后更新用户配额表

    assert(user);
    cJSON *request_body = NULL;
    char url_localtion[255];
    // //    sprintf(url_localtion, "vfsquotas?uname=%s", user); //拼接url字符串
    sprintf(url_localtion, target_url, user); //拼接url字符串

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

    if (cJSON_AddNumberToObject(request_body, USER_VFS_USED, (int)quota_size) == NULL)
    {
        goto error;
    }

    char *quota_result = fsEasyClient(target_url, HTTP_METHOD_PATCH, NULL, cJSON_Print(request_body));
    cJSON *result_json = cJSON_Parse(quota_result);

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
        printf("更新用户%s配额信息失败，方法返回值为%d,返回错误信息为:%s\n", user, result_code, result_error);
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

//用户配额信息处理函数
int UserVFSQuotasSearchHandler(char *target_url, char *user, int64_t quota_size)
{
    assert(user);
    cJSON *request_body = NULL;
    char url_localtion[255];
    sprintf(url_localtion, target_url, user); //拼接url字符串
    char *quota_result = fsEasyClient(url_localtion, HTTP_METHOD_GET, NULL, cJSON_Print(request_body));
    cJSON *result_json = cJSON_Parse(quota_result);
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
        printf("获取用户%s配额信息失败，方法返回值为%d,返回错误信息为:%s\n", user, result_code, result_error);
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

    printf("用户%s的总存储配额为:%d,已用配额为：%d\n", user, cJSON_GetObjectItem(result_spec, "vfslimit")->valueint, cJSON_GetObjectItem(result_spec, "vfsused")->valueint);
    goto end;
error:
    cJSON_Delete(request_body);
    return -1;
end:
    cJSON_Delete(request_body);
    return 0;
}

//用户挂载信息查询函数
int UserVFSMountSearchHandler(char *target_url, char *user, int64_t quota_size)
{

    assert(user);
    cJSON *request_body = NULL;
    char url_localtion[255];
    sprintf(url_localtion, target_url, user); //拼接url字符串
    char *quota_result = fsEasyClient(url_localtion, HTTP_METHOD_GET, NULL, cJSON_Print(request_body));
    cJSON *result_json = cJSON_Parse(quota_result);
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
        printf("获取用户%s文件挂载信息失败，方法返回值为%d,返回错误信息为:%s\n", user, result_code, result_error);
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
    if (cJSON_GetArraySize(result_spec) < 1)
    {
        printf("------用户%s 当前无虚拟文件挂载信息------\n", user);
        goto end;
    }
    printf("------用户%s的虚拟文件挂载信息如下------\n", user);
    for (int i = 0; i < cJSON_GetArraySize(result_spec); i++)
    {
        cJSON *mountItem = cJSON_GetArrayItem(result_spec, i);
        char *vmpoint = cJSON_GetObjectItem(mountItem, USER_VFS_VMPOINT)->valuestring;
        char *vmmode = cJSON_GetObjectItem(mountItem, USER_VFS_VMMODE)->valuestring;
        char *vmlocation = cJSON_GetObjectItem(mountItem, USER_VFS_LOCATION)->valuestring;
        printf("---挂载点:[%s]的挂载模式为:[%s],其挂载位置为:[%s]---\n", vmpoint, vmmode, vmlocation);
    }

    goto end;
error:
    cJSON_Delete(request_body);
    return -1;
end:
    cJSON_Delete(request_body);
    return 0;
}

//根据用户挂载点和用户信息，查出该挂载点对应的守护进程信息
int UserVFSMountAndDaemonSearchHandler(char *target_url, char *user, char *mountpoint, int64_t quota_size)
{
    assert(user);
    cJSON *request_body = NULL;
    char url_localtion[255];
    sprintf(url_localtion, target_url, user, mountpoint); //拼接url字符串
    char *quota_result = fsEasyClient(url_localtion, HTTP_METHOD_GET, NULL, cJSON_Print(request_body));
    cJSON *result_json = cJSON_Parse(quota_result);
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
        printf("获取用户%s文件挂载信息失败，方法返回值为%d,返回错误信息为:%s\n", user, result_code, result_error);
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
//用户挂载信息查询函数
int UserVFSRWUniqueSearchHandler(char *target_url, char *user, char *vfsname)
{

    assert(user);
    cJSON *request_body = NULL;
    char url_localtion[255];
    sprintf(url_localtion, target_url, vfsname); //拼接url字符串
    char *quota_result = fsEasyClient(url_localtion, HTTP_METHOD_GET, NULL, cJSON_Print(request_body));
    cJSON *result_json = cJSON_Parse(quota_result);
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
        printf("获取用户%s文件挂载信息失败，方法返回值为%d,返回错误信息为:%s\n", user, result_code, result_error);
        goto error;
    }
    int unique_size = cJSON_GetObjectItemCaseSensitive(result_json, SERVER_RESP_TOTAL)->valueint;
    return unique_size;
    goto end;
error:
    cJSON_Delete(request_body);
    return -1;
end:
    cJSON_Delete(request_body);
    return 0;
}

//用户虚拟文件系统挂载后向服务端进行信息注册
int UserVFSRegisterHandler(char *target_url, char *user, char *vfsname, char *vmpoint, const char *vmmode, char *vmperm, int vfsize)
{
    assert(user);
    assert(vmpoint);
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
    if (cJSON_AddStringToObject(request_body, USER_VFS_NAME, vfsname) == NULL)
    {
        goto error;
    }
    if (cJSON_AddStringToObject(request_body, USER_VFS_VMPOINT, vmpoint) == NULL)
    {
        goto error;
    }

    if (cJSON_AddStringToObject(request_body, USER_VFS_VMMODE, vmmode) == NULL)
    {
        goto error;
    }

    if (cJSON_AddNumberToObject(request_body, USER_VFS_VSIZE, vfsize) == NULL)
    {
        goto error;
    }
    if (vmperm == NULL) //TODO: 当vmperm 设置为NULL时，需要修改其默认值 ，根据JSON 设置其值应为“” ，根据本工具业务，其值应该设置为特殊权限777
    {
        vmperm = "";
    }
    if (cJSON_AddStringToObject(request_body, USER_VFS_VMPERM, vmperm) == NULL)
    {

        goto error;
    }

    char *quota_result = fsEasyClient(target_url, HTTP_METHOD_POST, NULL, cJSON_Print(request_body));
    cJSON *result_json = cJSON_Parse(quota_result);
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

//用户虚拟文件系统挂载后向服务端进行信息注册
int UserVFSRegisterWithDaemonHandler(char *target_url, char *user, char *vfsname, char *vmpoint, const char *vmmode, char *vmperm, int vfsize, int daemon_pid)
{
    assert(user);
    assert(vmpoint);
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
    if (cJSON_AddStringToObject(request_body, USER_VFS_NAME, vfsname) == NULL)
    {
        goto error;
    }
    if (cJSON_AddStringToObject(request_body, USER_VFS_VMPOINT, vmpoint) == NULL)
    {
        goto error;
    }

    if (cJSON_AddStringToObject(request_body, USER_VFS_VMMODE, vmmode) == NULL)
    {
        goto error;
    }

    if (cJSON_AddNumberToObject(request_body, USER_VFS_VSIZE, vfsize) == NULL)
    {
        goto error;
    }
    if (cJSON_AddNumberToObject(request_body, USER_VFS_VDAEMON_PID, daemon_pid) == NULL)
    {
        goto error;
    }
    if (vmperm == NULL) //TODO: 当vmperm 设置为NULL时，需要修改其默认值 ，根据JSON 设置其值应为“” ，根据本工具业务，其值应该设置为特殊权限777
    {
        vmperm = "";
    }
    if (cJSON_AddStringToObject(request_body, USER_VFS_VMPERM, vmperm) == NULL)
    {

        goto error;
    }

    char *quota_result = fsEasyClient(target_url, HTTP_METHOD_POST, NULL, cJSON_Print(request_body));
    cJSON *result_json = cJSON_Parse(quota_result);

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

//用户虚拟文件系统卸载后向服务端进行信息注销
/**
 * @user 用户
 * @vmpoint  挂载点
 * @vmmode 挂载模式 rw-->读写，r-->只读
 * 
 */
int UserVFSUnRegisterHandler(char *target_url, char *user, char *vmpoint, char *vmmode, char *vmperm)
{
    assert(user);
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

    if (cJSON_AddStringToObject(request_body, USER_VFS_VMPOINT, vmpoint) == NULL)
    {
        goto error;
    }
    if (vmmode == NULL)
    {
        vmmode = "";
    }
    if (cJSON_AddStringToObject(request_body, USER_VFS_VMMODE, vmmode) == NULL)
    {
        goto error;
    }

    if (vmperm == NULL)
    {
        vmperm = "";
    }
    if (cJSON_AddStringToObject(request_body, USER_VFS_VMPERM, vmperm) == NULL)
    {
        goto error;
    }

    char *quota_result = fsEasyClient(target_url, HTTP_METHOD_DELETE, NULL, cJSON_Print(request_body));
    cJSON *result_json = cJSON_Parse(quota_result);

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
        printf("注销用户%s虚拟文件挂载信息失败，方法返回值为%d,返回错误信息为:%s\n", user, result_code, result_error);
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
    int daemon_pid = cJSON_GetObjectItemCaseSensitive(result_spec, USER_VFS_VDAEMON_PID)->valueint;
    if (daemon_pid <= 0)
    {
        goto error;
    }
    cJSON_Delete(request_body);
    return daemon_pid;
error:
    cJSON_Delete(request_body);
    return -1;
}

int UserVFSLiveProbesHandler(char *target_url, char *user, int64_t quota_size)
{

    cJSON *request_body = cJSON_CreateObject();
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

    char *quota_result = fsEasyClient(target_url, HTTP_METHOD_POST, NULL, cJSON_Print(request_body));
    cJSON *result_json = cJSON_Parse(quota_result);

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
        printf("服务探针%s信息失败，方法返回值为%d,返回错误信息为:%s\n", user, result_code, result_error);
        goto error;
    }
    goto end;
    return 0;

error:
    cJSON_Delete(request_body);
    return -1;
end:
    cJSON_Delete(request_body);
    return 0;
}

//首次挂载时，NFS中挂载路径需向服务端注册，使得在二次挂载时可以根据该信息查询出挂载路径，进行二次挂载
int UserNFSMountPointRegisterHandler(char *target_url, char *user, char *vfsname, char *nfsmpoint)
{
    assert(user);
    assert(vfsname);
    assert(nfsmpoint);
    cJSON *request_body = NULL;
    request_body = cJSON_CreateObject();
    if (cJSON_AddStringToObject(request_body, USER_VFS_UNAME, user) == NULL)
    {
        goto error;
    }
    if (cJSON_AddStringToObject(request_body, USER_NFS_NAME, vfsname) == NULL)
    {
        goto error;
    }
    if (cJSON_AddStringToObject(request_body, USER_VFS_VMPOINT, nfsmpoint) == NULL)
    {
        goto error;
    }
    char *quota_result = fsEasyClient(target_url, HTTP_METHOD_POST, NULL, cJSON_Print(request_body));
    cJSON *result_json = cJSON_Parse(quota_result);
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
        printf("用户%sNFS信息注册失败，方法返回值为%d,返回错误信息为:%s\n", user, result_code, result_error);
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

//执行二次挂载时，需要根据文件名查找出在NFS文件中的挂载信息
char *UserNFSMountPointSearchHandler(char *target_url,char *user, char *vfsname)
{
    assert(user);
    assert(vfsname);
    cJSON *request_body = NULL;
    char url_localtion[255];
    sprintf(url_localtion, target_url, user, vfsname); //拼接url字符串
    char *nfs_result = fsEasyClient(url_localtion, HTTP_METHOD_GET, NULL, cJSON_Print(request_body));
    cJSON *result_json = cJSON_Parse(nfs_result);
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
        printf("查询用户%s 文件%snfs挂载信息失败，方法返回值为%d,返回错误信息为:%s\n", user, vfsname, result_code, result_error);
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

    char *nfsmpoint = cJSON_GetObjectItem(result_spec, "vmpoint")->valuestring;
    cJSON_Delete(request_body);
    return nfsmpoint;
error:
    cJSON_Delete(request_body);
    return "";

}

//查询该用户文件的挂载信息
int UserVFSMountStatusSearchHandler(char *target_url, char *user, char *vfsname)
{
    assert(user);
    assert(vfsname);
    cJSON *request_body = NULL;
    char url_localtion[255];
    sprintf(url_localtion, target_url, user, vfsname); //拼接url字符串
    char *quota_result = fsEasyClient(url_localtion, HTTP_METHOD_GET, NULL, cJSON_Print(request_body));
    cJSON *result_json = cJSON_Parse(quota_result);
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
        printf("查询用户%s 文件%s挂载信息失败，方法返回值为%d,返回错误信息为:%s\n", user, vfsname, result_code, result_error);
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
    if (cJSON_GetArraySize(result_spec) >= 1)
    {
        printf("用户%s的虚拟文件块%s在使用中无法进行直接删除!\n", user, vfsname);
        printf("------用户%s的虚拟文件%s挂载信息如下------\n", user, vfsname);
        for (int i = 0; i < cJSON_GetArraySize(result_spec); i++)
        {
            cJSON *mountItem = cJSON_GetArrayItem(result_spec, i);
            char *vmpoint = cJSON_GetObjectItem(mountItem, USER_VFS_VMPOINT)->valuestring;
            char *vmmode = cJSON_GetObjectItem(mountItem, USER_VFS_VMMODE)->valuestring;
            char *vmlocation = cJSON_GetObjectItem(mountItem, USER_VFS_LOCATION)->valuestring;
            printf("---挂载点:[%s]的挂载模式为:[%s],其挂载位置为:[%s]---\n", vmpoint, vmmode, vmlocation);
            //cJSON_Delete(mountItem);
        }
        return -1;
    }

    goto end;
error:
    cJSON_Delete(request_body);
    return -1;
end:
    cJSON_Delete(request_body);
    return 0;
}

//归还用户已用Quota配额
int UserVFSQuotaRevertHandler(char *target_url, char *user, char *vfsname)
{
    assert(user);
    assert(vfsname);
    cJSON *request_body = NULL;
    request_body = cJSON_CreateObject();
    if (cJSON_AddStringToObject(request_body, USER_VFS_UNAME, user) == NULL)
    {
        goto error;
    }

    if (vfsname == NULL)
    {
        vfsname = "";
    }
    if (cJSON_AddStringToObject(request_body, USER_VFS_NAME, vfsname) == NULL)
    {
        goto error;
    }
    char *quota_result = fsEasyClient(target_url, HTTP_METHOD_DELETE, NULL, cJSON_Print(request_body));
    cJSON *result_json = cJSON_Parse(quota_result);
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
        printf("归还用户%s文件%s失败，方法返回值为%d,返回错误信息为:%s\n", user, vfsname, result_code, result_error);
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
