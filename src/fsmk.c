/**
 * refer to https://blog.csdn.net/qq_38374864/article/details/72903920
 * https://www.quora.com/What-is-the-difference-between-open-and-fopen-in-C-How-are-they-different-with-each-other-in-terms-of-internal-working 
 * https://blog.csdn.net/kaiandshan/article/details/43226529    
 * https://stackoverflow.com/questions/1658476/c-fopen-vs-open  
 * https://jameshfisher.com/2017/02/24/what-is-mode_t.html
 * 
 * 
*/

#define _LARGEFILE64_SOURCE /* for large file handler*/
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h> /* strerror */
#include <string.h>
#include <stdint.h>
#include <sys/wait.h>
#include "fsutils.h"
#define MAX_SIZE 1024 * 10 // set max size to 10TB
#define MODE_PERMS S_IRUSR | S_IWUSR |S_IXUSR | S_IRGRP | S_IROTH
/**
 * 
 * 根据指定文件名和文件大小创建文件，并格式化该文件系统为虚拟文件系统，当前支持格式为xfs文件系统格式
 * 创建文件大小上限为10TB
 * 默认创建目录在用户home目录下/vfs/home/[fllename]
 * get  request filesystem size 
 * open file and  create fileblock
 * use  large file  size --> mb
 * if size means GB then create size*1024 if size means TB then create size**1024*1024GB
 */
int VFSBlockCreate(char *fb_name, int64_t vfs_size)
{
    char mkfs_cmd[255]; //当目录路径特别长时，会出现buffer overflow 错误，因此将字符串长度调大
    if (vfs_size > MAX_SIZE)
    { 
        printf("文件大小必须小于10TB\n");
        return -1;
    }
    errno = 0;                         
    if ((access(fb_name, F_OK)) != -1) // if file exists return
    {
        printf("files %s is already exists,exit the program!!!,errono is %d\n", fb_name,errno);
        return errno;
    }
    else
    {
        errno = 0;                             // reset errno
        int fd = creat64(fb_name, MODE_PERMS); // if file is not exists then create it
        if (fd == -1)
        {
            printf("文件系统打开错误码为: = %d,错误信息为: %s\n", errno, strerror(errno));
            return errno;
        }
        errno = 0;
        ftruncate(fd, vfs_size * 1024 * 1024 * 1024); //write the x-size file  GB
        close(fd);
    }

    errno = 0;                                                      //reset errno;
    if ((sprintf(mkfs_cmd, "mkfs.%s -f %s", "xfs", fb_name) == -1)) //make vitrual filesystem to be xfs   //TODO: support more filesystem types for example ext4
    {
        printf("mkfs.xfs命令调用错误，错误码为： %d,错误信息为: %s\n", errno, strerror(errno));
        return errno;
    }
    errno = 0;
    int mkfs_result = 0;
    char *argv[] = {"-f", fb_name,NULL};
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
        //return execv("mkfx.xfs", argv);
        //char *argv[] = {NULL,NULL};
        //  execv("pwd",argv);

        mkfs_result = execv("/sbin/mkfs.xfs", argv);
    }
    else
    {
        // Something horrible happened, like system out of memory
        return -1;
    }
    //TODO: 是否需要wait
    if (mkfs_result < 0)
    {
        printf("cmd: %s\t error: %s\n", mkfs_cmd, strerror(errno)); // 这里务必要把errno信息输出或记入Log
        return mkfs_result;
    }

    if (WIFEXITED(mkfs_result))
    {
       // printf("normal termination, exit status = %d,error info is:%s\n", WEXITSTATUS(mkfs_result), strerror(mkfs_result)); //取得cmdstring执行结果
        return WEXITSTATUS(mkfs_result);
    }
    else if (WIFSIGNALED(mkfs_result))
    {
        //printf("abnormal termination,signal number =%d,error info is:%s\n", WTERMSIG(mkfs_result), strerror(mkfs_result)); //如果cmdstring被信号中断，取得信号值
        return WTERMSIG(mkfs_result);
    }
    else if (WIFSTOPPED(mkfs_result))
    {
        //printf("process stopped, signal number =%d,error info is:%s\n", WSTOPSIG(mkfs_result), strerror(mkfs_result)); //如果cmdstring被信号暂停执行，取得信号值
        return WSTOPSIG(mkfs_result);
    }

    return 0;
}
