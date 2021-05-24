#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"
//#include "kernel/fs.h"

int
main(int argc, char *argv[]) {
  if(argc < 2) {
    fprintf(2, "Usage: xxx | xargs xxx\n");
    exit(1);
  } 
  int start_idx = 1;
  if(strcmp(argv[1], "-n") == 0) { 
    if(strcmp(argv[2], "1") == 0) 
      start_idx = 3;
    else {
      fprintf(2, "cmd:%s %s %s Usage: xargs -n 1\n", argv[0], argv[1], argv[2]);
      exit(1);
    }
  }
  
  char cmd[512], *cmd_argv[MAXARG];
  strcpy(cmd, argv[start_idx]); 
  cmd_argv[0] = cmd;
  for(int i = start_idx+1, j=1; i < argc; ++i) {
    char t[512];
    strcpy(t, argv[i]);
    cmd_argv[j] = t;
  }
  char tmp_c, final_args[512];
  int cc;
  while((cc = read(0, &tmp_c, 1)) > 0 || strlen(final_args) > 0) {
    if(tmp_c == '\n' || cc == 0) {//换行符或者终止符号
      if(fork() == 0) {
        cmd_argv[argc - start_idx] = final_args;
        exec(cmd, cmd_argv);
        fprintf(2, "exec error") ;
      }
      wait(0);
      memset(final_args, 0, 512);
    } else 
      final_args[strlen(final_args)] = tmp_c;
  }
  exit(0);
}
