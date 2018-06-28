#include "mtxw_instance.h"
#include "mtxw_controlblock.h"
#include <string.h>
#include <time.h>
#include <net/if.h>

//buffer��Ϣ����󳤶�
static UINT32  MTXW_AUDIO_BUFFER_MSG_SIZE_MAX = 1*1024;
static UINT32  MTXW_VIDEO_BUFFER_MSG_SIZE_MAX = 2*1024*1024;
#define  MTXW_BUFFER_MSG_SIZE_MAX ((this->bAudio)? (MTXW_AUDIO_BUFFER_MSG_SIZE_MAX):(MTXW_VIDEO_BUFFER_MSG_SIZE_MAX))


MTXW_Instance::MTXW_Instance(bool isAudio,INT32 uInstanceId,INT32 callId)
{
	MTXW_FUNC_TRACE();
	this->bAudio = isAudio;//true = audio, false = video
	this->uInstanceId = uInstanceId;
	this->callId = callId;//2018.4.2
	this->bRuning = false;

	//--�������	
	this->enDirection = MTXW_DIRECTION_SEND_RECEIVE; //���䷽��
	this->strLocalAddr="";
	this->uLocalPort=0xFFFF;
	this->strRemoteAddr="";
	this->uRemotePort=0xFFFF;

	this->strInterface = "";

	this->mSock = -1;
	this->mhSocketMutex = mtxwCreateMutex();

	this->ulRcvBytes = 0;
	this->ulSndBytes = 0;
	this->ulRcvPacketNum = 0;    
	this->ulDropPacketNum= 0;

	this->ulSndFrameNum = 0;
	this->ulRcvFrameNum = 0;
	this->ulSndFrameRate = 0;
	this->ulRcvFrameRate = 0;
	this->ulPlayFrameNum = 0;

	this->usEncodeRank = 3;
       this->usEncodeGDLI = 1;

       this->bFecSwitch = true;
       this->bPeerSupportFec = true;
       this->pRtpFecEncoder = 0;
       this->pRtpFecDecoder = 0;
       this->usSimulationDropRate = 0;
       srand((unsigned)time(NULL));
       this->bAdaptive = true;

       mPlayBuffer.pBuffer = 0;
       mPlayBuffer.ulSize = 0;
  
	//--�߳�
	this->sendThread = MTXW_INVALID_THREAD_HANDLE;
	this->receiveThread = MTXW_INVALID_THREAD_HANDLE;
	this->playThread = MTXW_INVALID_THREAD_HANDLE;

	//--������󳤶�����
	this->ulSendQueueLengthMax = 100; //100 frame

	if(this->bAudio)
	{
	   this->ulPlayQueueLengthMax = 50; //10 frame
	}
	else
	{
	    this->ulPlayQueueLengthMax = 20; //20 frame
	 }
	mtxwMemSet(&this->VedioCount, 0, sizeof(MTXW_VEDIO_COUNT_STRU));


	//--socket����buffer
	if((this->m_pSockRecvBuffer = (MTXW_DATA_BUFFER_STRU* ) mtxwMemAlloc(MTXW_BUFFER_MSG_SIZE_MAX)))//����64k�ڴ�
	{
		this->m_pSockRecvBuffer->uSize = MTXW_BUFFER_MSG_SIZE_MAX - sizeof(MTXW_DATA_BUFFER_STRU);
	}
	else
	{
		//�����ڴ�ʧ��
		MTXW_LOGE("\n Start(): allocate mem for m_pSockRecvBuffer fail!");

	}

	//--����buffer
	if((this->m_pEncodeBuffer = (MTXW_DATA_BUFFER_STRU* ) mtxwMemAlloc(MTXW_BUFFER_MSG_SIZE_MAX)))//����64k�ڴ�
	{
		this->m_pEncodeBuffer->uSize = MTXW_BUFFER_MSG_SIZE_MAX - sizeof(MTXW_DATA_BUFFER_STRU);
	}
	else
	{
		//�����ڴ�ʧ��
		MTXW_LOGE("\n Start(): allocate mem for m_pEncodeBuffer fail!");

	}
	//--����buffer
	if((this->m_pPackBuffer = (MTXW_DATA_BUFFER_STRU* ) mtxwMemAlloc(MTXW_BUFFER_MSG_SIZE_MAX)))//����64k�ڴ�
	{
		this->m_pPackBuffer->uSize = MTXW_BUFFER_MSG_SIZE_MAX - sizeof(MTXW_DATA_BUFFER_STRU);
	}
	else
	{
		//�����ڴ�ʧ��
		MTXW_LOGE("\n Start(): allocate mem for m_pPackBuffer fail!");

	}

	this->m_pDecodeBuffer = 0;

	if((this->m_pFecDecodeBuffer = (MTXW_DATA_BUFFER_STRU* ) mtxwMemAlloc(sizeof(MTXW_DATA_BUFFER_STRU)+1500)))//����64k�ڴ�
	{
		this->m_pFecDecodeBuffer->uSize = 1500;
	}
	else
	{
		//�����ڴ�ʧ��
		MTXW_LOGE("\n Start(): allocate mem for m_pPackBuffer fail!");

	}
	
	
}
MTXW_Instance::~MTXW_Instance()
{     
    
        MTXW_FUNC_TRACE();
	if(this->IsRunning())
	{
	    this->Stop();//ֹͣ�߳�
	}
	
	//--�ͷŶ�������ڴ�---------//

	while(!this->mSendQueue.empty())
	{
		mtxwMemFree(this->mSendQueue.front());
		this->mSendQueue.pop();
	}

	while(!this->mPlayQueue.empty())
	{
		mtxwMemFree(this->mPlayQueue.front());
		this->mPlayQueue.pop();
	}
	
	if(this->m_pSockRecvBuffer){mtxwMemFree(this->m_pSockRecvBuffer);}
	if(this->m_pEncodeBuffer){mtxwMemFree(this->m_pEncodeBuffer);}
	if(this->m_pPackBuffer){mtxwMemFree(this->m_pPackBuffer);}
	if(this->m_pDecodeBuffer){mtxwMemFree(this->m_pDecodeBuffer);}
	if(this->m_pFecDecodeBuffer){mtxwMemFree(this->m_pFecDecodeBuffer);}

	mtxwReleaseMutex(&mhSocketMutex);
	
}


