

#include "mtxw_comm.h"
#include "mtxw_controlblock.h"
#include <unistd.h>

int main()
{
    unsigned char data[4]={1,2,3,4};
    MTXW_LOGI("===========main: entry==============\n");
    int instId = MTXW_CB::CreateInstance(true);

    MTXW_CB::SetLocalAddress(instId,"127.0.0.1",5123);
    MTXW_CB::SetRemoteAddress(instId,"127.0.0.1",5123);

    MTXW_CB::Start(instId);

    sleep(3);
    
    for(int i=0;i<5;i++)
    {
    	MTXW_CB::SendAudioData(instId,data,sizeof(data));
    	sleep(1);
    }

    MTXW_CB::Stop(instId);

    MTXW_CB::DestroyInstance(instId);

    MTXW_LOGI("===========main: leave==============\n");

    return 0;
}

