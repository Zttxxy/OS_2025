#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void find(char *path, char *filename);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(2, "Usage: find <path> <name>\n");
        exit(1);
    }
    
    find(argv[1], argv[2]);
    exit(0);
}

void find(char *path, char *filename) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;
    
    // 打开目录
    if ((fd = open(path, O_RDONLY)) < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }
    
    // 获取文件状态
    if (fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }
    
    // 检查是否是目录
    if (st.type != T_DIR) {
        fprintf(2, "find: %s is not a directory\n", path);
        close(fd);
        return;
    }
    
    // 确保路径长度不会溢出缓冲区
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
        fprintf(2, "find: path too long\n");
        close(fd);
        return;
    }
    
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    
    // 读取目录项
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0)
            continue;
            
        // 跳过 "." 和 ".." 目录
        if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
            continue;
            
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        
        // 获取当前文件状态
        if (stat(buf, &st) < 0) {
            fprintf(2, "find: cannot stat %s\n", buf);
            continue;
        }
        
        // 如果是目录，递归查找
        if (st.type == T_DIR) {
            find(buf, filename);
        }
        
        // 检查文件名是否匹配
        if (strcmp(de.name, filename) == 0) {
            printf("%s\n", buf);
        }
    }
    
    close(fd);
}