INT32 MTXW_Instance::Start()
{
        MTXW_FUNC_TRACE();
#if (defined WIN32 || WIN)

    HANDLE ret;
#else   
    INT32 ret = 0;
#endif  

        INT32 rslt = 0;
        if(IsRunning())
        {
            MTXW_LOGE("\n MTXW_Instance is already started!");
            return -1;
        }

        if(0==this->m_pSockRecvBuffer ||
        0==this->m_pEncodeBuffer ||
        0==this->m_pPackBuffer)
        {
            return -1;
        }
        
      	//--1--����socket---------------------
        rslt = CreateSocket();
        if(0 != rslt)
        {
            MTXW_LOGE("\n CreateSocket fail!");
            return rslt;
        }


        this->bRuning = true;

       this->ulSndFrameNum = 0;
	this->ulRcvFrameNum = 0;
	this->ulSndFrameRate = 0;
	this->ulRcvFrameRate = 0;
	this->ulPlayFrameNum = 0;

        if(bFecSwitch){
            //--����FEC������
            pRtpFecEncoder = new RtpFecEncoder(this->rtpEncodeOutput,this,this->usEncodeRank,this->usEncodeGDLI);

            //--����FEC������
            pRtpFecDecoder = new RtpFecDecoder(this->rtpDecodeOutput,RcvCQI,UpdCQI_GDLI_RI,this);
        }
        this->bPeerSupportFec = true;
        
      	//--2--�����߳�--------------------

	if(MTXW_DIRECTION_SEND_RECEIVE == this->enDirection 
	|| MTXW_DIRECTION_SEND_ONLY == this->enDirection)
	{
		ret = mtxwCreateThread(&this->sendThread,MTXW_Instance::gSendThread,this);

#if (defined WIN32 || WIN)
#else
              if(0 != ret)
		{
		    MTXW_LOGE("\n Create SendThread  fail!Cause:%s",strerror(errno));
		}
#endif 

	}
	if(MTXW_DIRECTION_SEND_RECEIVE == this->enDirection 
	|| MTXW_DIRECTION_RECEIVE_ONLY == this->enDirection)
	{


		ret = mtxwCreateThread(&this->playThread,MTXW_Instance::gPlayThread,this);
		
#if (defined WIN32 || WIN)
#else
              if(0 != ret)
		{
		    MTXW_LOGE("\n Create PlayThread  fail!Cause:%s",strerror(errno));
		}
#endif 

	
		ret = mtxwCreateThread(&this->receiveThread,MTXW_Instance::gReceiveThread,this);
		
#if (defined WIN32 || WIN)
#else
              if(0 != ret)
		{
		    MTXW_LOGE("\n Create ReceiveThread  fail!Cause:%s",strerror(errno));
		}
#endif 




	}	

	ret = mtxwCreateThread(&this->statisticThread, MTXW_Instance::gStatisticThread,this);
#if (defined WIN32 || WIN)
#else     
       if(0 != ret)
       {
           MTXW_LOGE("\n Create statisticThread  fail!Cause:%s",strerror(errno));
       }
#endif 
	mtxwMemSet(&this->VedioCount, 0, sizeof(MTXW_VEDIO_COUNT_STRU));
	
	return 0;
}
INT32 MTXW_Instance::Stop()
{
        MTXW_FUNC_TRACE();
        //---ֹͣ�߳�---------------------
       if(false == this->bRuning){return 0;}
	this->bRuning = false;
	
       {
            MTXW_lock   lock(this->mSendQueue.getMutex(),"MTXW_Instance::Stop   mSendQueue");
            this->mSendQueue.signal();
       }
       
       {
            MTXW_lock   lock(this->mPlayQueue.getMutex(),"MTXW_Instance::Stop   mPlayQueue");
            this->mPlayQueue.signal();
       }

       usleep(10000);
        //--�ر�socket-----------------------
       {
       MTXW_lock   lock(&mhSocketMutex,"Stop mhSocketMutex");
       CloseSocket();
       }

        
        MTXW_LOGI("\n stop mtxwJoinThread!this->sendThread");
        mtxwJoinThread(this->sendThread);

        MTXW_LOGI("\n stop mtxwJoinThread!this->receiveThread");
        mtxwJoinThread(this->receiveThread);

        MTXW_LOGI("\n stop mtxwJoinThread!this->playThread");	
        mtxwJoinThread(this->playThread);
        
        MTXW_LOGI("\n stop mtxwJoinThread!this->statisticThread");	
        mtxwJoinThread(this->statisticThread);
	
	this->sendThread = MTXW_INVALID_THREAD_HANDLE;
	this->receiveThread = MTXW_INVALID_THREAD_HANDLE;
	this->playThread = MTXW_INVALID_THREAD_HANDLE;
	this->statisticThread = MTXW_INVALID_THREAD_HANDLE;



	//--��ո�����Ϣ����----------
	while(!this->mSendQueue.empty())
	{
		mtxwMemFree(this->mSendQueue.front());
		this->mSendQueue.pop();
	}

	while(!this->mPlayQueue.empty())
	{
		mtxwMemFree(this->mPlayQueue.front());
		this->mPlayQueue.pop();
	}

	delete pRtpFecEncoder;
	pRtpFecEncoder = 0;

	delete pRtpFecDecoder;
	pRtpFecDecoder = 0;
	
        //��ӡͳ����
        MTXW_LOGI("Video.rtp.rcv_cnt: %d", this->VedioCount.rtp_rcv_cnt);
        MTXW_LOGI("Video.rtp.lost_cnt: %d", this->VedioCount.rtp_lost_cnt);
        MTXW_LOGI("Video.rtp.drop_cnt: %d", this->VedioCount.rtp_drop_cnt);
        MTXW_LOGI("Video.Frame.rcv_cnt: %d", this->VedioCount.frame_rcv_cnt);
        MTXW_LOGI("Video.frame_drop_for_damage_cnt: %d", this->VedioCount.frame_drop_for_damage_cnt);
        MTXW_LOGI("Video.frame_drop_for_full_cnt: %d", this->VedioCount.frame_drop_for_full_cnt);

        MTXW_LOGA("MTXW_Instance::Stop finished");
        
	return 0;
}
INT32 MTXW_Instance::SetLocalAddress(std::string ip,UINT16 port)
{
        MTXW_FUNC_TRACE();
        if(IsRunning())
        {
            MTXW_LOGE("SetLocalAddress() failed ,because instance is running now!");
            return -1;
        }
        this->strLocalAddr = ip;
        this->uLocalPort = port;
	return 0;
}
INT32 MTXW_Instance::SetRemoteAddress(std::string ip,UINT16 port)
{
        MTXW_FUNC_TRACE();

        if(IsRunning())
        {
            MTXW_LOGE("SetRemoteAddress() failed ,because instance is running now!");
            return -1;
        }
        this->strRemoteAddr = ip;
        this->uRemotePort = port;
	return 0;
}
INT32 MTXW_Instance::SetDirection(MTXW_ENUM_DIRECTION direction)
{
        MTXW_FUNC_TRACE();

        if(IsRunning())
        {
            MTXW_LOGE("SetDirection() failed ,because instance is running now!");
            return -1;
        }
        this->enDirection = direction;
	return 0;
}

INT32 MTXW_Instance::SetInterface(std::string strInterface)
{
    this->strInterface = strInterface;
    return 0;
}

INT32 MTXW_Instance::SetFecParam(UINT16 FecSwitch,UINT16 InputGDLI,UINT16 InputRank,UINT16 SimulationDropRate,UINT16 adaptiveSwitch)
{
        MTXW_FUNC_TRACE();
        if(IsRunning())
        {
            MTXW_LOGE("SetFecParam() failed ,because instance is running now!");
            return -1;
        }
        
        if(FecSwitch==1){
            this->bFecSwitch = true;
        }else{
            this->bFecSwitch = false; 
        }
        this->usEncodeRank = InputRank;
        this->usEncodeGDLI = InputGDLI;
        this->usSimulationDropRate = SimulationDropRate;
        this->bAdaptive = (adaptiveSwitch>0);
        MTXW_LOGA("MTXW_Instance::SetFecParam is bFecSwitch %d usEncodeRank %d usEncodeGDLI %d usSimulationDropRate %d adaptiveSwitch%d",bFecSwitch,usEncodeRank,usEncodeGDLI,usSimulationDropRate,adaptiveSwitch);
	return 0;
}


