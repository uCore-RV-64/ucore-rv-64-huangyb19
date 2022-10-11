#include "syscall.h"
#include "defs.h"
#include "loader.h"
#include "syscall_ids.h"
#include "timer.h"
#include "trap.h"
#include "proc.h"

uint64 sys_write(int fd, char *str, uint len)
{
	debugf("sys_write fd = %d str = %x, len = %d", fd, str, len);
	if (fd != STDOUT)
		return -1;
	for (int i = 0; i < len; ++i) {
		console_putchar(str[i]);
	}
	return len;
}

__attribute__((noreturn)) void sys_exit(int code)
{
	exit(code);
	__builtin_unreachable();
}

uint64 sys_sched_yield()
{
	yield();
	return 0;
}

uint64 sys_gettimeofday(TimeVal *val, int _tz)
{
	uint64 cycle = get_cycle();
	val->sec = cycle / CPU_FREQ;
	val->usec = (cycle % CPU_FREQ) * 1000000 / CPU_FREQ;
	return 0;
}

/*
* LAB1: you may need to define sys_task_info here
*/
int sys_task_info(TaskInfo *ti)
{
	debugf("address:%p\n", ti);
	ti->status = curr_proc()->taskinfo->status;
	for (int i = 0; i < MAX_SYSCALL_NUM; i++) {
		ti->syscall_times[i] = curr_proc()->taskinfo->syscall_times[i];
	}
	uint64 cycle = get_cycle();
	uint64 current_time = (cycle / CPU_FREQ) * 1000 + \
					      (cycle % CPU_FREQ) * 1000 / CPU_FREQ;
	ti->time = current_time - curr_proc()->start_time;

	// debugf("info.status:%d\n", ti->status);
	// debugf("info.time=%d\n", ti->time);
	// debugf("info.syscall_times[SYSCALL_GETTIMEOFDAY]:%d\n", ti->syscall_times[169]);
	// debugf("info.syscall_times[SYSCALL_TASK_INFO]:%d\n", ti->syscall_times[410]);
	// debugf("info.syscall_times[SYSCALL_WRITE]:%d\n", ti->syscall_times[64]);
	// debugf("info.syscall_times[SYSCALL_YIELD]:%d\n", ti->syscall_times[124]);
	// debugf("info.syscall_times[SYSCALL_EXIT]:%d\n", ti->syscall_times[93]);
	return 0;
}

extern char trap_page[];

void syscall()
{
	struct trapframe *trapframe = curr_proc()->trapframe;
	int id = trapframe->a7, ret;
	uint64 args[6] = { trapframe->a0, trapframe->a1, trapframe->a2,
			   trapframe->a3, trapframe->a4, trapframe->a5 };
	tracef("syscall %d args = [%x, %x, %x, %x, %x, %x]", id, args[0],
	       args[1], args[2], args[3], args[4], args[5]);
	/*
	* LAB1: you may need to update syscall counter for task info here
	*/
	curr_proc()->taskinfo->syscall_times[id]++;
	
	switch (id) {
	case SYS_write:
		ret = sys_write(args[0], (char *)args[1], args[2]);
		break;
	case SYS_exit:
		sys_exit(args[0]);
		// __builtin_unreachable();
	case SYS_sched_yield:
		ret = sys_sched_yield();
		break;
	case SYS_gettimeofday:
		ret = sys_gettimeofday((TimeVal *)args[0], args[1]);
		break;
	/*
	* LAB1: you may need to add SYS_taskinfo case here
	*/
	case SYS_task_info:
		ret = sys_task_info(curr_proc()->taskinfo);
		break;
	default:
		ret = -1;
		errorf("unknown syscall %d", id);
	}
	trapframe->a0 = ret;
	tracef("syscall ret %d", ret);
}
