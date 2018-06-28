 #pragma once
#pragma comment(lib, "ws2_32.lib") 
#include <string>
#include <iostream>
#include <queue>

#include "mtxw_osapi.h"
#include "mtxw_comm.h"
#include "mtxw_audioinstance.h"
#include "mtxw_videoinstance.h"
#include "mtxw_controlblock.h"
using namespace std;

void F(void*)
{
}

int main(int argc, char* argv[])
{
	std::queue<int> que;
	HANDLE handle;
	WORD wVersionRequested;
    WSADATA wsaData;

    wVersionRequested = MAKEWORD(2, 2);

    int err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        /* Tell the user that we could not find a usable */
        /* Winsock DLL.                                  */
        printf("WSAStartup failed with error: %d\n", err);
        return 1;
    }


	//---测试创建mutex和线程的封装函数
	HANDLE mMutex = mtxwCreateMutex();
	mtxwCreateThread(&handle,LPTHREAD_START_ROUTINE(F),0);


    system("pause");
     _CrtDumpMemoryLeaks(); 
    return 0;
}