bool  MTXW_Instance::IsMultiCastIp(std::string strAddr)
{
       MTXW_FUNC_TRACE();
	const char *p=strAddr.c_str();
	char tmp[4]={0};
	for(int i=0;i<4;i++)
	{
		if(p[i]=='.'){break;}
		
		tmp[i]=p[i];
	}
	
	if((atoi(tmp)&0xF0) == 0xE0)
	{
		return true;
	}
	else
	{
		return false;
	}
}
INT32 MTXW_Instance::CreateSocket()
{
       MTXW_FUNC_TRACE();
       
	if(IsMultiCastIp(this->strLocalAddr))
	{      
		return CreateMultiCastSocket();
	}
	
	
	
#if (defined WIN32 || WIN)
    mSock = socket(AF_INET,SOCK_DGRAM, 0);
	int errCode = 0;
	if(INVALID_SOCKET==mSock)
	{
		return -1;
	}
	//�󶨱��˶˿�
    SOCKADDR_IN  localAddr;
    localAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(this->uLocalPort);

    int opt = 1;
    if(0!=setsockopt(mSock,SOL_SOCKET,SO_REUSEADDR,(const char*)&opt,sizeof(opt)))
    {
        errCode = WSAGetLastError();
        //print error
    }

	
    if(0!=bind(mSock,(SOCKADDR*)&localAddr,sizeof(SOCKADDR)))
    {
        errCode = WSAGetLastError();
        closesocket(mSock);
        mSock = -1;
        return -1;
    }
	
#else
        mSock = socket(AF_INET,SOCK_DGRAM,0);
        if(mSock<0)
        {   
            MTXW_LOGE("\n mSock:%d!Cause:%s ",mSock,  strerror(errno));
            return -1;
        }
	
        //�󶨱��˶˿�
        struct sockaddr_in localAddr;
        bzero(&localAddr,sizeof(localAddr));  
        localAddr.sin_family=AF_INET;  
        localAddr.sin_addr.s_addr=htonl(INADDR_ANY);  
        localAddr.sin_port=htons(this->uLocalPort);  

        int opt = 1;
        if(0!=setsockopt(mSock,SOL_SOCKET,SO_REUSEADDR,(const char*)&opt,sizeof(opt)))
        {
           MTXW_LOGE("\n setsockopt fail!Cause:%s",strerror(errno));
        }

    
        if(0!=bind(mSock,(struct sockaddr*)&localAddr,sizeof(localAddr)))
        {
            close(mSock);
            mSock = -1;
            MTXW_LOGE("\n bind mSock fail!Cause:%s port = %d addr = %d",
                            strerror(errno),localAddr.sin_port,localAddr.sin_addr.s_addr);
            return -1;
        }

        //--����socket�Ľ��շ��ͻ����С
       
        {
            UINT32 ulRcvBufSize = (this->bAudio) ?  2*1024*1024 : 10*1024*1024;
            UINT32 ulSndBufSize = 2*1024*1024;
            UINT32 ullen = sizeof(ulRcvBufSize);
            INT32 rslt;

            rslt = setsockopt(mSock,SOL_SOCKET,SO_RCVBUF,&ulRcvBufSize,sizeof(ulRcvBufSize));
            MTXW_LOGA("MTXW_Instance::CreateSocket setsockopt rslt %d",rslt);
            rslt = getsockopt(mSock, SOL_SOCKET, SO_RCVBUF, &ulRcvBufSize, (socklen_t*)&ullen);
            MTXW_LOGA("MTXW_Instance::CreateSocket socket receive buffer size is %d",ulRcvBufSize);

            
            setsockopt(mSock,SOL_SOCKET,SO_SNDBUF,&ulSndBufSize,sizeof(ulSndBufSize));
            getsockopt(mSock, SOL_SOCKET, SO_SNDBUF, &ulSndBufSize, (socklen_t*)&ullen);
            MTXW_LOGA("MTXW_Instance::CreateSocket socket send buffer size is %d",ulSndBufSize);
       }

	
#endif

    return 0;
}

INT32 MTXW_Instance::CreateMultiCastSocket()
{
    MTXW_FUNC_TRACE();
#if (defined WIN32 || WIN)
    mSock = socket(AF_INET,SOCK_DGRAM, 0);
	if(INVALID_SOCKET==mSock)
	{
		return -1;
	}
	//�󶨱��˶˿�
    SOCKADDR_IN  localAddr;
    localAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(this->uLocalPort);

    int opt = 1;
	int errCode = 0;
    if(0!=setsockopt(mSock,SOL_SOCKET,SO_REUSEADDR,(const char*)&opt,sizeof(opt)))
    {
            errCode = WSAGetLastError();
            //print error
    }

    if(0!=bind(mSock,(SOCKADDR*)&localAddr,sizeof(SOCKADDR)))
	{
		closesocket(mSock);
		mSock = -1;
		return -1;
	}
	
    //�رձ��ػػ�
    bool loop = 1;  
    
#if 0
    if(setsockopt(mSock,IPPROTO_IP, IP_MULTICAST_LOOP, reinterpret_cast<char FAR *>(&loop), sizeof(loop))<0)
	{
		closesocket(mSock);
		mSock = -1;
		return -1;
	}
#endif

    struct ip_mreq mreq;  
    mreq.imr_multiaddr.S_un.S_addr = inet_addr(this->strLocalAddr.c_str());  //�㲥ip��ַ
    mreq.imr_interface.s_addr = inet_addr(INADDR_ANY);  //Ҫ����������
    if(setsockopt(mSock, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<char FAR *>(&mreq), sizeof(mreq))<0)
	{
		closesocket(mSock);
		mSock = -1;
		return -1;
	}		

#else
	
    mSock = socket(AF_INET,SOCK_DGRAM,0);
    if(mSock<0)
    {     
        MTXW_LOGE("\n mSock:%d!Cause:%s ",mSock,  strerror(errno));
        return -1;
    }
	
    /* allow multiple sockets to use the same PORT number */  
    u_int yes=1;
    if (setsockopt(mSock,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) < 0)     
    {  
        close(mSock);
        mSock = -1;
        MTXW_LOGE("\n setsockopt SO_REUSEADDR fail!Cause:%s ",strerror(errno));
        return -1;    
    }
	
    //�󶨱��˶˿�
    struct sockaddr_in localAddr;
    bzero(&localAddr,sizeof(localAddr));  
    localAddr.sin_family=AF_INET;  
    localAddr.sin_addr.s_addr=htonl(INADDR_ANY);  
    localAddr.sin_port=htons(this->uLocalPort);  
    int optVal = 1;
    int optLen = sizeof(optVal);
    if(0!=setsockopt(mSock,SOL_SOCKET,SO_REUSEADDR,(const char*)&optVal,optLen))
    {
        MTXW_LOGA("\n CreateMultiCastSocket setsockopt (SO_REUSEADDR)fail!Cause:%s",strerror(errno));
    }

    optVal = (this->bAudio) ?  2*1024*1024 : 10*1024*1024;

    if(0!=setsockopt(mSock,SOL_SOCKET,SO_RCVBUF,(const char*)&optVal,optLen))
    {
        MTXW_LOGA("\n CreateMultiCastSocket setsockopt (SO_RCVBUF) fail!Cause:%s",strerror(errno));
    }

    if(0!=getsockopt(mSock, SOL_SOCKET, SO_RCVBUF, &optVal, (socklen_t*)&optLen))
    {
        MTXW_LOGA("\n CreateMultiCastSocket getsockopt (SO_RCVBUF) fail!Cause:%s",strerror(errno));
    }
    else
    {
        MTXW_LOGA("\n CreateMultiCastSocket getsockopt (SO_RCVBUF = %d) ",optVal);
    }

    if(0!=bind(mSock,(struct sockaddr *)&localAddr,sizeof(localAddr)))
    {
        close(mSock);
        mSock = -1;
        MTXW_LOGE("\n bind mSock fail!Cause:%s port = %d addr = %d",
                        strerror(errno),localAddr.sin_port,localAddr.sin_addr.s_addr);
        return -1;
    }	

    const char *p= this->strInterface.c_str();
    struct ip_mreq mreq;

    mreq.imr_multiaddr.s_addr=inet_addr(this->strLocalAddr.c_str());  //�㲥ip��ַ 
    if(p && strlen(p)>1)
    {
        struct ifreq ifr;
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name,p,IFNAMSIZ-1);
        ioctl(mSock,SIOCGIFADDR,&ifr);
        const char* pIpAddr =inet_ntoa( ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr);
        mreq.imr_interface.s_addr = inet_addr(pIpAddr);
        MTXW_LOGA("\n CreateMultiCastSocket iface = %s ip = %s ",p,pIpAddr);
    }
    else
    {
        mreq.imr_interface.s_addr=htonl(INADDR_ANY);   //Ҫ����������
        MTXW_LOGA("CreateMultiCastSocket INADDR_ANY");
    }
    if (setsockopt(mSock,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0)     
    {    
        close(mSock);
        mSock = -1;
        MTXW_LOGE("\n setsockopt IP_ADD_MEMBERSHIP fail!Cause:%s ",strerror(errno));
        return -1;    
    } 
	
	
#endif

    return 0;
	
}

