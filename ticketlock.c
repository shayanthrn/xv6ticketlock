// Mutual exclusion spin locks.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "ticketlock.h"

void
initlockticket(struct ticketlock *lk, char *name)
{
  lk->name = name;
  lk->ticket =0 ;
  lk->index = 0;
  lk->cpu = 0;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.
void
acquireticket(struct ticketlock *lk)
{
  pushcli(); // disable interrupts to avoid deadlock.
  if(holdingticket(lk))
    panic("acquireticket");

  myproc()->ticket=fetch_and_add(&lk->index,1);
  cprintf("ticket proc %d is : %d  and lock is : %d and lock is : %s \n",myproc()->pid,myproc()->ticket,lk->ticket,lk->name);
  while(myproc()->ticket!=lk->ticket);
  

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that the critical section's memory
  // references happen after the lock is acquired.
  __sync_synchronize();

  // Record info about lock acquisition for debugging.
  lk->cpu = mycpu();
  getcallerpcs(&lk, lk->pcs);
}

// Release the lock.
void
releaseticket(struct ticketlock *lk)
{
  if(!holdingticket(lk))
    panic("releaseticket");

  lk->pcs[0] = 0;
  lk->cpu = 0;

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that all the stores in the critical
  // section are visible to other cores before the lock is released.
  // Both the C compiler and the hardware may re-order loads and
  // stores; __sync_synchronize() tells them both not to.
  __sync_synchronize();

  // Release the lock, equivalent to lk->locked = 0.
  // This code can't use a C assignment, since it might
  // not be atomic. A real OS would use C atomics here.
  fetch_and_add(&lk->ticket,1);

  popcli();
}


// Check whether this cpu is holding the lock.
int
holdingticket(struct ticketlock *lock)
{
  int r;
  pushcli();
  r = lock->index>=lock->ticket && lock->cpu == mycpu();
  popcli();
  return r;
}


// Pushcli/popcli are like cli/sti except that they are matched:
// it takes two popcli to undo two pushcli.  Also, if interrupts
// are off, then pushcli, popcli leaves them off.
