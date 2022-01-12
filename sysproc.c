#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int 
sys_calculate_sum_of_digits(void) {
  int number = myproc()->tf->ebx;
  cprintf("KERNEL = SYS calculate sum of digits. Call for number %d\n", number);
  return calculate_sum_of_digits(number);
}

int
sys_get_parent_pid(void) {
  return get_parent_pid();
}

void 
sys_set_process_parent(void) {
  int pid = myproc()->tf->ebx;
  cprintf("KERNEL = SYS set process parent. Call for pid %d\n", pid);
  return set_process_parent(pid);
} 

void
sys_print_process(void) {
  print_process();
}

void
sys_set_mhrrn_param_process(void) {
  int pid, priority;

  if(argint(0, &pid) < 0)
    return;
  
  if(argint(1, &priority) < 0)
    return;
  
  set_mhrrn_param_process(pid, priority);
}

void
sys_set_mhrrn_param_system(void) {
  int priority;

  if(argint(0, &priority) < 0)
    return;

  set_mhrrn_param_system(priority);
}

void 
sys_set_level(void) {
  int pid, level;

  if(argint(0, &pid) < 0)
    return;

  if(argint(1, &level) < 0)
    return;

  set_level(pid, level);
}

void
sys_sem_init(void) 
{
  int i, v;
  
  if(argint(0, &i) < 0)
    return;

  if(argint(1, &v) < 0)
    return;

  sem_init(i, v);
}

void
sys_sem_acquire(void)
{
  int i;

  if(argint(0, &i) < 0)
    return;
    
  sem_acquire(i);
}

void
sys_sem_release(void)
{ 
  int i;

  if(argint(0, &i) < 0)
    return;
    
  sem_release(i);
}