INT32 MTXW_Instance::CloseSocket()
{
    MTXW_FUNC_TRACE();
#if (defined WIN32 || WIN)
	if(IsMultiCastIp(this->strLocalAddr))
	{    //�˳��鲥ip����
		 struct ip_mreq mreq;  
         mreq.imr_multiaddr.S_un.S_addr = inet_addr(this->strLocalAddr.c_str());  //�㲥ip��ַ
         mreq.imr_interface.s_addr = inet_addr(INADDR_ANY);  //Ҫ����������
		 setsockopt(mSock, IPPROTO_IP, IP_DROP_MEMBERSHIP, reinterpret_cast<char FAR *>(&mreq), sizeof(mreq));
	}
	closesocket(mSock);
    mSock = INVALID_SOCKET;
#else
    close(mSock);
    mSock = -1;
#endif
    
	return 0;
}
	
    //codec���룬pInData-��Σ�����Ϊԭʼ��������(��PCM)  pOutData-���Σ�����Ϊ���������(��AMR)
INT32 MTXW_Instance::Encode(const MTXW_DATA_BUFFER* pInData,MTXW_DATA_BUFFER* pOutData)
{
        MTXW_FUNC_TRACE();
	return 0;
}
	
	//decode���룬pInData-��Σ�����Ϊ����������(��AMR)  pOutData-���Σ�����Ϊ���������(��PCM)
INT32 MTXW_Instance::Decode(const MTXW_DATA_BUFFER* pInData,MTXW_DATA_BUFFER* pOutData)
{
        MTXW_FUNC_TRACE();
	return 0;
}
	
	//���������������δ��������ݣ�����Ǵ���������
	//pPackedData: �����߱����ṩ�㹻��Ļ�����������������
INT32 MTXW_Instance::Pack(MTXW_DATA_BUFFER_STRU* pInputData, MTXW_DATA_BUFFER_STRU*pPackedData)
{
    MTXW_FUNC_TRACE();
    UINT16 *pLength = (UINT16*)(&pPackedData->pData[0]);

    *pLength = pInputData->uSize;

    mtxwMemCopy(pPackedData->pData+2, pInputData->pData, pInputData->uSize);
    
    return 0;
}
	

INT32 MTXW_Instance::ReceiveRtpPacket(MTXW_DATA_BUFFER_STRU* pInputData)
{
    MTXW_FUNC_TRACE();
    MTXW_DATA_BUFFER_STRU *pBuffer = 0;

    if((pBuffer = (MTXW_DATA_BUFFER_STRU* ) mtxwMemAlloc(sizeof(MTXW_DATA_BUFFER_STRU)+pInputData->uSize)))
    {
    	pBuffer->uSize = pInputData->uSize;
    }
    else
    {
        //�����ڴ�ʧ��
        MTXW_LOGE("mtxwMemAlloc fail!");
        return -1;
    }

    mtxwMemCopy(pBuffer->pData, pInputData->pData, pBuffer->uSize);


    
    {
         MTXW_lock   lock(this->mPlayQueue.getMutex(),"MTXW_Instance::ReceiveRtpPacket mPlayQueue");//���ݶ�������

         this->mPlayQueue.push(pBuffer); //���ݷ��벥�Ŷ���
    }



    
    return 0;
}

INT32 MTXW_Instance::SendTo(UINT8* pData,UINT16 usOnePackLen,UINT32 time, MTXW_SOCKADDR* pDestAddr,UINT16 usDstAddrLen)
{
     INT32 rslt = usOnePackLen;
     if(this->bFecSwitch && (this->pRtpFecEncoder!=0)&&this->bPeerSupportFec){
         pRtpFecEncoder->input(pData, usOnePackLen);
     }else{
         _SendTo(pData,usOnePackLen,time,pDestAddr,usDstAddrLen);
     }

     return rslt;
}

