#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void panic(char *s) {
  fprintf(2, "%s\n", s);
  exit(1);
}

void 
runcmd(){
  int q[2];
  if(pipe(q) < 0) 
	panic("pipe");	
  
  int number1, number2;
  int init = 0;
  int cnt = 0;
  while(read(0, &number2, 4)){
	if (cnt == 0) {
	  number1 = number2;
  	  printf("prime %d\n", number1);
      cnt = 1;
	  continue;
    }
    if(number2 % number1 == 0) 
  	  continue;   //drop 
    else if (init == 0) {
      int fid = fork();
      if (fid < 0) {
        panic("fork");
      }
      if (fid == 0) {
        close(0);
		dup(q[0]);
        close(q[0]);
        close(q[1]);
        runcmd();
      }
	  init = 1;
    }
    write(q[1], &number2, 4);
  }

  close(q[0]);	
  close(q[1]);	
  //if (init == 1) 
  wait(0);
  //printf("PID:%d child pid:%d\n", getpid(), wait(0));
  exit(0);
}

int 
main(int argc, char *argv){
  int p[2];
  if(pipe(p) < 0) {
    fprintf(2, "Error to create pipe.\n");
    exit(1);
  }
 
  int fid = fork(); 
  if (fid < 0) 
	panic("fork in main");
  else if (fid == 0) {
    close(0);
    dup(p[0]);
    close(p[0]);
    close(p[1]);
    runcmd();
  }
  else {
    int number = 2;
    for(int i = number; i <= 35; ++i) {
	  if(i == 2 || i % number != 0) {
		write(p[1], &i, 4);
		//printf("now is %d\n", i);
      }
    }
  }
  close(p[0]);
  close(p[1]);
  wait(0);
  exit(0);
}



