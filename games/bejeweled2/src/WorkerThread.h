#ifndef __WORKERTHREAD_H__
#define __WORKERTHREAD_H__

// WorkerThread.h: the original's task thread (Sexy::WorkerThread, 0x5e35b8): DoTask hands a function and its argument to
// a thread that waits on an event. The port has no threads (the game never needed one to play): a task runs in line.

namespace Sexy
{

class WorkerThread
{
public:
	typedef void (*TaskFunc)(void*);

	void DoTask(TaskFunc theFunc, void* theArg) { theFunc(theArg); }     // 0x5e35b8
};

}

#endif