/*********************************************************************************************
*����:������socket�Ϸ�������
*
*
**********************************************************************************************/
INT32 MTXW_Instance::_SendTo(UINT8* pData,UINT16 usOnePackLen,UINT32 time, MTXW_SOCKADDR* pDestAddr,UINT16 usDstAddrLen)
{
    int rslt = 0;
    UINT16 count = 0;
    UINT16 count1 = 0;
    char pStrBuff[128];
    UINT32 index = 0;

    UINT16 usCurrentAudioSeq = 0;
    UINT16 usCurrentVedioSeq = 0;


    fd_set  wset; 
    struct timeval tv;
    tv.tv_sec   = 0;  //��
    tv.tv_usec = 10000; //��λ΢��
    UINT32 ulResendCount = 0;




    for(int i = 0;i<usOnePackLen && i < 32;i++)
    {
      index += sprintf(pStrBuff+index, "%02x ",pData[i]);
    }
    MTXW_LOGD("MTXW_Instance::SendThread thread_id:%d one Packet Context:%s",(int)pthread_self(),pStrBuff);

    
    RESEND:
#if 1

    //---1---�ȴ�socket״̬��Ϊ��д״̬----------------------
    while(this->IsRunning())
    {
        FD_ZERO(&wset);
        FD_SET(this->mSock, &wset);
        tv.tv_sec   = 0;  //��
        tv.tv_usec = 10000; //��λ΢��

        rslt = select(this->mSock+1, NULL, &wset, NULL, &tv);
        if(0 < rslt)
        {
            if(!FD_ISSET(this->mSock,&wset))
            {
                MTXW_LOGE("MTXW_Instance::SendThread FD_ISSET!");
                continue;
            }
            break;
        }
        else if(0 == rslt)
        {
            if(count %100 == 0 )MTXW_LOGE("MTXW_Instance::SendThread SELECT TIMEOUT! count:%d",count);
            count++;
            continue;
        }
        else if(0 > rslt)
        {
            MTXW_LOGE("MTXW_Instance::SendThread SELECT ERROR!rslt=%d CAUSE:%s",rslt,strerror(errno));
            continue;
        }
    }
    if(!this->IsRunning()){return 0;}
#endif				

    if(this->bAudio)
    {
        usCurrentAudioSeq = (pData[2]<<8)| pData[3];
        
        MTXW_LOGD("MTXW_Instance::SendThread Audio seq = %d",usCurrentAudioSeq);                              
    }
    else
    {
        usCurrentVedioSeq = (pData[2]<<8)|pData[3];
        MTXW_LOGD("MTXW_Instance::SendThread Vedio seq = %d",usCurrentVedioSeq);            
    }
        

    rslt = sendto(mSock, 
	                 (char*) pData,
	                 usOnePackLen,
	                 time,//MSG_DONTWAIT,
	                 pDestAddr, 
	                 usDstAddrLen); 

    //--2--����ʧ�ܺ�����ط�------
    if(rslt != usOnePackLen)
    {
        ulResendCount++;
        if(ulResendCount>=10)
        {
            //����10�η���ʧ�ܣ�������socket
            {
                MTXW_lock   lock(&mhSocketMutex, "SendThreadSocketMutex");
                CloseSocket();
                CreateSocket();
            }
           MTXW_LOGE("Resect Socket");
        }
        if(count1 %100 == 0 ){MTXW_LOGE("\n sendto rslt = %d usOnePackLen=%d",rslt,usOnePackLen);}
        count1++;

        if(this->IsRunning()){
            goto RESEND;
        }
    }else{
        this->ulSndBytes += usOnePackLen;		      
        MTXW_LOGD("\n sendto rslt = %d usOnePackLen=%d",rslt,usOnePackLen);
    
    }

    return rslt;


}
void MTXW_Instance::SendThread()
{
	MTXW_FUNC_TRACE();
	MTXW_DATA_BUFFER_STRU* pSendData=0;
	int rslt = 0;

	UINT32 ulSendPacketCnt = 0;


#if (defined WIN32 || WIN)

#define MTXW_SOCKADDR SOCKADDR
	SOCKADDR_IN  DestAddr;
	DestAddr.sin_addr.S_un.S_addr = inet_addr(this->strRemoteAddr.c_str());
	DestAddr.sin_family = AF_INET;
	DestAddr.sin_port = htons(this->uRemotePort);
#else
#define MTXW_SOCKADDR sockaddr
	sockaddr_in DestAddr;
	bzero(&DestAddr,sizeof(DestAddr));  
	DestAddr.sin_family=AF_INET;  
	DestAddr.sin_addr.s_addr=inet_addr(this->strRemoteAddr.c_str());
	DestAddr.sin_port=htons(this->uRemotePort);  
#endif

	
	while(this->IsRunning())
	{
		pSendData = 0;
		
		//--1--�ӷ��Ͷ���ȡ����-------------------
		{
		       MTXW_lock   lock(this->mSendQueue.getMutex(),"MTXW_Instance::SendThread  mSendQueue");//���ݶ�������
                    if(!this->IsRunning()){break;} //��ֹStopʱ�Ĺ㲥���źŶ�ʧ
			if(this->mSendQueue.empty())
			{
			       MTXW_LOGV("\n Start mSendQueue wait!");
				this->mSendQueue.wait();
				
			}
			MTXW_LOGV("\n SendThread wake up");

			if(!this->IsRunning())
			{
			    break;
			}
			
			if(this->mSendQueue.empty())
			{
				continue;
			}
			else
			{
				pSendData = this->mSendQueue.front();
				this->mSendQueue.pop();
				this->ulSndFrameNum ++;
			}
			
		}
		
		//--2--����-------------------------------
		
		this->Encode(pSendData,pSendData);
                
		//--3--RTP���----------------------------
		ClearPackBuffer();//���buffer��ʼ��
		this->Pack(pSendData, this->m_pPackBuffer);           

		//--4--socket����-------------------------

		//����������,����Ϊ0���Ǿ�����,2��һ��������
		if((*(UINT16*)(this->m_pPackBuffer->pData)) == 0 && this->bAudio)
		{
		     m_pPackBuffer->uSize = 3;
		     UINT16 *pOnePackLen = (UINT16*) (&m_pPackBuffer->pData[0]);
		     *pOnePackLen = 1;
		     m_pPackBuffer->pData[2]=0;
		   /*  
                   rslt = sendto(mSock, 
                         (char*) &(this->m_pPackBuffer->pData[0]),
                         1,
                         0,//MSG_DONTWAIT,
                         (MTXW_SOCKADDR*)&DestAddr, 
                         sizeof(DestAddr)); 
                         
                   continue;  
                   */
		}
		UINT16 usOnePackLen =0;
		UINT32 ulTotalLen = 0;

		MTXW_LOGV("this->m_pPackBuffer->uSize :%d ", this->m_pPackBuffer->uSize);
		while((usOnePackLen = *((UINT16*)(this->m_pPackBuffer->pData+ulTotalLen)))>0)
		{
                    
		      MTXW_LOGV("usOnePackLen :%d ",usOnePackLen);
		      ulTotalLen += 2;
		      if(ulTotalLen+usOnePackLen > this->m_pPackBuffer->uSize)
		      {
		          //������ 
		          MTXW_LOGE("MTXW_Instance::SendThread. The rest of the buff %d > %d. is not enough to send!",(ulTotalLen+usOnePackLen),(this->m_pPackBuffer->uSize));
		          break;
		      }

		      rslt = SendTo(&this->m_pPackBuffer->pData[ulTotalLen],
		                 usOnePackLen,
		                 0,//MSG_DONTWAIT,
		                 (MTXW_SOCKADDR*)&DestAddr, 
		                 sizeof(DestAddr)); 

		      ulTotalLen += usOnePackLen;

		      ulSendPacketCnt++;//���Ͱ���ͳ��
		      //---2018.4.2--�����͵�FEC��Ϣͨ������������ʵ����ȡ

        		if(false==this->bAudio && this->enDirection == MTXW_DIRECTION_SEND_ONLY){
        		    if(ulSendPacketCnt%50==49){//���if������Ϊ��:1)����һ������ٲ�ѯ��2)��ֹƵ������
                                   //�������Ƶ���ͣ�ͨ������ͨ�����ж��Ƿ�Է��Ƿ�֧��FEC
                                   this->bPeerSupportFec = MTXW_CB::isAssoInstSupportFec(this->callId,this->uInstanceId);
        		    }
                       }
		}
		

		//--5--�ͷ��ڴ�---------------------------
		mtxwMemFree(pSendData);
		
	}
	MTXW_LOGA("SendThread Exit!");
}
void MTXW_Instance::PlayThread()
{
       MTXW_FUNC_TRACE();
	MTXW_DATA_BUFFER_STRU* pPlayData=0;

#ifndef WIN32
	struct timespec currtime = {0, 0};
	struct timespec pretime={0,0};//ǰһ�β��ŵ�ʱ��
#endif


	unsigned long uPlayFrameInterval;
	unsigned long uDelta;

	bool  initFlag = true;
	unsigned long uPlayQueueSize=0;
	
	while(this->IsRunning())
	{
		pPlayData = 0;
		//--1--�Ӳ��Ŷ����ó�����------------
		{
			MTXW_lock   lock(this->mPlayQueue.getMutex(),"MTXW_Instance::PlayThread mPlayQueue");//���ݶ�������
			if(!this->IsRunning()){break;} //��ֹStopʱ�Ĺ㲥���źŶ�ʧ

			uPlayQueueSize = this->mPlayQueue.size();
			
			if(this->mPlayQueue.empty() || 
			   ((true==initFlag)&&GetInitPlayQueueSize()>uPlayQueueSize))   //��ʼ��������
			{
			       MTXW_LOGV("\n Start mPlayQueue wait!");
				this->mPlayQueue.wait();
			}
			MTXW_LOGV("\n mPlayQueue wake up (%s) mPlayQueue size = %ld",(this->bAudio) ? "Audio":"Video",uPlayQueueSize);
			
			if(this->mPlayQueue.empty() ||(!this->IsRunning()) || ((true==initFlag)&&GetInitPlayQueueSize()>uPlayQueueSize))  
			{
				continue;
			}
			else
			{
				pPlayData = this->mPlayQueue.front();
				this->mPlayQueue.pop();
				initFlag = false;
				if(!bAudio)
				{
				    this->VedioCount.frame_rcv_cnt++ ;
				    this->VedioCount.frame_buffer_size = uPlayQueueSize;
				    if(this->VedioCount.frame_buffer_size>this->GetPlayQueueLengthMax()/2)
				    {
				        MTXW_LOGE("Video.Frame.buffer_size: %d", this->VedioCount.frame_buffer_size);
				    }
				    else
				    {
				        MTXW_LOGI("Video.Frame.buffer_size: %d", this->VedioCount.frame_buffer_size);
				    }
				}
			}
			
		}
		
		//--2--���ýӿڲ���----------------
		uPlayFrameInterval = this->GetPlayFrameInterval();//ms
#ifndef WIN32
		clock_gettime(CLOCK_MONOTONIC, &currtime); //��ȡ��ǰʱ��
		
		if(currtime.tv_nsec>pretime.tv_nsec)
		{
		    uDelta = (currtime.tv_sec - pretime.tv_sec)*1000000000 + (currtime.tv_nsec - pretime.tv_nsec);//nano second
		}
		else
		{
		    uDelta = (currtime.tv_sec - pretime.tv_sec - 1)*1000000000 + (currtime.tv_nsec + 1000000000 - pretime.tv_nsec);//nano second
		}

		MTXW_LOGV("MTXW_Instance::PlayThread() %s  uDelta = %ld!", (this->bAudio) ? "Audio":"Video", uDelta);

		if(this->bAudio)//�����������
		{
                if(uPlayQueueSize>5)
                {
                    uPlayFrameInterval = uPlayFrameInterval-1; //���ٲ���
                }
		}

		if(uDelta<uPlayFrameInterval*1000000)
		{
		    //usleep((uPlayFrameInterval*1000000 - uDelta)/1000);//us
		}

		//clock_gettime(CLOCK_MONOTONIC, &pretime); //��¼��֡���ŵ�ʱ���
		pretime = currtime;
#endif
              MTXW_LOGV("MTXW_Instance::PlayThread() %s  before play", (this->bAudio) ? "Audio":"Video");
              Play(pPlayData);
              ulPlayFrameNum ++;
		MTXW_LOGV("MTXW_Instance::PlayThread() %s  after play", (this->bAudio) ? "Audio":"Video");
		if( !this->bAudio && ulPlayFrameNum%100==0)
		{
		    MTXW_LOGE("framerate (playThread): ulPlayFrameNum=%d",ulPlayFrameNum);
		}
		
		//--3--�ͷ��ڴ�--------------------
		mtxwMemFree(pPlayData);
		
	}

	MTXW_LOGA("PlayThread Exit!");
}
void MTXW_Instance::ReceiveThread()
{

       MTXW_FUNC_TRACE();
	INT32   iReadlen;
	UINT32 uBufferSize;

       char pStrBuff[128];
	UINT32 index = 0;

        UINT16 usLastAudioSeqNumber = 0;

        UINT16 usLastVedioSeqNumber = 0;
        UINT16 usCurrentAudioSeq = 0;

        UINT16 usCurrentVedioSeq = 0;
        UINT8  ucRtp_PT;
        UINT8 ucRtp_V;
  
#if (defined WIN32 || WIN)	

#else
	struct sockaddr_in addr;
	socklen_t addr_len;
	bzero(&addr, sizeof(addr));

#endif

	fd_set  rset; 
	struct timeval tv;
	tv.tv_sec   = 0;  //��
	tv.tv_usec = 10000; //��λ΢��
	

	if(!this->m_pSockRecvBuffer)
	{
	       MTXW_LOGI("MTXW_Instance::ReceiveThread m_pSockRecvBuffer is NULL,EXIT!");
		return;
	}

	uBufferSize = this->m_pSockRecvBuffer->uSize;//��ʱ����sock receive buffer��size
	while(this->IsRunning())
	{
		iReadlen = 0;
			
		//--1--��socket�������ݰ�------------------
		this->m_pSockRecvBuffer->uSize = uBufferSize;//�ָ�buffer������size

#if 0
            FD_ZERO(&rset);
            FD_SET(this->mSock, &rset);
            tv.tv_sec   = 0;  //��
            tv.tv_usec = 10000; //��λ΢��

            if(select(this->mSock+1, &rset, NULL, NULL, &tv) <= 0){continue;}

            if(!FD_ISSET(this->mSock,&rset)){
            MTXW_LOGE("MTXW_Instance::ReceiveThread FD_ISSET!");
            continue;}
#endif

		ClearSockRecvBuffer();
		memset(pStrBuff,0,sizeof(pStrBuff));
		index = 0;

              {
              MTXW_lock   lock(&mhSocketMutex, "ReceiveThread SocketMutex");
#if (defined WIN32 || WIN)	
		
		iReadlen = recvfrom(this->mSock,
								(char*)this->m_pSockRecvBuffer->pData,
								this->m_pSockRecvBuffer->uSize,
								0,
								0,
								0);
		
#else
		iReadlen = recvfrom(this->mSock,
								(char*)this->m_pSockRecvBuffer->pData,
								this->m_pSockRecvBuffer->uSize,
								MSG_DONTWAIT,
								(struct sockaddr *)&addr,
								&addr_len);

#endif
              }
		MTXW_LOGV("MTXW_Instance::ReceiveThread iReadlen:%d",iReadlen);
		if(iReadlen <=0)
		{
		     usleep(5000);
		     continue;
		}		
		this->ulRcvBytes += iReadlen;
		this->ulRcvPacketNum++;

              for(int i = 0;i<iReadlen && i < 32;i++)
              {
                  index += sprintf(pStrBuff+index, "%02x ",(this->m_pSockRecvBuffer->pData[i]));
		}
#if (defined WIN32 || WIN)	

#else
		MTXW_LOGV("Receive one Packet,IP:%s,port:%d,size:%d",
		            inet_ntoa(addr.sin_addr),ntohs(addr.sin_port),iReadlen);
		MTXW_LOGV("MTXW_Instance::ReceiveThread ThreadId:%d Packet Context:%s",(int)pthread_self(),pStrBuff);

                ucRtp_V = this->m_pSockRecvBuffer->pData[0]>>6;
                ucRtp_PT = this->m_pSockRecvBuffer->pData[1]&0x7F;

                if(ucRtp_V==2&&ucRtp_PT!=0)//�ж�RTP�İ汾��PT�Ƿ�Ϸ�
                {

                    if(this->ulRcvPacketNum%100==99){//���if��Ϊ�˽���һ������ݺ��ٸ�������
                        if(this->enDirection == MTXW_DIRECTION_SEND_RECEIVE)
                        {
                            if(bFecSwitch && pRtpFecDecoder!=0){
                                if(false == pRtpFecDecoder->getFecFlag()){//˵���Զ˲�֧��FEC����
                                    //֪ͨ���˷���ʱ������FEC����
                                    this->bPeerSupportFec = false;
                                }else{
                                    this->bPeerSupportFec = true;
                                }
                            }
                        }
                    }
                    
                    if(this->bAudio)
                    {
                        usLastAudioSeqNumber = usCurrentAudioSeq;
                        usCurrentAudioSeq = ((this->m_pSockRecvBuffer->pData[2])<<8)|
                                                   (this->m_pSockRecvBuffer->pData[3]);
                        
                        /*MTXW_LOGE("MTXW_Instance::ReceiveThread Audio SN = %d",usCurrentAudioSeq);
                        if((usCurrentAudioSeq- usLastAudioSeqNumber)!=1)
                        {
                          MTXW_LOGE("MTXW_Instance::ReceiveThread Audio usCurrentSeq=%d usLastSeqNumber=%d",
                                                usCurrentAudioSeq,usLastAudioSeqNumber);
                        }*/                               
                    }
                    else
                    {
                        usLastVedioSeqNumber = usCurrentVedioSeq;
                        usCurrentVedioSeq = ((this->m_pSockRecvBuffer->pData[2])<<8)|
                                                   (this->m_pSockRecvBuffer->pData[3]);
                        /*MTXW_LOGE("MTXW_Instance::ReceiveThread Vedio SN = %d",usCurrentVedioSeq);
                        if((usCurrentVedioSeq- usLastVedioSeqNumber)!=1)
                        {
                            MTXW_LOGE("MTXW_Instance::ReceiveThread Vedio usCurrentSeq=%d usLastSeqNumber=%d",
                                                usCurrentVedioSeq,usLastVedioSeqNumber);
                        }*/             
                    }
                }
#endif

		this->m_pSockRecvBuffer->uSize = iReadlen;

              if(bFecSwitch && pRtpFecDecoder!=0){
		
		pRtpFecDecoder->input(this->m_pSockRecvBuffer->pData,this->m_pSockRecvBuffer->uSize);
		}
		else{
	
		
              
		//--2--RTP����-----------------------------
		//MTXW_LOGV("%s before ReceiveRtpPacket()",this->bAudio ? "audio" : "video");
	       this->ReceiveRtpPacket(this->m_pSockRecvBuffer);
	       //MTXW_LOGV("%s after ReceiveRtpPacket()",this->bAudio ? "audio" : "video");
	       }
	       
	}
	
	MTXW_LOGA("ReceiveThread Exit!");

}

