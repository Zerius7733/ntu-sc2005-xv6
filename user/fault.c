#include "kernel/types.h"
#include "kernel/riscv.h"
#include "user/user.h"

static void
usage(void)
{
  fprintf(2, "usage: fault null|heap\n");
  exit(1);
}

int
main(int argc, char *argv[])
{
  volatile char *p;

  if(argc != 2)
    usage();

  if(strcmp(argv[1], "null") == 0){
    p = (char *)0;
    *p = 1;
  } else if(strcmp(argv[1], "heap") == 0){
    p = sbrk(PGSIZE);
    if((uint64)p == 0xffffffffffffffffL){
      printf("sbrk failed\n");
      exit(1);
    }
    p[PGSIZE] = 1;
  } else {
    usage();
  }

  exit(0);
}
