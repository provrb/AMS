/*
	Cross platform threading library for
	both windows and linux.
*/

#ifndef __CPTHREADS_H__
#define __CPTHREADS_H__

#include <stdbool.h>
#include <errno.h>

#ifdef _WIN32
#include <processthreadsapi.h>
#elif __unix__
#include <pthread.h>
#endif

typedef unsigned long cpthreadID;

/*
	A thread with information about a
	cross platform thread that was created
*/
typedef struct cpthreadStruct
{
	/*
		unsigned long represnting the thread id.
		On linux this is whats used to wait for the thread to finish.
	*/
	cpthreadID threadID;

	/*
		If on windows, a HANDLE to a thread will be here.
		Otherwise it will be NULL.
	*/
	void* winThreadHandle;
} cpthread;

/*
	Create a thread for a function.

	Works on windows by using processthread api and
	also works on linux using pthreads.

	A struct of information about the thread is then returned
*/
cpthread cpThreadCreate(void* (*function)(void*), void* parameter);

/*
	Wait for a thread to finish before
	resuming normal activity
*/
void cpThreadJoin(cpthread threadInfo);

#endif // __CPTHREADS_H__