void MTXW_Instance::StatisticThread()
{
    MTXW_FUNC_TRACE();

    UINT8 count = 0;
    UINT32 frameRatePeriod = 0;
    UINT32 preRcvFrameNum = 0;
    UINT32 preSndFrameNum = 0;
    const UINT32 FR_PERIOD = 3; 

    while(this->IsRunning())
    {
        
        if(count < 50)
        {
            count++;
            usleep(20000);//����20ms
            continue;
        }

        MTXW_CB::UpdateStatistic(this->uInstanceId, 0, this->ulSndBytes/1000.0);
        MTXW_CB::UpdateStatistic(this->uInstanceId, 1, this->ulRcvBytes/1000.0);
        MTXW_CB::UpdateStatistic(this->uInstanceId, 2, this->ulRcvPacketNum);
        if(!this->bAudio)
        {
            MTXW_CB::UpdateStatistic(this->uInstanceId, 3, this->GetDropPacketNum());
            MTXW_LOGV("this->uInstanceId=%d, DropPacketNum=%d" ,this->uInstanceId, this->GetDropPacketNum());
        }

        MTXW_LOGV("this->uInstanceId=%d, ulSndBytes=%lf" ,this->uInstanceId, this->ulSndBytes/1000.0);
        MTXW_LOGV("this->uInstanceId=%d, ulRcvBytes=%lf" ,this->uInstanceId, this->ulRcvBytes/1000.0);
        this->ulSndBytes = 0;
        this->ulRcvBytes = 0;

        if(++frameRatePeriod >= FR_PERIOD)//ÿ����ͳ��һ��֡��
        {
            this->ulSndFrameRate = (this->ulSndFrameNum - preSndFrameNum)/frameRatePeriod;
            preSndFrameNum = this->ulSndFrameNum;
            this->ulRcvFrameRate = (this->ulRcvFrameNum - preRcvFrameNum)/frameRatePeriod;
            preRcvFrameNum = this->ulRcvFrameNum;
            frameRatePeriod = 0;

            if(!this->bAudio){
                MTXW_LOGE("framerate sndFrameRate=%d RcvFrameRate=%d RcvFrameNum=%d",this->ulSndFrameRate,this->ulRcvFrameRate,this->ulRcvFrameNum);
            }
            _UpdCQI_RFR(this->ulRcvFrameRate);
        }

        
        count = 0;
    }
}


