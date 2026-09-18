#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int p[2];
  char buf[5];
  
  if(pipe(p) < 0){
    fprintf(2, "pingpong: pipe failed\n");
    exit(1);
  }
  int pid = fork();
  if(pid < 0){
    fprintf(2, "pingpong: fork failed\n");
    close(p[0]);
    close(p[1]);
    exit(1);
  }
  if(pid == 0){
    // Child
    for (int j=0; j<5; j++) {
      char* message;
      if ((j % 2) == 0) {
        message = "ping";
      } else {
        message = "pong";
      }
      if(write(p[1], message, 4) != 4){
        fprintf(2, "pingpong: write failed\n");
        close(p[0]);
        close(p[1]);
        exit(1);
      }
    }
    close(p[0]);
    close(p[1]);
    exit(0);
  } else {
    // Parent
    for (int j=0; j<5; j++) {
      if(read(p[0], buf, 4) != 4){
        fprintf(2, "pingpong: read failed\n");
        close(p[0]);
        close(p[1]);
        wait(0);
        exit(1);
      }
      buf[4] = 0;
      printf("%d: received %s\n", getpid(), buf);
    }

    close(p[0]);
    close(p[1]);
    wait(0);
    exit(0);
  }
}
