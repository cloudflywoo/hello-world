
#ifndef __MTXW_COMM_H__
#define __MTXW_COMM_H__

#include "mtxw.h"
#include "mtxw_osapi.h"

#include "mtxw_h264payloadheader.h"
#include <string.h>
#include <queue>
#include <vector>
#include <string>
#include <map>
#include <list>

extern UINT32 gMTXWLogSwitch;

#if (defined WIN32 || WIN)

#else
#include <jni.h>
#include <android/log.h>
#endif

#if (defined WIN32 || WIN)

#define MTXW_LOGV  ((void)0)
#define MTXW_LOGD  ((void)0)
#define MTXW_LOGI   ((void)0)
#define MTXW_LOGW  ((void)0)
#define MTXW_LOGE   ((void)0)
#define MTXW_LOGF   ((void)0)
#define MTXW_LOGA  ((void)0)


//-1.函数轨迹打印-
#define MTXW_FUNC_TRACE(strFuncName)  ((void)0)

#else


/*6:verbose 5:debug 4:info 3:warning 2:erro 1:fatal  0:close*/

#define MTXW_LOGV(...)   \
    (gMTXWLogSwitch>= 6) \
    ? ((void)__android_log_print(ANDROID_LOG_VERBOSE, "MTXW_LIB",__VA_ARGS__))\
    : ((void)0)


#define MTXW_LOGD(...)  \
    (gMTXWLogSwitch >= 5) \
    ?((void)__android_log_print(ANDROID_LOG_DEBUG, "MTXW_LIB", __VA_ARGS__)) \
    : ((void)0) 
    
#define MTXW_LOGI(...)   \
    (gMTXWLogSwitch>= 4) \
    ? ((void)__android_log_print(ANDROID_LOG_INFO, "MTXW_LIB",__VA_ARGS__))\
    : ((void)0)
    
#define MTXW_LOGW(...)  \
    (gMTXWLogSwitch >= 3) \
    ? ((void)__android_log_print(ANDROID_LOG_WARN, "MTXW_LIB", __VA_ARGS__)) \
    : ((void)0)
#define MTXW_LOGE(...)   \
    (gMTXWLogSwitch >= 2) \
    ? ((void)__android_log_print(ANDROID_LOG_ERROR, "MTXW_LIB", __VA_ARGS__)) \
    : ((void)0)

#define MTXW_LOGF(...)   \
    (gMTXWLogSwitch >= 1) \
    ? ((void)__android_log_print(ANDROID_LOG_FATAL, "MTXW_LIB", __VA_ARGS__)) \
    : ((void)0)
    
#define MTXW_LOGA(...)  \
    (gMTXWLogSwitch >= 0) \
    ? ((void)__android_log_print(ANDROID_LOG_INFO, "MTXW_LIB",__VA_ARGS__))\
    : ((void)0)
    
//-1.函数轨迹打印-
#define MTXW_FUNC_TRACE() MTXW_LOGD("file:%s line:%d function:%s",__FILE__,__LINE__,__FUNCTION__)

#endif

//--1---枚举定义------------------------------------
enum MTXW_ENUM_AUDIO_CODEC_TYPE
{
   MTXW_AUDIO_AMR = 0,
   MTXW_AUDIO_G729,
   MTXW_AUDIO_MAX

};
enum MTXW_ENUM_AMR_ALIGN_MODE
{
    MTXW_AMR_OA_MODE = 0, //字节对模式
    MTXW_AMR_BE,    //节省带宽模式
};
enum MTXW_ENUM_VIDEO_CODEC_TYPE
{
   MTXW_VIDEO_H264 = 0,
   MTXW_VIDEO_MAX
};


enum MTXW_ENUM_DIRECTION
{
   MTXW_DIRECTION_SEND_RECEIVE = 0,
   MTXW_DIRECTION_SEND_ONLY,
   MTXW_DIRECTION_RECEIVE_ONLY
};

enum MTXW_ENUM_STATE
{
    MTXW_STATE_INIT = 0,
    MTXW_STATE_RUNING,
    MTXW_STATE_STOP
};

enum MTXW_ENUM_H264_FU_A_TYPE
{
    MTXW_COMPLETE_NALU = 0,
    MTXW_FU_A_START,
    MTXW_FU_A_NO_START_END,
    MTXW_FU_A_END,
    MTXW_FU_A_MAX
};