void MTXW_Instance::ClearEncodeBuffer()
{
    MTXW_FUNC_TRACE();
    this->m_pEncodeBuffer->uSize = MTXW_BUFFER_MSG_SIZE_MAX - sizeof(MTXW_DATA_BUFFER_STRU);
    mtxwMemSet(this->m_pEncodeBuffer->pData,0,this->m_pEncodeBuffer->uSize); 
}

void MTXW_Instance::ClearPackBuffer()
{
    this->m_pPackBuffer->uSize = MTXW_BUFFER_MSG_SIZE_MAX - sizeof(MTXW_DATA_BUFFER_STRU);
    mtxwMemSet(this->m_pPackBuffer->pData,0,this->m_pPackBuffer->uSize); 
}

void MTXW_Instance::ClearSockRecvBuffer()
{
    this->m_pSockRecvBuffer->uSize = MTXW_BUFFER_MSG_SIZE_MAX - sizeof(MTXW_DATA_BUFFER_STRU);
    mtxwMemSet(this->m_pSockRecvBuffer->pData,0,this->m_pSockRecvBuffer->uSize); 
}


void MTXW_Instance::_rtpDecodeOutput(UINT8 *pData, UINT16 ulDataLen)
{

    mtxwMemCopy(m_pFecDecodeBuffer->pData, pData, ulDataLen);
    m_pFecDecodeBuffer->uSize = ulDataLen;

    ReceiveRtpPacket(this->m_pFecDecodeBuffer);
    
    
}

