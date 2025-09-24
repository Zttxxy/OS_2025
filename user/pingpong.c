#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
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
        close(f2c[1]);  // close write end of parent-to-child
        close(c2f[0]);  // close read end of child-to-parent
        
        int parent_pid;
        read(f2c[0], &parent_pid, sizeof(parent_pid));
        close(f2c[0]);
        
        printf("%d: received ping from pid %d\n", getpid(), parent_pid);
        
        int child_pid = getpid();
        write(c2f[1], &child_pid, sizeof(child_pid));
        close(c2f[1]);
        
        exit(0);
    } else {  // parent process
        close(f2c[0]);  // close read end of parent-to-child
        close(c2f[1]);  // close write end of child-to-parent
        
        int parent_pid = getpid();
        write(f2c[1], &parent_pid, sizeof(parent_pid));
        close(f2c[1]);
        
        int child_pid;
        read(c2f[0], &child_pid, sizeof(child_pid));
        close(c2f[0]);
        
        printf("%d: received pong from pid %d\n", getpid(), child_pid);
        
        wait(0);
        exit(0);
    }
}