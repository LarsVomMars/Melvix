#ifndef MELVIX_PROCESS_H
#define MELVIX_PROCESS_H

#include <stdint.h>
#include <memory/paging.h>
#include <interrupts/interrupts.h>

struct mmap {
	u32 text;
	u32 bss;
	u32 data;
	u32 stack;
};

struct process {
	struct page_directory *cr3;
	struct regs registers;

	u32 pid;
	u32 gid;
	char *name;

	int state;
	int thread;

	u32 stdin;
	u32 stdout;
	u32 stderr;

	u32 brk;
	u32 handlers[6];

	struct process *parent;
	struct process *next;
};

void scheduler(struct regs *regs);

void process_kill(u32 pid);

u32 process_spawn(struct process *process);

void process_suspend(u32 pid);
void process_wake(u32 pid);
u32 process_child(struct process *process, u32 pid);
u32 process_fork(u32 pid);

int process_wait_gid(u32 gid, int *status);
int process_wait_pid(u32 pid, int *status);

struct process *process_from_pid(u32 pid);

void process_init(struct process *proc);

struct process *process_make_new();

u32 kexec(char *path);

u32 uexec(char *path);

extern struct process *current_proc;

extern u32 stack_hold;

#define PID_NOT_FOUND ((struct process *)0xFFFFFFFF)

#define PROC_RUNNING 0
#define PROC_ASLEEP 1

#define PROC_THREAD 0
#define PROC_PROC 1
#define PROC_ROOT 2

#define WAIT_ERROR (-1)
#define WAIT_OKAY 0

#endif