void MTXW_Instance::_rtpEncodeOutput(bool bRFlag,UINT8 *pData, UINT16 ulDataLen)
{
    INT32 rslt;

#if (defined WIN32 || WIN)

#define MTXW_SOCKADDR SOCKADDR
	SOCKADDR_IN  DestAddr;
	DestAddr.sin_addr.S_un.S_addr = inet_addr(this->strRemoteAddr.c_str());
	DestAddr.sin_family = AF_INET;
	DestAddr.sin_port = htons(this->uRemotePort);
#else
#define MTXW_SOCKADDR sockaddr
	sockaddr_in DestAddr;
	bzero(&DestAddr,sizeof(DestAddr));  
	DestAddr.sin_family=AF_INET;  
	DestAddr.sin_addr.s_addr=inet_addr(this->strRemoteAddr.c_str());
	DestAddr.sin_port=htons(this->uRemotePort);  
#endif

    //--ģ�ⶪ�����������ʱʹ��

    if(usSimulationDropRate>0){
        UINT16 sn = (pData[2]<<8)|pData[3];
        UINT16 usRand = rand()%100;
        //UINT16 usDropRate = 2;
        if(usRand<usSimulationDropRate){
            return;
        }
    }
     rslt = _SendTo(pData,
                       ulDataLen,
                       0,
                       (MTXW_SOCKADDR*)&DestAddr, 
	             sizeof(DestAddr));

  
}

/*
*���ն˶Զ˷������ŵ�������Ϣ
*
*/
void MTXW_Instance::_RcvCQI(CqiStru cqi)
{
    /*
    mRcvCQI��Ȼ�ж��̲߳���(�����̺߳ͽ����߳�)��
    �����Բ���Ҫ�����̼߳���ͬ������Ϊ��
    ���ݲ�ͬ������������Σ����������
    CQI�������²���ʱ���Ժ��б�Ҫ�ټ�ͬ����
    */
    
    this->mRcvCQI = cqi;

    if(cqi.bAcqi){//Я���˹����ŵ���cqi������Ҫ֪ͨ�����ŵ�
        MTXW_CB::UpdAssoInstCQI(this->callId,this->uInstanceId, cqi.acqi);
    }

    if(this->bAdaptive){//����Ӧ�㷨���ش�
        //--����֪ͨFEC���������նԶ˷�����FEC��������FEC����
        this->pRtpFecEncoder->setGDLI(cqi.cqi.GDLI);
        this->pRtpFecEncoder->setRank(cqi.cqi.RI);
    }

    
}

void MTXW_Instance::SetFeedbackCQI()
{
    
    if(0 != this->pRtpFecEncoder 
    && this->bFecSwitch  //FEC���ش�
    && this->bAdaptive   //����Ӧ�㷨���ش�
    && MTXW_DIRECTION_SEND_RECEIVE == this->enDirection) //��������˫���
    {
        if(this->bAudio )//ͨ�������ŵ�����CQI��Ϣ
        {
            //--1)--��ȡ������Ƶ�ŵ���CQI��
            this->mSndCQI.bAcqi = MTXW_CB::GetAssoInstCQI(this->callId,this->uInstanceId, this->mSndCQI.acqi);

            //--2)--����FEC Encoder �������͸��Զ�
            this->pRtpFecEncoder->updateCQI(this->mSndCQI);

            //MTXW_LOGE("MTXW_Instance::SetFeedbackCQI this->mSndCQI.bAcqi = %d ",this->mSndCQI.bAcqi);
        }
    }
}

/*
*����FEC���������ݽ��յĶ������������CQI��Ϣ
*
*/
void MTXW_Instance::_UpdCQI_GDLI_RI(UINT8 GDLI,UINT8 RI)
{
    /*
    mSndCQI��Ȼ�ж��̲߳���(�����̺߳ͽ����߳�)��
    �����Բ���Ҫ�����̼߳���ͬ������Ϊ��
    ���ݲ�ͬ������������Σ����������
    CQI�������²���ʱ���Ժ��б�Ҫ�ټ�ͬ����
    */

    this->mSndCQI.cqi.GDLI = GDLI;
    this->mSndCQI.cqi.RI = RI;

    SetFeedbackCQI();


}

void MTXW_Instance::_UpdCQI_RFR(UINT8 RFR)
{
    /*
    mSndCQI��Ȼ�ж��̲߳���(�����̺߳ͽ����߳�)��
    �����Բ���Ҫ�����̼߳���ͬ������Ϊ��
    ���ݲ�ͬ������������Σ����������
    CQI�������²���ʱ���Ժ��б�Ҫ�ټ�ͬ����
    */

    this->mSndCQI.cqi.RFR = RFR;

    SetFeedbackCQI();
}

bool MTXW_Instance::GetCQIByAssInst(CqiInforStru &cqiInfo)
{
    //MTXW_LOGE("MTXW_Instance::GetCQIByAssInst this->mSndCQI.cqi.GDLI = %d this->mSndCQI.cqi.RFR =%d",
 //   this->mSndCQI.cqi.GDLI,this->mSndCQI.cqi.RFR);
    if(MTXW_DIRECTION_SEND_ONLY != this->enDirection 
    && this->mSndCQI.cqi.GDLI!=INVALID_CQI_VALUE 
    && this->mSndCQI.cqi.RFR != INVALID_CQI_VALUE)
    {
        cqiInfo.GDLI = this->mSndCQI.cqi.GDLI;
        cqiInfo.RI = this->mSndCQI.cqi.RI;
        cqiInfo.RFR = this->mSndCQI.cqi.RFR;
        return true;
    }else{
        return false;
    }
}

void MTXW_Instance::RcvCQIFromAssInst(CqiInforStru cqiInfo)
{
    this->mRcvCQI.cqi.GDLI = cqiInfo.GDLI;
    this->mRcvCQI.cqi.RI = cqiInfo.RI;
    this->mRcvCQI.cqi.RFR =cqiInfo.RFR;

    if(this->bAdaptive){//����Ӧ�㷨���ش�
        //--����֪ͨFEC���������նԶ˷�����FEC��������FEC����
        this->pRtpFecEncoder->setGDLI(cqiInfo.GDLI);
        this->pRtpFecEncoder->setRank(cqiInfo.RI);
    }
    
}

INT32 MTXW_Instance::SetPlayBuffer(UINT8 *pBuffer, INT32 Length)
{
     mPlayBuffer.pBuffer = pBuffer;
     mPlayBuffer.ulSize = Length;
     MTXW_LOGE("MTXW_Instance::SetPlayBuffer() uLength = %d mPlayBuffer.ulSize=%d",Length,mPlayBuffer.ulSize);
     return 0;
}

