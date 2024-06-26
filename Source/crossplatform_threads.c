#include "Headers/crossplatform_threads.h"
#include "Headers/ccolors.h"

cpthread cpThreadCreate(void* (*function)(void*), void* parameter)
{
	cpthread threadInfo = { 0 };

#ifdef _WIN32
	threadInfo.winThreadHandle = CreateThread(
		NULL, // Default security attributes
		0,    // Default stack size
		function, // Pass function to create thread for
		parameter, // Pass any function parameters
		0,    // Default creation flags
		&threadInfo.threadID // Return id of thread
	);
	
	if (threadInfo.winThreadHandle == NULL)
	{
		ErrorPrint(true, "Error Making Winthread", "An error occured while running CreateThread()");
		return threadInfo;
	}

#elif __unix__
	int threadCreationResult = pthread_create(
		&threadInfo.threadID, // Put the thread	id back
		NULL, // Default attributes
		function,
		parameter
	);
	
	if (threadCreationResult != 0) 
	{
		ErrorPrint(true, "Error Making pthread", "Error while making a posix thread");
		return threadInfo;
	}	
#else
	ErrorPrint(true, "Unsupported Platform", "This application is running on an unsupported platform");
	ExitApp();
#endif

	return threadInfo;
}

void cpThreadJoin(cpthread threadInfo)
{
#ifdef _WIN32
	if (threadInfo.winThreadHandle == NULL)
	{
		ErrorPrint(true, "Error waiting for thread to finish", "Windows thread handle is NULL");
		return;
	}

	WaitForSingleObject(threadInfo.winThreadHandle, INFINITE);
#elif __unix__
	int joinResult = pthread_join(threadInfo.threadID, NULL);
	if (joinResult != 0)
	{
		ErrorPrint(true, "Error Running pthread_join()", "Error waiting for pthread to finish");
		return;
	}
#else
	ErrorPrint(true, "Unsupported Platform", "This application is running on an unsupported platform");
	ExitApp();
#endif
}