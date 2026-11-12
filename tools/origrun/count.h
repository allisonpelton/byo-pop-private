// The --count counters in the sound stand-in (count.c), and the block of shared memory origrun reads them from.
#pragma once
#include <stdint.h>

#define COUNT_MAGIC     0x4E43524Fu    // 'ORCN'
#define COUNT_MAXFUNC   16             // functions counted
#define COUNT_MAXTHREAD 16             // threads that run counted functions
#define COUNT_NAMELEN   24

// One thread's numbers. The thread that runs the function is the only writer: it makes seq odd while it writes and even
// when it is done, and a reader takes the copy again when seq was odd or changed (a 64-bit number is two stores).
typedef struct {
	volatile uint32_t seq;
	uint32_t tid;
	volatile uint64_t calls[COUNT_MAXFUNC];
	volatile uint64_t cycles[COUNT_MAXFUNC];    // the function's own cycles on this thread: its time less the counted functions it called
	volatile uint64_t readTsc;                  // time-stamp ticks spent in the cycle readings of the calls counted so far (two a call), in the running game
} CountThread;

typedef struct {
	uint32_t magic;
	uint32_t nfunc;                             // functions in place (0 with an error)
	volatile uint32_t nthread;                  // threads that ran a counted function
	volatile uint32_t misses;                   // frames dropped without a return (an exception or a longjmp out of a counted function)
	volatile uint32_t paused;                   // origrun sets it ("count pause"): a function entered meanwhile is not counted (the jump and the stub still run)
	uint32_t readCost;                          // cycles one thread cycle reading costs, measured at the start
	char error[120];                            // what went wrong, empty when the list is in place
	char name[COUNT_MAXFUNC][COUNT_NAMELEN];
	uint32_t addr[COUNT_MAXFUNC];
	CountThread thread[COUNT_MAXTHREAD];
} CountShared;

void count_install(void);   // at the stand-in's load: the detours on the listed functions, if origrun asked for them
void count_detach(void);
