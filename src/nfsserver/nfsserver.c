/**
 *NFS虚拟化文件系统工具-服务端
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
#include <time.h>
#include <dirent.h>
#include <assert.h>
#include "confuse.h"
#include "fsperm.h"
#include "fsmk.h"
#include "fsmount.h"
#include "fsutils.h"
#include "fsclient.h"
#include "mongoose.h"
#include "cJSON.h"
#include "vfs.h"

static char *NFS_SERVER_HTTP_PORT = NULL;
static char *NFS_SERVER_CREATE_VFS_URL = NULL;
static char *NFS_SERVER_DELETE_VFS_URL = NULL;
static char *NFS_SERVER_MOUNT_VFS_URL = NULL;
static char *NFS_SERVER_UMOUNT_VFS_URL = NULL;
static char *NFS_SERVER_LIVE_PROBLE_URL = NULL;
static char *NFS_BLOCK_DEFAULT_DIR = NULL; //NFS文件块默认存储位置
static char *NFS_SHARE_DEFAULT_DIR = NULL; //NFSg共享文件夹位置

int init_config(char *conf)
{
    cfg_opt_t opts[] = {
        CFG_SIMPLE_STR("NFS_BLOCK_DEFAULT_DIR", &NFS_BLOCK_DEFAULT_DIR),
        CFG_SIMPLE_STR("NFS_SHARE_DEFAULT_DIR", &NFS_SHARE_DEFAULT_DIR),
        CFG_SIMPLE_STR("NFS_SERVER_HTTP_PORT", &NFS_SERVER_HTTP_PORT),
        CFG_SIMPLE_STR("NFS_SERVER_CREATE_VFS_URL", &NFS_SERVER_CREATE_VFS_URL),
        CFG_SIMPLE_STR("NFS_SERVER_DELETE_VFS_URL", &NFS_SERVER_DELETE_VFS_URL),
        CFG_SIMPLE_STR("NFS_SERVER_MOUNT_VFS_URL", &NFS_SERVER_MOUNT_VFS_URL),
        CFG_SIMPLE_STR("NFS_SERVER_UMOUNT_VFS_URL", &NFS_SERVER_UMOUNT_VFS_URL),
        CFG_SIMPLE_STR("NFS_SERVER_LIVE_PROBLE_URL", &NFS_SERVER_LIVE_PROBLE_URL),

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

static struct mg_serve_http_opts s_http_server_opts;
static const struct mg_str s_get_method = MG_MK_STR("GET");
static const struct mg_str s_post_method = MG_MK_STR("POST");
static const struct mg_str s_delele_method = MG_MK_STR("DELETE");
static int is_equal(const struct mg_str *s1, const struct mg_str *s2)
{
    return s1->len == s2->len && memcmp(s1->p, s2->p, s2->len) == 0;
}
static char *concat_nfsblock_location(char *nfsdefault_dir, char *nfsblock_name)
{
    //文件夹不存在，则创建该文件夹
    if (!opendir(nfsdefault_dir))
    {
        mkdir(nfsdefault_dir, S_IRWXU | S_IRWXG | S_IRWXO);
    }
    char *nfsblock_location = (char *)malloc(strlen(nfsdefault_dir) + strlen(nfsblock_name) + 1);
    sprintf(nfsblock_location, "%s%s", nfsdefault_dir, nfsblock_name);
    return nfsblock_location;
}
//接收请求，执行创建和格式化文件系统
static void handle_create_vfsblock(struct mg_connection *nc, const struct http_message *hm)
{
    int result_code = 1000;
    char *err_info = malloc(sizeof(char) * 255);
    const struct mg_str *body =
        hm->query_string.len > 0 ? &hm->query_string : &hm->body;
    mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");

    char *body_content = (char *)malloc(sizeof(char) * (body->len) + 1);
    if (body_content == NULL)
    {
        err_info = "分配body_content内存信息出错，程序退出！ \n";
        goto end;
    }
    strncpy(body_content, body->p, body->len);
    cJSON *req_body_json = cJSON_Parse(body_content);
    if (req_body_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            sprintf(err_info, "cJson解析出错，错误信息为%s\n", error_ptr);
            goto end;
        }
    }
    if (!cJSON_IsObject(req_body_json))
    {
        sprintf(err_info, "请求Request Body 格式错误，请检查！，其值为%s\n", cJSON_Print(req_body_json));
        goto end;
    }
    if (!cJSON_HasObjectItem(req_body_json, USER_VFS_VSIZE))
    {
        err_info = "json参数vfsize不存在！";
        goto end;
    }
    int vfs_size = cJSON_GetObjectItemCaseSensitive(req_body_json, USER_VFS_VSIZE)->valueint;
    if (!cJSON_HasObjectItem(req_body_json, USER_VFS_NAME))
    {
        err_info = "json参数vfsname不存在！";
        goto end;
    }
    char *vfs_blockname = cJSON_GetObjectItemCaseSensitive(req_body_json, USER_VFS_NAME)->valuestring;
    char *vfs_blockname_location = concat_nfsblock_location(NFS_BLOCK_DEFAULT_DIR, vfs_blockname);
    int block_create_res = VFSBlockCreate(vfs_blockname_location, vfs_size);
    if (block_create_res != 0)
    {
        sprintf(err_info, "创建文件%s错误", vfs_blockname_location);
        goto end;
    }
    result_code = 200;
    err_info = "";
    goto end;

end:
    cJSON_Delete(req_body_json);
    mg_printf_http_chunk(nc, "{ \"code\": %d , \"error\": \"%s\",\"kind\": \"%s\",\"total\":%d,\"spec\":\"%s\"}", result_code, err_info, "vfsblockcreate", vfs_size, vfs_blockname_location);
    mg_send_http_chunk(nc, "", 0);
    free(body_content);
}

//接收请求，执行文件块删除操作
static void handle_delete_vfsblock(struct mg_connection *nc, const struct http_message *hm)
{
    int result_code = 1000;
    char *err_info = malloc(sizeof(char) * 255);
    const struct mg_str *body =
        hm->query_string.len > 0 ? &hm->query_string : &hm->body;

    mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
    char *body_content = malloc(sizeof(char) * hm->body.len);
    if (body_content == NULL)
    {
        err_info = "分配body_content内存信息出错，程序退出！";
        goto end;
    }
    strcpy(body_content, body->p);

    cJSON *req_body_json = cJSON_Parse(body_content);

    if (req_body_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            sprintf(err_info, "cJson解析出错，错误信息为%s\n", error_ptr);
            goto end;
        }
    }
    if (!cJSON_IsObject(req_body_json))
    {
        sprintf(err_info, "请求Request Body 格式错误，请检查！，其值为%s\n", cJSON_Print(req_body_json));
        goto end;
    }

    char *vfs_user = cJSON_GetObjectItemCaseSensitive(req_body_json, USER_VFS_UNAME)->valuestring;
    if (!cJSON_HasObjectItem(req_body_json, USER_VFS_NAME))
    {
        err_info = "json参数vfsname不存在！";
        goto end;
    }
    char *vfs_blockname = cJSON_GetObjectItemCaseSensitive(req_body_json, USER_VFS_NAME)->valuestring;
    char *vfs_blockname_location = concat_nfsblock_location(NFS_BLOCK_DEFAULT_DIR, vfs_blockname);
    //TODO: 该处逻辑需要优化
    if (remove(vfs_blockname_location) != 0)
    {
        sprintf(err_info, "删除用户%s文件%s失败，错误信息为：%s", vfs_user, vfs_blockname_location, strerror(errno));
        goto end;
    }
    result_code = 200;
    err_info = "";
    goto end;

end:
    cJSON_Delete(req_body_json);
    mg_printf_http_chunk(nc, "{ \"code\": %d , \"error\": \"%s\",\"kind\": \"%s\",\"total\":%d,\"spec\":\"%s\"}", result_code, err_info, "vfsblockremove", 0, vfs_blockname_location);
    mg_send_http_chunk(nc, "", 0);
    free(body_content);
}

//接收请求，执行文件块绑定操作
static void handle_mount_vfsblock(struct mg_connection *nc, const struct http_message *hm)
{
    int result_code = 1000;
    char *err_info = malloc(sizeof(char) * 255);
    const struct mg_str *body =
        hm->query_string.len > 0 ? &hm->query_string : &hm->body;
    mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
    char *body_content = malloc(sizeof(char) * hm->body.len);
    if (body_content == NULL)
    {
        err_info = "分配body_content内存信息出错，程序退出！";
        goto end;
    }
    strcpy(body_content, body->p);
    cJSON *req_body_json = cJSON_Parse(body_content);
    if (req_body_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            sprintf(err_info, "cJson解析出错，错误信息为%s\n", error_ptr);
            goto end;
        }
    }
    if (!cJSON_IsObject(req_body_json))
    {
        printf("请求Request Body 格式错误，请检查！，其值为%s\n", cJSON_Print(req_body_json));
        err_info = "请求Request Body 格式错误，请检查！";
        goto end;
    }
    if (!cJSON_HasObjectItem(req_body_json, USER_VFS_NAME))
    {
        err_info = "json参数vfsname不存在！";
        goto end;
    }
    char *vfs_blockname = cJSON_GetObjectItemCaseSensitive(req_body_json, USER_VFS_NAME)->valuestring;
    if (!cJSON_HasObjectItem(req_body_json, USER_VFS_VMPOINT))
    {
        err_info = "文件挂载点不存在！";
        goto end;
    }
    char *vfs_mpoint = cJSON_GetObjectItemCaseSensitive(req_body_json, USER_VFS_VMPOINT)->valuestring;
    if (!cJSON_HasObjectItem(req_body_json, USER_VFS_VMMODE))
    {
        err_info = "文件挂载模式参数未知或错误！";
        goto end;
    }
    char *vfs_mmode = cJSON_GetObjectItemCaseSensitive(req_body_json, USER_VFS_VMMODE)->valuestring;
    char *vfs_block_location = concat_nfsblock_location(NFS_BLOCK_DEFAULT_DIR, vfs_blockname);
    char *vfs_mpoint_location = concat_nfsblock_location(NFS_SHARE_DEFAULT_DIR, formartNFSMountPoint(vfs_mpoint));
    errno = 0;
    DIR *targetDir = opendir(vfs_mpoint_location);
    if (targetDir)
    {
        closedir(targetDir);
    }
    else if (ENOENT == errno)
    { //文件夹不存在
        mkpath(vfs_mpoint_location, S_IRWXU | S_IRWXG | S_IRWXO);
    }

    int vfsmount_res = VFSMount(vfs_block_location, vfs_mpoint_location, vfs_mmode);
    //TODO: 需要检查是否需要重起NFS服务器
    if (vfsmount_res != 0)
    {
        sprintf(err_info, "以挂载模式%s挂载文件%s到挂载点%s,失败！\n", vfs_mmode, vfs_blockname, vfs_mpoint);
        goto end;
    }
    //TODO: 在挂载NFS服务端时不需要修改文件权限
    // int perm_res = VFSDirPermission(vfs_mpoint, NULL);
    // if (perm_res != 0)
    // {
    //     sprintf(err_info, "修改文件夹%s权限错误！\n", vfs_mpoint_location);
    //     goto end;
    // }

    result_code = 200;
    err_info = "";
    goto end;
end:
    cJSON_Delete(req_body_json);
    mg_printf_http_chunk(nc, "{ \"code\": %d , \"error\": \"%s\",\"kind\": \"%s\",\"total\":%d,\"spec\":\"%s\"}", result_code, err_info, "vfsblock", 0, vfs_mpoint_location);
    mg_send_http_chunk(nc, "", 0);
    free(body_content);
}

//接收请求参数，执行文件卸载操作
static void handle_unmount_vfsblock(struct mg_connection *nc, const struct http_message *hm)
{

    char *err_info = malloc(sizeof(char) * 255);
    char *vfs_mpoint_location = NULL;
    int result_code = 1000;
    const struct mg_str *body =
        hm->query_string.len > 0 ? &hm->query_string : &hm->body;
    mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
    char *body_content = malloc(sizeof(char) * hm->body.len);
    if (body_content == NULL)
    {
        err_info = "分配body_content内存信息出错，程序退出！";
        goto end;
    }
    strcpy(body_content, body->p);

    cJSON *req_body_json = cJSON_Parse(body_content);
    if (req_body_json == NULL)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            sprintf(err_info, "cJson解析出错，错误信息为%s\n", error_ptr);
            goto end;
        }
    }

    if (!cJSON_IsObject(req_body_json))
    {
        printf("请求Request Body 格式错误，请检查！，其值为%s\n", cJSON_Print(req_body_json));
    }
    
    if (!cJSON_HasObjectItem(req_body_json, USER_VFS_VMPOINT))
    {
        err_info = "文件挂载点不存在！";
        goto end;
    }

    char *vfs_mpoint = cJSON_GetObjectItemCaseSensitive(req_body_json, USER_VFS_VMPOINT)->valuestring;
    //TODO: 需要检查是否需要重起NFS服务器
    vfs_mpoint_location = concat_nfsblock_location(NFS_SHARE_DEFAULT_DIR, formartNFSMountPoint(vfs_mpoint));
    int vfs_umount_res = VFSUMount(NULL, vfs_mpoint_location, NULL);
    if (vfs_umount_res != 0)
    {
        sprintf(err_info, "从挂载点%s上执行卸载操作失败！\n", vfs_mpoint );
        goto end;
    }
    result_code = 200;
    err_info = "";
    goto end;
end:
    cJSON_Delete(req_body_json);
    mg_printf_http_chunk(nc, "{ \"code\": %d , \"error\":\"%s\",\"kind\": \"%s\",\"total\":%d,\"spec\":\"%s\"}", result_code, err_info, "vfsumount", 0, vfs_mpoint_location);
    mg_send_http_chunk(nc, "", 0);
    free(body_content);
}
//不支持方法处理
static void handler_unsupported_method(struct mg_connection *nc, const struct http_message *hm)
{
    mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
    mg_printf_http_chunk(nc, "{ \"method not supported !\"}");

    mg_send_http_chunk(nc, "", 0);
}
//服务探针
static void handler_liveprobe_method(struct mg_connection *nc, const struct http_message *hm)
{
    mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
    mg_printf_http_chunk(nc, "{ \"code\": %d , \"error\": \"%s\",\"kind\": \"%s\",\"total\":%d,\"spec\":\"%s\"}", 200, "", "nfs-server lives ok!!!", 0, "NFS SERVER RUNNING OK ！");

    mg_send_http_chunk(nc, "", 0);
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
 
    struct http_message *hm = (struct http_message *)ev_data;
    if (ev == MG_EV_HTTP_REQUEST)
    {
        if (mg_vcmp(&hm->uri, NFS_SERVER_CREATE_VFS_URL) == 0)
        {
            if (is_equal(&hm->method, &s_post_method))
            {
                handle_create_vfsblock(nc, hm);
            }
            else
            {
                handler_unsupported_method(nc, hm);
            }
        }
        else if (mg_vcmp(&hm->uri, NFS_SERVER_DELETE_VFS_URL) == 0) //删除VFS文件块
        {
            if (is_equal(&hm->method, &s_delele_method))
            {
                handle_delete_vfsblock(nc, hm);
            }
            else
            {
                handler_unsupported_method(nc, hm);
            }
        }
        else if (mg_vcmp(&hm->uri, NFS_SERVER_MOUNT_VFS_URL) == 0) //绑定VFS文件块
        {
            if (is_equal(&hm->method, &s_post_method))
            {
                handle_mount_vfsblock(nc, hm);
            }
            else
            {
                handler_unsupported_method(nc, hm);
            }
        }
        else if (mg_vcmp(&hm->uri, NFS_SERVER_UMOUNT_VFS_URL) == 0) //卸载VFS文件块
        {
          //  printf("is_equal:%d\n", is_equal(&hm->method, &s_post_method));
            if (is_equal(&hm->method, &s_post_method))
            {
                handle_unmount_vfsblock(nc, hm);
            }
            else
            {
                handler_unsupported_method(nc, hm);
            }
        }
        else if (mg_vcmp(&hm->uri, NFS_SERVER_LIVE_PROBLE_URL) == 0)
        { //服务探针
            if (is_equal(&hm->method, &s_get_method))
            {
                handler_liveprobe_method(nc, hm);
            }
            else
            {
                handler_unsupported_method(nc, hm);
            }
        }
        else
        {
            handler_unsupported_method(nc, hm);
        }
    }
}

int main(void)
{
    if (init_config("./etc/nfs_server.conf") != 0)
    {
        printf("初始化项目配置文件失败，程序退出!\n");
        exit(EXIT_FAILURE);
    }
    // daemon(0,0);
    struct mg_mgr mgr;
    struct mg_connection *nc;
    mg_mgr_init(&mgr, NULL);
    printf("Starting web server on port %s\n", NFS_SERVER_HTTP_PORT);
    nc = mg_bind(&mgr, NFS_SERVER_HTTP_PORT, ev_handler);
    if (nc == NULL)
    {
        printf("Failed to create listener\n");
        return 1;
    }
    mg_set_protocol_http_websocket(nc);
    s_http_server_opts.document_root = "."; // Serve current directory
    s_http_server_opts.enable_directory_listing = "yes";

    for (;;)
    {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);

    return 0;
}