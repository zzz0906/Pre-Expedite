#include <errno.h>
#include <malloc.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <syslog.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include "confuse.h"
#include "fsclient.h"
#include "vfs.h"

/**
 *  @brief Function isdigitstr() 判断传入字符串是否全数字
 *  @param[in]   char *str     字符串
 *  @retval  1: 全字符串，0:非全字符串     
 *  @pre  
 *  @post 
 */
 int isdigitstr(char *str)
{
    return (strspn(str, "0123456789") == strlen(str));
}
// parse file param function ie 100g 100t 100G 100g parse to 100g 100*1024g first parse G T --to g t
int64_t ParseFileSizeParam(char *char_size)
{
    if (isdigitstr(char_size))
    { // no unit in request size ,ie return -1
        if ((int64_t)atol(char_size) <= 1024)
        {
            return (int64_t)atol(char_size);
        }
        return -1;
    }

    int char_size_length = strlen(char_size);
    char unit = char_size[char_size_length - 1]; // get the unit of request size i.e g,G,t,T

    char char_size_number[char_size_length - 1];

    memcpy(char_size_number, char_size, (char_size_length - 1));
    char_size_number[char_size_length - 1] = '\0';
    if (!isdigitstr(char_size_number)) // check the param is all numbers
    {
        return -1; // param number is wrong i.e 1s1313g
    }

    if (tolower(unit) == 'g')
    { // param is g | G
        return (int64_t)atol(char_size_number);
    }
    else if (tolower(unit) == 't') // param is t |T
    {
        /* code */
        return (int64_t)atol(char_size_number) * 1024; // parse tb to gb;
    }
    else
    {
        printf("转换用户参数size失败: = %d,错误信息为: %s\n", errno, strerror(errno));
        return -1; // error
    }
    return -1;
}


char *GetUserName()
{
    uid_t uid = getuid(); //在chmod u+s 会改变getuid返回值

    struct passwd *pw = getpwuid(uid);
    if (pw)
    {
        return pw->pw_name;
    }

    return NULL;
}

//系统调用，直接采用system方式调用/sbin/xxx 会会出现权限问题，采用fork exec替换
int system_alternative(char *pgm, char *argv[])
{
    pid_t pid = fork();
    if (pid > 0)
    {
        // We're the parent, so wait for child to finish
        int status;
        waitpid(pid, &status, 0);
        return status;
    }
    else if (pid == 0)
    {
        // We're the child, so run the specified program.  Our exit status will
        // be that of the child program unless the execv() syscall fails.
        return execv(pgm, argv);
    }
    else
    {
        // Something horrible happened, like system out of memory
        return -1;
    }
}

void skeleton_daemon()
{
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if (pid < 0)
    {
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
    }
    else
    {

        exit(EXIT_SUCCESS);
    }
}

void kill_daemon(int pid)
{
    int status;
    errno = 0;
    if (kill(pid, SIGTERM) != 0)
    {
        printf("杀死进程%d出错，错误信息为:%s\n", pid, strerror(errno));
        return;
    }
    else
    {

        wait(&status);
        if (WIFSIGNALED(status))
        {
            printf("chile process receive signal %d\n", WTERMSIG(status));
        }
        printf("当前程序进程%d及其子进程已经被杀死！\n", pid);
    }
}
//初始化守护进程
//refer to https://stackoverflow.com/questions/17954432/creating-a-daemon-in-linux
int Init_Daemon()
{
    pid_t child_pid;
    //fork off the parent process
    child_pid = fork();
    if (child_pid < 0)
    {
        printf("fork 子进程失败,程序退出!\n");
        goto error;
    }
    /* Success: Let the parent terminate */
    if (child_pid > 0)
    {
        exit(EXIT_SUCCESS);
    }
    /* On success: The child process becomes session leader */
    if (setsid() < 0)
    {
        goto error;
    }
    /* Catch, ignore and handle signals */
    //TODO: Implement a working signal handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    /* Fork off for the second time*/
    child_pid = fork();
    /* An error occurred */
    if (child_pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (child_pid > 0)
        exit(EXIT_SUCCESS);
    /* Set new file permissions */
    umask(0);

    //TODO: 创建子进程
    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    chdir("/");

    /* Close all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x >= 0; x--)
    {
        close(x);
    }
    printf("守护子进程为-->%d", child_pid);
    return 0;
error:
    exit(EXIT_FAILURE);
    return -1;
}

int VFSFileDelete(char *user, char *filename)
{
    printf("确定要删除用户%s文件%s 吗？ [y|n]\n", user, filename);
    char ch = getchar();
    if (tolower(ch) != 'y')
    {
        printf("请确认是否要删除该文件%s\n", filename);
        return -1;
    }
    if (remove(filename) == 0)
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

char *StringConcat(char *s1, char *s2)
{
    // alloca() 使用alloca()函数
    char *ns = malloc(strlen(s1) + strlen(s2) + 1);     
    ns[0] = '\0';
    strcat(ns, s1);
    strcat(ns, s2);
    return ns;
}

//当前目录是否属于当前所有者
//
int IsDirBelongToUser(char *dir, char *user)
{
    assert(dir);
    assert(user);

    struct stat info;
    stat(dir, &info);
    struct passwd *upwd = getpwuid(info.st_uid); //在此处会有隐藏问题，采用此语句后会修改 getpwuid的值 user 若采用是 getpwuid方式会改变user的值

    if (upwd != NULL)
    {

        if (strcmp(upwd->pw_name, user) == 0)

        {
            return 0;
        }
        return -1;
    }
    return -1;
    ;
}

//格式化处理文件挂载点，将文件路径最后一个'/'字符去掉
char *formartVfsMountPoint(char *mountpoint)
{   
   
    assert(mountpoint);
    char last_character = mountpoint[strlen(mountpoint) - 1];
   
    if (last_character == '/')
    {   
  
        char *formated = malloc(sizeof(char)*(strlen(mountpoint)-1));
      
          memcpy(formated,mountpoint,strlen(mountpoint)-1);
          formated[strlen(mountpoint)-1]='\0';
      
        return formated;
    }
   
    return mountpoint;
}


//判断给定的文件夹是否为绝对路径
int isDirAbsolute(char *dir){
    assert(dir);
    char first_charactor = dir[0];
    if (first_charactor == '/'){
        return 0;
    }else{
        return -1;
    }
}


//处理在NFS服务器中挂载时去掉挂载路径的首字符‘/’
char *formartNFSMountPoint(char *mountpoint)
{
    assert(mountpoint);
    char first_charactor = mountpoint[0];
    if (first_charactor == '/')
    {
        char *formated = malloc(sizeof(char) * (strlen(mountpoint) - 1));
        formated = strcpy(formated, mountpoint + 1);
        //formated[strlen(mountpoint) - 1] = '\0';
        return formated;
    }
    return mountpoint;
}