#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if(argc != 1){
    fprintf(2, "usage: proccount\n");
    exit(1);
  }

  printf("%d\n", getproccount());
  exit(0);
}
