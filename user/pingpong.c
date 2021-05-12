#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char * argv[]) {
  int p1[2], p2[2];   
  if(pipe(p1) < 0 || pipe(p2) < 0) {
    fprintf(2, "Pipe create falied\n");
    exit(1);
  }

  if(fork() == 0) {
    char c;
    read(p1[0], &c, 1);
    //printf("%d: received ping %c p1:(%d %d) \n", getpid(), c, p1[0], p1[1]); 
    printf("%d: received ping\n", getpid());
    c += 1;
    write(p2[1], &c, 1);
    close(p2[1]);
    exit(0);
  } 
  
  char c = 'a';
  write(p1[1], &c, 1);
  close(p1[1]);

  wait(0);
  read(p2[0], &c, 1);
  //printf("%d: received pong %c p2:(%d %d) \n", getpid(), c, p2[0], p2[1]); 
  printf("%d: received pong\n", getpid());
  exit(0);
}
