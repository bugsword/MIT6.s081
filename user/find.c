#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(const char *path, const char *target) {
  int fd;
  char buf[512], *p;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "can't open file:%s", path);
    return;
  }
  
  if(fstat(fd, &st) < 0) {
    fprintf(2, "can't open file stat:%s", path);
    close(fd);
    return;
  }
  switch(st.type) {
  case T_FILE:
    fprintf(2, "Input path %s must be dir path", path);
    break;
  
  case T_DIR:
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
        continue;
      }
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0) {
        printf("Can't stat %s", buf);
        continue;
      }
      if(st.type == T_FILE && strcmp(de.name, target) == 0) {
        printf("%s\n", buf);
      } else if (st.type == T_DIR) {
        find(buf, target);
      }
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[]) {
  if(argc != 3) {
    fprintf(2, "Usage: find dir file_name\n");
    exit(1);
  }
  find(argv[1], argv[2]);
  exit(0);
}
