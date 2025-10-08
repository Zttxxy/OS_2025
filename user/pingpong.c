#include "kernel/types.h"
#include "user/user.h"

/*
    父进程:    创建管道 → fork → 发送ping → 等待pong → 打印 → 退出
    子进程:               → 等待ping → 打印 → 发送pong → 退出
*/

int main(int argc, char *argv[]) {
    //[0]读端，[1]写端  
    int f2c[2];  // parent to child
    int c2f[2];  // child to parent
    
    if (pipe(f2c) < 0 || pipe(c2f) < 0) {
        fprintf(2, "pipe failed\n");
        exit(1);
    }
    
    int pid = fork();
    if (pid < 0) {
        fprintf(2, "fork failed\n");
        exit(1);
    }
    
    if (pid == 0) {  // child process
        close(f2c[1]);  // close write end of parent-to-child 子进程只读
        close(c2f[0]);  // close read end of child-to-parent 子进程只写
        

        // read ping from parent, read会阻塞直到父进程写入数据
        int parent_pid;
        read(f2c[0], &parent_pid, sizeof(parent_pid));
        close(f2c[0]);
        
        printf("%d: received ping from pid %d\n", getpid(), parent_pid);
        

        // send pong to parent
        int child_pid = getpid();
        write(c2f[1], &child_pid, sizeof(child_pid));// write 会阻塞直到父进程读取数据,向字->父管道写入子进程PID
        close(c2f[1]);
        
        exit(0);
    } else {  // parent process
        close(f2c[0]);  // close read end of parent-to-child， 父进程只写
        close(c2f[1]);  // close write end of child-to-parent， 父进程只读
        
        int parent_pid = getpid();
        write(f2c[1], &parent_pid, sizeof(parent_pid));//父进程发送自己的PID
        close(f2c[1]);
        
        int child_pid;
        read(c2f[0], &child_pid, sizeof(child_pid));//等待子进程发送PID
        close(c2f[0]);
        
        printf("%d: received pong from pid %d\n", getpid(), child_pid);
        
        wait(0);
        exit(0);
    }
}