#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

#define MAX_STARVATION 8000

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  p->level = 2;
  p->last_exec = ticks;
  p->arrival_time = ticks;
  p->exec_time = 1;
  // cprintf("NEW PROCESS\n");
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }
  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

// Round Robin

// int rr_is_empty(void) {
//   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
//     if(p->level == 1)
//       return 1;
//   }
//   return 0;
// }

static struct proc *rr_last_proc = &ptable.proc[NPROC] - 1;

struct proc *rr_get_sched_proc() { // working ??
  struct proc *p = rr_last_proc;

  do{
    p++;
    if(p == &ptable.proc[NPROC])
      p = ptable.proc;
    if(p->state != RUNNABLE || p->level != 1)
      continue;

    rr_last_proc = p;
    return p;
  } while (p != rr_last_proc);

  rr_last_proc = &ptable.proc[NPROC] - 1;
  return 0;
}

struct proc *lcfs_get_sched_proc() { // not working
  struct proc *p;
  struct proc *res = 0;
  int mx_arrival = -1;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != RUNNABLE || p->level != 2)
      continue;

    if(mx_arrival < p->arrival_time) {
      res = p;
      mx_arrival = p->arrival_time;
    }
  }
  return res;
}

struct proc *mhrrn_get_sched_proc() {
  struct proc *p;
  struct proc *res = 0;
  int mx_mhrrn = -1;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state != RUNNABLE || p->level != 3)
      continue;

    int p_mhrrn = ((((ticks - p->arrival_time) + p->exec_time) / p->exec_time) + p->mhrrn_prior) / 2;
    if(mx_mhrrn < p_mhrrn){
      res = p;
      mx_mhrrn = p_mhrrn;
      cprintf("MHRRN"); cprintf("%d", p->pid); cprintf(" "); cprintf(p->name); cprintf("\n");
    }
  }
  return res;
}

struct proc *get_sched_proc() {
  struct proc *p;
  p = rr_get_sched_proc();
  if(p)
    return p;
  p = lcfs_get_sched_proc();
  if(p) {
    return p;
  }
  p = mhrrn_get_sched_proc();
  if(p) {
    return p;
  }
  return 0;
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.

void age() {
  struct proc *p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;

    if(ticks - p->last_exec > MAX_STARVATION && p->level > 1) {
      p->level = 1;
      cprintf("AGE: ");cprintf("%d", p->pid);cprintf(" ");cprintf(p->name);cprintf("\n");
    }
  }
}

// void check() {
//   struct proc *p;
//   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
//     if(p->state == UNUSED)
//       continue;

//   }
// }

void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);

    age();
    p = get_sched_proc();
    if(p != 0) {
      // cprintf("FLAG ");cprintf(p->name);cprintf("\n");
      p->last_exec = ticks;
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();
      p->exec_time += ticks - p->last_exec;
      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    else {
      // cprintf("NO PROCESS RUNING\n");
    }
    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

int
calculate_sum_of_digits(int number)
{
  int result = 0;
  while(number) {
    result += number % 10;
    number /= 10;
  }
  return result;
}

int 
get_parent_pid() {
  struct proc *p = myproc()->parent;
  cprintf("Process %d real parent is: %d\n", myproc()->pid, p->pid);
  while (p->isvirt) {
    p = p->real_parent;
  }
  return p->pid;
}

struct proc*
get_proc(int pid) {
  struct proc* p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if(p->pid == pid)
      return p;
  }
  panic("There is no process with this pid.");
}

void 
set_process_parent(int pid) {
  struct proc* p = get_proc(pid);
  struct proc* myp = myproc();
  myp->isvirt = 1;
  myp->real_parent = p->parent;
  p->parent = myproc();
  cprintf("proc %d parent changed to %d\n", p->pid, myp->pid);
}

char*
get_state_string(int state)
{
  if (state == 0) {
    return "UNUSED";
  }
  else if (state == 1) {
    return "EMBRYO";
  }
  else if (state == 2) {
    return "SLEEPING";
  }
  else if (state == 3) {
    return "RUNNABLE";
  }
  else if (state == 4) {
    return "RUNNING";
  }
  else if (state == 5) {
    return "ZOMBIE";
  }
  else {
    return "";
  }
}

int
get_int_len(int num)
{
  int len = 0;
  if (num == 0)
    return 1;
  while (num > 0) {
    len++;
    num = num / 10;
  }
  return len;
}

void 
print_process(void)
{
  struct proc *p;
  int i = 0;

  cprintf("name");
  for(i = 0; i < 18 - 4; i++)
    cprintf(" ");
  cprintf("pid");
  for(i = 0; i < 6 - 3; i++)
    cprintf(" ");
  cprintf("state");
  for(i = 0; i < 10 - 5; i++)
    cprintf(" "); 
  cprintf("level");
  for(i = 0; i < 8 - 5; i++)
    cprintf(" ");
  cprintf("cycle");
  for(i = 0; i < 10 - 7; i++)
    cprintf(" ");
  cprintf("arrive");
  for(i = 0; i < 10 - 7; i++)
    cprintf(" ");
  cprintf("age");
  for(i = 0; i < 8 - 3; i++)
    cprintf(" ");
  cprintf("HRRN");
  for(i = 0; i < 2; i++)
    cprintf(" ");
  cprintf("\n");
  for(i = 0; i < 10*8 - 6; i++)
    cprintf(".");
  cprintf("\n");


  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){ 
    if (p->state != UNUSED) {
      cprintf(p->name);
      for(i = 0; i < 18 - strlen(p->name); i++)
        cprintf(" ");
      cprintf("%d",p->pid);
      for(i = 0; i < 6 - get_int_len(p->pid); i++)
        cprintf(" ");
      cprintf(get_state_string(p->state));
      for(i = 0; i < 12 - strlen(get_state_string(p->state)); i++)
        cprintf(" ");
      
      cprintf("%d", p->level);
      for(i = 0; i < 8 - get_int_len(p->level); i++)
        cprintf(" ");
      cprintf("%d", p->exec_time);
      for(i = 0; i < 7 - get_int_len(p->exec_time); i++)
        cprintf(" ");
      cprintf("%d", p->arrival_time);
      for(i = 0; i < 8 - get_int_len(p->arrival_time); i++)
        cprintf(" ");
      cprintf("%d", ticks - p->last_exec);
      for(i = 0; i < 8 - get_int_len(ticks - p->last_exec); i++)
        cprintf(" ");
      int p_hrrn = (((ticks - p->arrival_time) + p->exec_time) / p->exec_time);
      cprintf("%d", p_hrrn);
      for(i = 0; i < 7 - get_int_len(p->last_exec); i++)
        cprintf(" ");
      cprintf("\n");
    }
  }
  release(&ptable.lock);
}

void
set_mhrrn_param_process(int pid, int priority) {
  struct proc* p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if(p->pid == pid)
      p->mhrrn_prior = priority;
  }
}

void
set_mhrrn_param_system(int priority) {
  struct proc* p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    p->mhrrn_prior = priority;
  }
}

void 
set_level(int pid, int level) {
  struct proc* p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if(p->pid == pid)
      p->level = level;
  }
}