enum MTXW_ENUM_FRAME_TYPE
{
    MTXW_FRAME_TYPE_IDR = 0,
    MTXW_FRAME_TYPE_NON_IDR,
    MTXW_FRAME_TYPE_MAX,    
};

enum MTXW_ENUM_CONTINUE_DROP_FRAME_TYPE
{
    MTXW_STOP_DROP_FRAME = 0,//不丢
    MTXW_CONTINUE_DROP_DAMAGE_FRAME, //由于烂帧而丢弃
    MTXW_CONTINUE_DROP_FULL_FRAME, //由于播放队列满而丢弃
};

//--2--struct声明------------------------------------
#pragma pack(push,1)

typedef struct MTXW_PCM_DATA_HEADER
{
	
}MTXW_PCM_DATA_HEADER_STRU;


typedef struct MTXW_H264_DATA_HEADER
{
    UINT16 uRotatedAngle;
    UINT16 uWidth;
    UINT16 uHeight;
    UINT32 ulSsrc; 

    //2017-11-14- added
    bool isNonIDRFrame; //true- 为MTXW_NALU_TYPE_1_NON_IDR 帧   
}MTXW_H264_DATA_HEADER_STRU;

typedef struct MTXW_H264_FRAGMENT_HEADER
{
    UINT32 SN;  //sequence number, 从RTP 头获取
    bool     bMark;//从RTP 头获取
    MTXW_H264_RTP_PAYLOAD_HEADER_UNION  rtpPayloadHeader;
     
}MTXW_H264_FRAGMENT_HEADER_STRU;

typedef union MTXW_DATA_HEADER
{
    MTXW_PCM_DATA_HEADER_STRU pcm_data_header;
    MTXW_H264_DATA_HEADER_STRU h264_data_header;
    MTXW_H264_FRAGMENT_HEADER_STRU h264_fragment_header;
    
}MTXW_DATA_HEADER_UNION;

typedef struct MTXW_DATA_BUFFER
{
    MTXW_DATA_HEADER_UNION header;

    unsigned int uSize;
    unsigned char pData[0];	
}MTXW_DATA_BUFFER_STRU;

typedef struct MTXW_VEDIO_COUNT
{
    UINT32 rtp_drop_cnt;
    UINT32 rtp_rcv_cnt;
    UINT32 rtp_buffer_size;
    UINT32 rtp_lost_cnt;
    UINT32 frame_drop_for_damage_cnt;
    UINT32 frame_drop_for_full_cnt;
    UINT32 frame_rcv_cnt;
    UINT32 frame_buffer_size;
    
}MTXW_VEDIO_COUNT_STRU;

#pragma pack(pop)
typedef MTXW_DATA_BUFFER_STRU*  MTXW_DATA_BUFFER_POINTER;

//--3---class声明-------------------------------------

class MTXW_lock
{
private:
    MTXW_MUTEX_HANDLE *mhMutex;
    std::string tag;
    
public:
    MTXW_lock(MTXW_MUTEX_HANDLE *hMutex,std::string tag)
    {
		#ifndef WIN32

        MTXW_LOGI("MTXW_lock %s befor lock ",tag.c_str());
        mhMutex=hMutex;
        this->tag = tag;
        mtxwLockMutex(mhMutex);
        MTXW_LOGI("MTXW_lock %s after lock ",tag.c_str());
		#endif
    }
    virtual ~MTXW_lock()
    {
		#ifndef WIN32
        MTXW_LOGI("~MTXW_lock %s befor unlock ",tag.c_str());
        mtxwUnlockMutex(mhMutex);
        MTXW_LOGI("~MTXW_lock %s after unlock ",tag.c_str());
		#endif
    }

};

template <class T>
class MTXW_queue : public std::queue<T>
{
private:
     MTXW_MUTEX_HANDLE mhMutex;  
  
	 
#if (defined WIN32 || WIN)
     HANDLE mReady;
#else
     pthread_cond_t    mReady;
#endif

public:
    MTXW_queue():std:: queue<T>()
    {
       
        mhMutex = mtxwCreateMutex();

#if (defined WIN32 || WIN)
        mReady = INVALID_HANDLE_VALUE;
#else
        mReady = PTHREAD_COND_INITIALIZER;
#endif
					
    }
    
    MTXW_MUTEX_HANDLE *getMutex()
    {
        return &mhMutex;
    }
    virtual ~MTXW_queue()
    {
        mtxwReleaseMutex(&mhMutex);
		
#if (defined WIN32 || WIN)
		// do nothing now, some optimazation can be done in future;
#else
	    pthread_cond_broadcast(&mReady);
	    pthread_cond_destroy(&mReady);
#endif
		
		
    }
	
