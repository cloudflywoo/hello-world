#include "mtxw_instance.h"
#include "mtxw_controlblock.h"
#include <string.h>
#include <time.h>
#include <net/if.h>

//buffer消息的最大长度
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

	//--传输参数	
	this->enDirection = MTXW_DIRECTION_SEND_RECEIVE; //传输方向
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
  
	//--线程
	this->sendThread = MTXW_INVALID_THREAD_HANDLE;
	this->receiveThread = MTXW_INVALID_THREAD_HANDLE;
	this->playThread = MTXW_INVALID_THREAD_HANDLE;

	//--队列最大长度设置
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


	//--socket接收buffer
	if((this->m_pSockRecvBuffer = (MTXW_DATA_BUFFER_STRU* ) mtxwMemAlloc(MTXW_BUFFER_MSG_SIZE_MAX)))//分配64k内存
	{
		this->m_pSockRecvBuffer->uSize = MTXW_BUFFER_MSG_SIZE_MAX - sizeof(MTXW_DATA_BUFFER_STRU);
	}
	else
	{
		//分配内存失败
		MTXW_LOGE("\n Start(): allocate mem for m_pSockRecvBuffer fail!");

	}

	//--编码buffer
	if((this->m_pEncodeBuffer = (MTXW_DATA_BUFFER_STRU* ) mtxwMemAlloc(MTXW_BUFFER_MSG_SIZE_MAX)))//分配64k内存
	{
		this->m_pEncodeBuffer->uSize = MTXW_BUFFER_MSG_SIZE_MAX - sizeof(MTXW_DATA_BUFFER_STRU);
	}
	else
	{
		//分配内存失败
		MTXW_LOGE("\n Start(): allocate mem for m_pEncodeBuffer fail!");

	}
	//--编码buffer
	if((this->m_pPackBuffer = (MTXW_DATA_BUFFER_STRU* ) mtxwMemAlloc(MTXW_BUFFER_MSG_SIZE_MAX)))//分配64k内存
	{
		this->m_pPackBuffer->uSize = MTXW_BUFFER_MSG_SIZE_MAX - sizeof(MTXW_DATA_BUFFER_STRU);
	}
	else
	{
		//分配内存失败
		MTXW_LOGE("\n Start(): allocate mem for m_pPackBuffer fail!");

	}

	this->m_pDecodeBuffer = 0;

	if((this->m_pFecDecodeBuffer = (MTXW_DATA_BUFFER_STRU* ) mtxwMemAlloc(sizeof(MTXW_DATA_BUFFER_STRU)+1500)))//分配64k内存
	{
		this->m_pFecDecodeBuffer->uSize = 1500;
	}
	else
	{
		//分配内存失败
		MTXW_LOGE("\n Start(): allocate mem for m_pPackBuffer fail!");

	}
	
	
}
MTXW_Instance::~MTXW_Instance()
{     
    
        MTXW_FUNC_TRACE();
	if(this->IsRunning())
	{
	    this->Stop();//停止线程
	}
	
	//--释放队列里的内存---------//

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
        
      	//--1--创建socket---------------------
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
            //--创建FEC编码器
            pRtpFecEncoder = new RtpFecEncoder(this->rtpEncodeOutput,this,this->usEncodeRank,this->usEncodeGDLI);

            //--创建FEC解码器
            pRtpFecDecoder = new RtpFecDecoder(this->rtpDecodeOutput,RcvCQI,UpdCQI_GDLI_RI,this);
        }
        this->bPeerSupportFec = true;
        
      	//--2--创建线程--------------------

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
        //---停止线程---------------------
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
        //--关闭socket-----------------------
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



	//--清空各种消息队列----------
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
	
        //打印统计量
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
	//绑定本端端口
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
	
        //绑定本端端口
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

        //--设置socket的接收发送缓存大小
       
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
	//绑定本端端口
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
	
    //关闭本地回环
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
    mreq.imr_multiaddr.S_un.S_addr = inet_addr(this->strLocalAddr.c_str());  //广播ip地址
    mreq.imr_interface.s_addr = inet_addr(INADDR_ANY);  //要监听的网口
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
	
    //绑定本端端口
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

    mreq.imr_multiaddr.s_addr=inet_addr(this->strLocalAddr.c_str());  //广播ip地址 
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
        mreq.imr_interface.s_addr=htonl(INADDR_ANY);   //要监听的网口
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
	{    //退出组播ip监听
		 struct ip_mreq mreq;  
         mreq.imr_multiaddr.S_un.S_addr = inet_addr(this->strLocalAddr.c_str());  //广播ip地址
         mreq.imr_interface.s_addr = inet_addr(INADDR_ANY);  //要监听的网口
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
	
    //codec编码，pInData-入参，内容为原始采样数据(如PCM)  pOutData-出参，内容为编码后的输出(如AMR)
INT32 MTXW_Instance::Encode(const MTXW_DATA_BUFFER* pInData,MTXW_DATA_BUFFER* pOutData)
{
        MTXW_FUNC_TRACE();
	return 0;
}
	
	//decode解码，pInData-入参，内容为编码后的数据(如AMR)  pOutData-出参，内容为解码后的输出(如PCM)
INT32 MTXW_Instance::Decode(const MTXW_DATA_BUFFER* pInData,MTXW_DATA_BUFFER* pOutData)
{
        MTXW_FUNC_TRACE();
	return 0;
}
	
	//打包函数，输入是未打包的数据，输出是打包后的数据
	//pPackedData: 调用者必须提供足够大的缓存用于输出打包数据
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
        //分配内存失败
        MTXW_LOGE("mtxwMemAlloc fail!");
        return -1;
    }

    mtxwMemCopy(pBuffer->pData, pInputData->pData, pBuffer->uSize);


    
    {
         MTXW_lock   lock(this->mPlayQueue.getMutex(),"MTXW_Instance::ReceiveRtpPacket mPlayQueue");//数据队列上锁

         this->mPlayQueue.push(pBuffer); //数据放入播放队列
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
*功能:负责在socket上发送数据
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
    tv.tv_sec   = 0;  //秒
    tv.tv_usec = 10000; //单位微秒
    UINT32 ulResendCount = 0;




    for(int i = 0;i<usOnePackLen && i < 32;i++)
    {
      index += sprintf(pStrBuff+index, "%02x ",pData[i]);
    }
    MTXW_LOGD("MTXW_Instance::SendThread thread_id:%d one Packet Context:%s",(int)pthread_self(),pStrBuff);

    
    RESEND:
#if 1

    //---1---等待socket状态变为可写状态----------------------
    while(this->IsRunning())
    {
        FD_ZERO(&wset);
        FD_SET(this->mSock, &wset);
        tv.tv_sec   = 0;  //秒
        tv.tv_usec = 10000; //单位微秒

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

    //--2--发送失败后进行重发------
    if(rslt != usOnePackLen)
    {
        ulResendCount++;
        if(ulResendCount>=10)
        {
            //连续10次发送失败，则重启socket
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
		
		//--1--从发送队列取数据-------------------
		{
		       MTXW_lock   lock(this->mSendQueue.getMutex(),"MTXW_Instance::SendThread  mSendQueue");//数据队列上锁
                    if(!this->IsRunning()){break;} //防止Stop时的广播的信号丢失
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
		
		//--2--编码-------------------------------
		
		this->Encode(pSendData,pSendData);
                
		//--3--RTP打包----------------------------
		ClearPackBuffer();//打包buffer初始化
		this->Pack(pSendData, this->m_pPackBuffer);           

		//--4--socket发送-------------------------

		//静音包发送,长度为0的是静音包,2秒一个静音包
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
		          //出错了 
		          MTXW_LOGE("MTXW_Instance::SendThread. The rest of the buff %d > %d. is not enough to send!",(ulTotalLen+usOnePackLen),(this->m_pPackBuffer->uSize));
		          break;
		      }

		      rslt = SendTo(&this->m_pPackBuffer->pData[ulTotalLen],
		                 usOnePackLen,
		                 0,//MSG_DONTWAIT,
		                 (MTXW_SOCKADDR*)&DestAddr, 
		                 sizeof(DestAddr)); 

		      ulTotalLen += usOnePackLen;

		      ulSendPacketCnt++;//发送包数统计
		      //---2018.4.2--单向发送的FEC信息通过关联的语音实例获取

        		if(false==this->bAudio && this->enDirection == MTXW_DIRECTION_SEND_ONLY){
        		    if(ulSendPacketCnt%50==49){//这个if条件是为了:1)启动一会儿后再查询，2)防止频繁调用
                                   //单向的视频发送，通过语音通道来判断是否对方是否支持FEC
                                   this->bPeerSupportFec = MTXW_CB::isAssoInstSupportFec(this->callId,this->uInstanceId);
        		    }
                       }
		}
		

		//--5--释放内存---------------------------
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
	struct timespec pretime={0,0};//前一次播放的时间
#endif


	unsigned long uPlayFrameInterval;
	unsigned long uDelta;

	bool  initFlag = true;
	unsigned long uPlayQueueSize=0;
	
	while(this->IsRunning())
	{
		pPlayData = 0;
		//--1--从播放队列拿出数据------------
		{
			MTXW_lock   lock(this->mPlayQueue.getMutex(),"MTXW_Instance::PlayThread mPlayQueue");//数据队列上锁
			if(!this->IsRunning()){break;} //防止Stop时的广播的信号丢失

			uPlayQueueSize = this->mPlayQueue.size();
			
			if(this->mPlayQueue.empty() || 
			   ((true==initFlag)&&GetInitPlayQueueSize()>uPlayQueueSize))   //初始播放条件
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
		
		//--2--调用接口播放----------------
		uPlayFrameInterval = this->GetPlayFrameInterval();//ms
#ifndef WIN32
		clock_gettime(CLOCK_MONOTONIC, &currtime); //获取当前时间
		
		if(currtime.tv_nsec>pretime.tv_nsec)
		{
		    uDelta = (currtime.tv_sec - pretime.tv_sec)*1000000000 + (currtime.tv_nsec - pretime.tv_nsec);//nano second
		}
		else
		{
		    uDelta = (currtime.tv_sec - pretime.tv_sec - 1)*1000000000 + (currtime.tv_nsec + 1000000000 - pretime.tv_nsec);//nano second
		}

		MTXW_LOGV("MTXW_Instance::PlayThread() %s  uDelta = %ld!", (this->bAudio) ? "Audio":"Video", uDelta);

		if(this->bAudio)//语音快进策略
		{
                if(uPlayQueueSize>5)
                {
                    uPlayFrameInterval = uPlayFrameInterval-1; //加速播放
                }
		}

		if(uDelta<uPlayFrameInterval*1000000)
		{
		    //usleep((uPlayFrameInterval*1000000 - uDelta)/1000);//us
		}

		//clock_gettime(CLOCK_MONOTONIC, &pretime); //记录本帧播放的时间点
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
		
		//--3--释放内存--------------------
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
	tv.tv_sec   = 0;  //秒
	tv.tv_usec = 10000; //单位微秒
	

	if(!this->m_pSockRecvBuffer)
	{
	       MTXW_LOGI("MTXW_Instance::ReceiveThread m_pSockRecvBuffer is NULL,EXIT!");
		return;
	}

	uBufferSize = this->m_pSockRecvBuffer->uSize;//临时保存sock receive buffer的size
	while(this->IsRunning())
	{
		iReadlen = 0;
			
		//--1--从socket接收数据包------------------
		this->m_pSockRecvBuffer->uSize = uBufferSize;//恢复buffer的正常size

#if 0
            FD_ZERO(&rset);
            FD_SET(this->mSock, &rset);
            tv.tv_sec   = 0;  //秒
            tv.tv_usec = 10000; //单位微秒

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

                if(ucRtp_V==2&&ucRtp_PT!=0)//判断RTP的版本和PT是否合法
                {

                    if(this->ulRcvPacketNum%100==99){//这个if是为了接收一会儿数据后再更新数据
                        if(this->enDirection == MTXW_DIRECTION_SEND_RECEIVE)
                        {
                            if(bFecSwitch && pRtpFecDecoder!=0){
                                if(false == pRtpFecDecoder->getFecFlag()){//说明对端不支持FEC功能
                                    //通知本端发送时不进行FEC编码
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
	
		
              
		//--2--RTP处理-----------------------------
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
            usleep(20000);//休眠20ms
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

        if(++frameRatePeriod >= FR_PERIOD)//每三秒统计一次帧率
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

    //--模拟丢包，仿真测试时使用

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
*接收端对端反馈的信道质量信息
*
*/
void MTXW_Instance::_RcvCQI(CqiStru cqi)
{
    /*
    mRcvCQI虽然有多线程操作(发送线程和接收线程)，
    但可以不需要进行线程加锁同步，因为其
    数据不同步不会带来大的危害，最多就是
    CQI反馈更新不及时；以后有必要再加同步锁
    */
    
    this->mRcvCQI = cqi;

    if(cqi.bAcqi){//携带了关联信道的cqi，则需要通知关联信道
        MTXW_CB::UpdAssoInstCQI(this->callId,this->uInstanceId, cqi.acqi);
    }

    if(this->bAdaptive){//自适应算法开关打开
        //--立刻通知FEC编码器按照对端反馈的FEC参数进行FEC编码
        this->pRtpFecEncoder->setGDLI(cqi.cqi.GDLI);
        this->pRtpFecEncoder->setRank(cqi.cqi.RI);
    }

    
}

void MTXW_Instance::SetFeedbackCQI()
{
    
    if(0 != this->pRtpFecEncoder 
    && this->bFecSwitch  //FEC开关打开
    && this->bAdaptive   //自适应算法开关打开
    && MTXW_DIRECTION_SEND_RECEIVE == this->enDirection) //数据流是双向的
    {
        if(this->bAudio )//通过语音信道反馈CQI信息
        {
            //--1)--获取关联视频信道的CQI，
            this->mSndCQI.bAcqi = MTXW_CB::GetAssoInstCQI(this->callId,this->uInstanceId, this->mSndCQI.acqi);

            //--2)--传给FEC Encoder 反馈发送给对端
            this->pRtpFecEncoder->updateCQI(this->mSndCQI);

            //MTXW_LOGE("MTXW_Instance::SetFeedbackCQI this->mSndCQI.bAcqi = %d ",this->mSndCQI.bAcqi);
        }
    }
}

/*
*本端FEC解码器根据接收的丢包情况产生的CQI信息
*
*/
void MTXW_Instance::_UpdCQI_GDLI_RI(UINT8 GDLI,UINT8 RI)
{
    /*
    mSndCQI虽然有多线程操作(发送线程和接收线程)，
    但可以不需要进行线程加锁同步，因为其
    数据不同步不会带来大的危害，最多就是
    CQI反馈更新不及时；以后有必要再加同步锁
    */

    this->mSndCQI.cqi.GDLI = GDLI;
    this->mSndCQI.cqi.RI = RI;

    SetFeedbackCQI();


}

void MTXW_Instance::_UpdCQI_RFR(UINT8 RFR)
{
    /*
    mSndCQI虽然有多线程操作(发送线程和接收线程)，
    但可以不需要进行线程加锁同步，因为其
    数据不同步不会带来大的危害，最多就是
    CQI反馈更新不及时；以后有必要再加同步锁
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

    if(this->bAdaptive){//自适应算法开关打开
        //--立刻通知FEC编码器按照对端反馈的FEC参数进行FEC编码
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

