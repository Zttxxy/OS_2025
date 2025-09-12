#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main() {
    int f2c[2];  // 父进程到子进程的管道
    int c2f[2];  // 子进程到父进程的管道
    char buf[1];
    int pid;
    int parent_pid = getpid();  // 保存父进程ID
    
    // 创建两个管道
    if (pipe(f2c) < 0 || pipe(c2f) < 0) {
        fprintf(2, "pipe creation failed\n");
        exit(1);
    }
    
    pid = fork();
    if (pid < 0) {
        fprintf(2, "fork failed\n");
        exit(1);
    } else if (pid == 0) {
        // 子进程
        close(f2c[1]);  // 关闭父到子管道的写端
        close(c2f[0]);  // 关闭子到父管道的读端
        
        // 从父进程读取数据
        read(f2c[0], buf, 1);
        close(f2c[0]);  // 关闭父到子管道的读端
        
        // 父进程ID通过管道发送，或者使用其他方式传递
        printf("%d: received ping\n", getpid());
        
        // 向父进程发送响应
        write(c2f[1], buf, 1);
        close(c2f[1]);  // 关闭子到父管道的写端
        
        exit(0);
    } else {
        // 父进程
        close(f2c[0]);  // 关闭父到子管道的读端
        close(c2f[1]);  // 关闭子到父管道的写端
        
        // 向子进程发送数据
        buf[0] = 'X';  // 任意字节数据
        write(f2c[1], buf, 1);
        close(f2c[1]);  // 关闭父到子管道的写端
        
        // 从子进程读取响应
        read(c2f[0], buf, 1);
        close(c2f[0]);  // 关闭子到父管道的读端
        
        printf("%d: received pong\n", getpid());
        
        wait(0);  // 等待子进程结束
        exit(0);
    }
}