	void push (const T& val)//重载push函数，数据插入时发送一个信号量，唤醒读等待的线程
	{
		std::queue<T>::push(val);
#if (defined WIN32 || WIN)
    	// do nothing now, some optimazation can be done in future;
#else
        pthread_cond_broadcast(&mReady);
#endif
    }
	
    void wait() //调用者调用该函数前，必须对mhMutex上锁
    {

#if (defined WIN32 || WIN)
            Sleep(10);
#else
            pthread_cond_wait(&mReady, &mhMutex);
#endif

    	
    }

    void signal()
    {
#if (defined WIN32 || WIN)

#else
        {  
            pthread_cond_broadcast(&mReady);
        }
#endif
        
    }

    
};

template <class T_KEY, class T_VALUE>
class MTXW_map : public std::map<T_KEY,T_VALUE>
{
private:
     MTXW_MUTEX_HANDLE mhMutex;

public:
     MTXW_map():std:: map<T_KEY,T_VALUE>()
    {
        mhMutex = mtxwCreateMutex();
    }
    
     MTXW_MUTEX_HANDLE *getMutex()
    {
        return &mhMutex;
    }

    virtual ~MTXW_map()
    {
        mtxwReleaseMutex(&mhMutex);
    }
    
};





template <class Tx>
class MTXW_sortlist:private std::list<Tx>
{
private:
    /*
    return value:
    0:  E1 == E2
    1:  E1 >  E2
    -1:  E1 <  E2
    */
    int (*mCompareFun) (const Tx & E1,const Tx & E2);
public:

    MTXW_sortlist():std:: list<Tx>()
    {
    	mCompareFun = 0;
    }
    void setcomparefun(int(*compare)(const Tx& element1,const Tx& element2)){mCompareFun=compare;}
    typename std::list<Tx>::iterator end(){return std::list<Tx>::end();}
    typename std::list<Tx>::iterator begin(){return std::list<Tx>::begin();}
    void clear(){std::list<Tx>::clear();}
    bool empty(){return std::list<Tx>::empty();}
    typename std::list<Tx>::iterator erase (typename std::list<Tx>::iterator position){return std::list<Tx>::erase();}
    unsigned int size(){return std::list<Tx>::size();}

    void pop_back(){std::list<Tx>::pop_back();}
    void pop_front(){std::list<Tx>::pop_front();}

    bool iscontain(const Tx & val)
    {
        typename std::list<Tx>::iterator it=begin();
        for (it=begin(); it!=end(); ++it)
    	 {
            if(0==mCompareFun(val,*it)){return true;}
        }

        return false;
       
    }

    void push (const Tx & val)
    {
    	if(empty())
    	{
    		std::list<Tx>::push_back(val);
    		return;
    	}

    	//--1--search the insert position-------------
    	typename std::list<Tx>::iterator it=begin();
    	typename std::list<Tx>::iterator pos=end();
    	int rslt;
    	
    	for (pos=end(); pos!=begin(); --pos)
    	{
			it = pos;
            rslt = mCompareFun(val,*(--it));
            if(-1==rslt)
            {
                	// val <*it
                	continue;

            }else if(0==rslt)
            {
                	// val == *it
                	break;
            }else
            {
                	// val > *it
                	break;
            }
    	}
    	//--2--insert the data into list--------------

    	std::list<Tx>::insert(pos,val);
    }


};




//--4---全局函数/宏函数声明------------------------------------

//----内存操作函数----------------//
void *mtxwMemAlloc(UINT usize);
void  mtxwMemFree(void* p);
INT32 mtxwGetMemCnt();
#define mtxwMemSet(p,c,size) memset((p),(c),(size))
#define mtxwMemCopy(dest,src,size)   memcpy((dest),(src),(size))
#define mtxwMemMove(dest,src,count)  memmove((dest),(src),(count))
#define mtxwMemCmp(buf1,buf2,count)  memcmp((buf1),(buf2),(count))


UINT16 mtxwHtons(UINT16 hostShort);
UINT16 mtxwNtohs(UINT16 netShort);
UINT32 mtxwHtonl(UINT32 hostShort);
UINT32 mtxwNtohl(UINT32 netShort);

std::string mtxwGetBuildTime();



#endif  //__MTXW_COMM_H__


