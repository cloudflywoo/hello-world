#ifndef __MTXW_OSAPI_H__
#define __MTXW_OSAPI_H__

//============WIN平台==========================================================//
#if (defined WIN32 || WIN)

#include <windows.h> 
#include <process.h> 

//#include <winsock2.h>
//#include <ws2tcpip.h>

	
#define MTXW_MUTEX_HANDLE HANDLE
#define MTXW_THREAD_HANDLE HANDLE  

#define MTXW_INVALID_THREAD_HANDLE  INVALID_HANDLE_VALUE

#define mtxwCreateMutex() CreateMutex(0,false,0)
#define mtxwLockMutex(phMutex) WaitForSingleObject(*phMutex,INFINITE)
#define mtxwUnlockMutex(phMutex) ReleaseMutex(*phMutex)
#define mtxwReleaseMutex(phMutex) CloseHandle(*phMutex)

//-创建线程，pHandle:线程句柄指针，类型为MTXW_THREAD_HANDLE*， pThreadFunc: 线程函数
#define mtxwCreateThread(pHandle,pThreadFunc,pParam) *##pHandle = CreateThread(0,0,LPTHREAD_START_ROUTINE(pThreadFunc),(pParam),0,0)

//-等待某个线程的结束，hThread:要等待的线程的句柄，类型为MTXW_THREAD_HANDLE
#define mtxwJoinThread(hThread) WaitForSingleObject(hThread,INFINITE)
	
#else
//============Linux平台==========================================================//
#include <pthread.h>

#include <sys/socket.h>  
#include <sys/types.h>
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <netdb.h> 
#include <unistd.h>
#include <errno.h>


#define MTXW_MUTEX_HANDLE pthread_mutex_t
#define MTXW_THREAD_HANDLE pthread_t

#define MTXW_INVALID_THREAD_HANDLE  0

#define mtxwCreateMutex() PTHREAD_MUTEX_INITIALIZER	
#define mtxwLockMutex(phMutex) pthread_mutex_lock(phMutex)
#define mtxwUnlockMutex(phMutex) pthread_mutex_unlock(phMutex)
#define mtxwReleaseMutex(phMutex) pthread_mutex_destroy(phMutex)

//-创建线程，pHandle:线程句柄指针，类型MTXW_THREAD_HANDLE*， pThreadFunc: 线程函数
#define mtxwCreateThread(pHandle,pThreadFunc,pParam) pthread_create((pHandle), NULL, (pThreadFunc), (pParam))

//-等待某个线程的结束，hThread:要等待的线程的句柄，类型为MTXW_THREAD_HANDLE
#define mtxwJoinThread(hThread) pthread_join(hThread,0)

	
#endif  // end of #if(defined WIN32 || WIN)

#endif //end of #ifndef __MTXW_OSAPI_H__