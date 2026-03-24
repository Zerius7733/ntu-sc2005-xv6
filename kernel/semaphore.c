#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

#define MAX_SEMAPHORES 32

struct semaphore {
  struct spinlock lock;
  int value;
  int in_use;
};

struct semaphore semaphores[MAX_SEMAPHORES];
struct spinlock sem_table_lock;

void
seminit(void)
{
  initlock(&sem_table_lock, "sem_table");
  for(int i = 0; i < MAX_SEMAPHORES; i++) {
    semaphores[i].in_use = 0;
    initlock(&semaphores[i].lock, "semaphore");
  }
}

int
sem_alloc(int value)
{
  acquire(&sem_table_lock);
  for(int i = 0; i < MAX_SEMAPHORES; i++) {
    if(!semaphores[i].in_use) {
      semaphores[i].in_use = 1;
      semaphores[i].value = value;
      release(&sem_table_lock);
      return i;
    }
  }
  release(&sem_table_lock);
  return -1;  // No free semaphores
}

int
sem_free(int semid)
{
  if(semid < 0 || semid >= MAX_SEMAPHORES)
    return -1;
  
  acquire(&sem_table_lock);
  semaphores[semid].in_use = 0;
  release(&sem_table_lock);

  return 0;
}

int
sem_wait(int semid)
{
  if(semid < 0 || semid >= MAX_SEMAPHORES || !semaphores[semid].in_use)
    return -1;
 
  struct semaphore *sem = &semaphores[semid];

  acquire(&sem->lock);
  while(sem->value == 0) {
    sleep(sem, &sem->lock);
  }
  sem->value--;
  release(&sem->lock);
  return 0;
}

int
sem_signal(int semid)
{
  if(semid < 0 || semid >= MAX_SEMAPHORES || !semaphores[semid].in_use)
    return -1;

  struct semaphore *sem = &semaphores[semid];

  acquire(&sem->lock);
  sem->value++;
  wakeup(sem);
  release(&sem->lock);
  return 0;
}

int
sem_getvalue(int semid)
{
  if(semid < 0 || semid >= MAX_SEMAPHORES || !semaphores[semid].in_use)
    return -1;

  struct semaphore *sem = &semaphores[semid];
  int value;

  acquire(&sem->lock);
  value = sem->value;
  release(&sem->lock);

  return value;
}
