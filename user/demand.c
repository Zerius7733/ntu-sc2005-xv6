#include "kernel/types.h"
#include "kernel/riscv.h"
#include "user/user.h"

int
main(void)
{
  int i;
  char *base;

  base = sbrk(30 * PGSIZE);
  if((uint64)base == 0xffffffffffffffffL){
    printf("sbrk failed\n");
    exit(1);
  }

  printf("before touch: vp=%d pp=%d\n", countvp(), countpp());
  for(i = 0; i < 30; i++)
    base[i * PGSIZE] = i;
  printf("after touch: vp=%d pp=%d\n", countvp(), countpp());

  exit(0);
}
