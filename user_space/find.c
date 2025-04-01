#include "ulib/user_all.h"
// #include <fcntl.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/stat.h>
// #include <sys/types.h>
// #include <sys/wait.h>
// #include <unistd.h>

char *fmtname(char *path) {
    static char buf[DIRSIZ + 1];
    char *p;

    // Find first character after last slash.
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return blank-padded name.
    if (strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
    return buf;
}

int file_name_cmp(char *s1, char *s2) {
    while (*s1 == *s2 && *s1 != '\0') {
        s1++;
        s2++;
    }
    return !((*s1 == *s2 && *s1 == '\0') || (*s1 == '\0' && *s2 == ' ') ||
             (*s1 == ' ' && *s2 == '\0'));
}

void find(char *path, char *target) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, 0)) < 0) {
        printf("find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0) {
        printf("find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch (st.type) {
    case T_FILE:
        // printf("FILE : %s\n", path);
        if (!file_name_cmp(fmtname(path), target)) {
            printf("%s\n", path);
        }
        break;

    case T_DIR:
        // printf("DIR : %s\n", path);
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf) {
            printf("find: path too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';
        while (read(fd, &de, sizeof(de)) == sizeof(de)) {
            if (de.inum == 0) {
                continue;
            }
            if (de.name[0] == '.') {
                // printf("E\n");
                continue;
            }
            // printf("de.name : %s END\n", de.name);
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            if (stat(buf, &st) < 0) {
                printf("find: cannot stat %s\n", buf);
                continue;
            }
            if (st.type != T_DIR) {
                // printf("DIR FILE : %s END\n", buf);
                // printf("fmtname : %s END\n", fmtname(buf));
                if (!file_name_cmp(fmtname(buf), target)) {
                    printf("%s\n", buf);
                }
            } else {
                // printf("DIR DIR BEGIN : %s END\n", buf);
                find(buf, target);
                // printf("DIR DIR END : %s END\n", buf);
            }
        }
        break;
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("enter a path and a target name\n");
        exit(0);
    }
    find(argv[1], argv[2]);

    exit(0);
}
