/** http clinet for register mount info to server 
 * refer to https://stackoverflow.com/questions/22077802/simple-c-example-of-doing-an-http-post-and-consuming-the-response  
 * refer to https://www.libhttp.org/    
 * refer to http://www.fifi.org/doc/libghttp-dev/html/ghttp.html    
 * http客户端，用于注册文件系统挂载点到服务器
 * 程序编译时需要链接libcurl库
 * 程序JSON解析库 CJSON 库 refer to https://github.com/DaveGamble/cJSON    
 * 
*/
#include <stdio.h>
#include <curl/curl.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <sys/time.h>
#include "fsutils.h"
#include "vfs.h"
#ifdef _WIN32
#define WAITMS(x) Sleep(x)
#else
/* Portable sleep for platforms other than Windows. */
#define WAITMS(x)                        \
    struct timeval wait = {0, (x)*1000}; \
    (void)select(0, NULL, NULL, NULL, &wait);
#endif
//定义请求方法

//client请求返回结果
typedef struct
{
    char *response;
    size_t len;
    size_t size;
} curl_response;

/**
 * 
 * fsclient callback 函数  
 */
static size_t fsclient_callback(void *contents, size_t size, size_t nmeemb, void *user_p)
{

    curl_response *result_t = (curl_response *)user_p;
    size_t realsize = size * nmeemb; /* calculate buffer size */
    if (result_t->size < (realsize + 1))
    {
        result_t->response = (char *)realloc(result_t->response, (realsize + 1));
        if (result_t->response == NULL)
        {
            return 0;
        }
        result_t->size = realsize + 1;
    }
    result_t->len = realsize;
    memcpy(result_t->response, contents, realsize + 1);
    result_t->response[result_t->len] = '\0';
    return realsize;
}

/*
虚拟文件系统客户端
@url 方法url
@method 方法 get|post
@body  方法请求体
*/
char *fsEasyClient(char *url, char *method, char *header, char *body)
{

    char *result;
    CURL *curl;
    CURLcode res;
    struct curl_slist *hlist_headers = NULL; // 请求头 header
    curl_response *presult = malloc(sizeof(curl_response));
    presult->size = 1024;
    presult->len = 0;
    presult->response = (char *)calloc(1, 1024);

    if (presult->response == NULL)
    {
        printf("calloc内存分配失败，程序退出!\n");
        return NULL;
    }

    char *url_location = NULL;
    if (body != NULL)
    {
        url_location = (char *)malloc(1 + strlen(url) + strlen(body));
        strcpy(url_location, url);
    }
    else
    {
        url_location = (char *)malloc(1 + strlen(url));

        strcpy(url_location, url);
    }

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (!curl)
    {
        printf("初始化curls失败，init curl failed,程序退出!!!\n");
        return NULL;
    }

    hlist_headers = curl_slist_append(hlist_headers, "Content-Type:application/json");
    hlist_headers = curl_slist_append(hlist_headers, "charset:utf-8");
    if (header != NULL) // 对NULL值进行处理
    {
        hlist_headers = curl_slist_append(hlist_headers, header);
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hlist_headers); //设置http请求头
    curl_easy_setopt(curl, CURLOPT_URL, url_location);         //设置方法 路径
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fsclient_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, presult);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    if (strcmp(method, HTTP_METHOD_GET) == 0) //处理HTTP GET方法
    {
    }
    else if (strcmp(method, HTTP_METHOD_POST) == 0) //处理HTTP POST方法
    {
        curl_easy_setopt(curl, CURLOPT_POST, 1);

        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(body));
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    }
    else if (strcmp(method, HTTP_METHOD_PATCH) == 0) //TODO:处理HTTP PATCH方法
    {
        /* code */
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, HTTP_METHOD_PATCH);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(body));
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    }
    else if (strcmp(method, HTTP_METHOD_PUT) == 0) //处理HTTP PUT方法
    {
        curl_easy_setopt(curl, CURLOPT_PUT, 1);
    }
    else if (strcmp(method, HTTP_METHOD_DELETE) == 0) //处理 HTTP DELETEF方法
    {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, HTTP_METHOD_DELETE);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(body));
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    }
    else
    {
        printf("fsclient方法仅支持GET，POST，PATCH，PUT，DELETE，当前请求方法未知，程序退出！！！\n");
        return NULL;
    }
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        //TODO: 处理错误
        printf("libcurl 执行请求%s失败，错误信息为:%s\n", url_location, curl_easy_strerror(res));

        return NULL;
    }
    {
        //TODO: 处理函数返回值
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    result = (char *)malloc(sizeof(presult->response));
    result = presult->response;
    free(url_location);
    free(presult);

    return result;
}

/*
虚拟文件系统客户端
@handler_count  请求handler 个数
@curl_hanlders 请求handlers
@body  方法请求体
*/
int fsMultiClient(int handler_count, CURL *curl_handlers[])
{
    CURLM *curlmulti;
    int running_curl = 0;
    int repeat = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    for (int i = 0; i < handler_count; i++)
    {
        curl_handlers[i] = curl_easy_init;
    }

    /* init a multi stack */
    curlmulti = curl_multi_init(); //初始化curlmutil

    for (int i = 0; i < handler_count; i++)
    {
        /* add the individual transfers */
        curl_multi_add_handle(curlmulti, curl_handlers[i]);
    }
    /* we start some action by calling perform right away */
    curl_multi_perform(curlmulti, &running_curl);
    while (running_curl)
    {
        //struct timeval timeout;
        // long curl_timeo=-1;
        CURLMcode mcode; /* curl_multi_wait() return code */
        int numfds;
        /* wait for activity, timeout or "nothing" */
        mcode = curl_multi_wait(curlmulti, NULL, 0, 1000, &numfds);
        if (mcode != CURLM_OK)
        {
            printf("mutil curl wait error,error info is:%s\n", curl_multi_strerror(mcode));
            break;
        }
        /* 'numfds' being zero means either a timeout or no file descriptors to
       wait for. Try timeout on first occurrence, then assume no file
       descriptors and no file descriptors to wait for means wait for 100
       milliseconds. */

        if (!numfds)
        {
            repeat++; /* count number of repeated zero numfds */
            if (repeat > 1)
            {
                WAITMS(100); ///* sleep 100 milliseconds */
            }
        }
        else
        {
            repeat = 0;
            curl_multi_perform(curlmulti, &running_curl);
        }
    }
    //curl_multi_cleanup(curlmulti);
    for (int i = 0; i < handler_count; i++)
    {

        curl_multi_remove_handle(curlmulti, curl_handlers[i]);
        curl_easy_cleanup(curl_handlers[i]);
    }
    curl_multi_cleanup(curlmulti);
    curl_global_cleanup();

    return 0;
}
