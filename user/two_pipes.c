#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int p1[2], p2[2];
  char buf[6];

  if(pipe(p1) < 0){
    fprintf(2, "two_pipes: first pipe failed\n");
    exit(1);
  }
  if(pipe(p2) < 0){
    fprintf(2, "two_pipes: second pipe failed\n");
    close(p1[0]);
    close(p1[1]);
    exit(1);
  }

  int pid = fork();
  if(pid < 0){
    fprintf(2, "two_pipes: fork failed\n");
    close(p1[0]);
    close(p1[1]);
    close(p2[0]);
    close(p2[1]);
    exit(1);
  }

  if(pid == 0){
    int n;

    // The child receives on p1 and replies on p2.
    close(p1[1]);
    close(p2[0]);

    n = read(p1[0], buf, 4);
    if(n != 4){
      fprintf(2, "two_pipes: child read failed\n");
      close(p1[0]);
      close(p2[1]);
      exit(1);
    }
    buf[n] = 0;
    printf("%d: received %s\n", getpid(), buf);

    if(write(p2[1], "pong", 4) != 4){
      fprintf(2, "two_pipes: child write failed\n");
      close(p1[0]);
      close(p2[1]);
      exit(1);
    }

    close(p1[0]);
    close(p2[1]);
    exit(0);
  } else {
    int n;
    int status = 0;

    // The parent sends on p1 and receives on p2.
    close(p1[0]);
    close(p2[1]);

    if(write(p1[1], "ping", 4) != 4){
      fprintf(2, "two_pipes: parent write failed\n");
      status = 1;
    }
    close(p1[1]);

    if(status == 0){
      n = read(p2[0], buf, 4);
      if(n != 4){
        fprintf(2, "two_pipes: parent read failed\n");
        status = 1;
      } else {
        buf[n] = 0;
        printf("%d: received %s\n", getpid(), buf);
      }
    }

    close(p2[0]);
    if(wait(0) < 0){
      fprintf(2, "two_pipes: wait failed\n");
      status = 1;
    }
    exit(status);
  }
}
