#include <type.h>

#include <kern/syscall.h>
#include <kern/time.h>
#include <kern/process.h>

static size_t timecounter;

/*
 * 时间戳加一
 */
void
timecounter_inc(void)
{
	timecounter++;
}

/*
 * 获取内核当前的时间戳
 */
size_t
kern_get_ticks(void)
{
	return timecounter;
}

ssize_t
do_get_ticks(void)
{
	return (ssize_t)kern_get_ticks();
}

ssize_t
do_delay_ticks(int ticks)
{
	size_t now = do_get_ticks();
	proc_t *self = p_proc_ready;
	delay_object_t *item = delay_table;
	for (int i = 0; i < PCB_SIZE; ++i) {
		if (item->handle == NULL) {
			break;
		}
		++item;
	}
	item->handle = self;
	item->fut_tick = now + ticks;
	while (item->handle == self) {}
	return do_get_ticks() - now;
}
