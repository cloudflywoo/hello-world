#include "com_mediatransxw_MediaTransXW.h"

#include "mtxw_controlblock.h"

#ifdef __cplusplus
extern "C" {
#endif

JavaVM *g_jvm;
jclass g_javaclass;
int g_MTXWSpeechEnLibDTime = 150;
int g_MTXWSpeechEnable = 0;  
bool g_MTXWActiveSpeechLib = 0; 


//It will enter this function when lib is load.
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {

       MTXW_LOGA("MTXW_LIB OnLoad");
        
	JNIEnv* env;
	g_jvm = vm;
	
	//always print mtxw_lib version name.
	MTXW_LOGA("%s",MTXW_CB::GetVersionName().c_str());
	
	if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
		MTXW_LOGE("JavaVM::GetEnv() failed");
		abort();
	}
/*
	gCachedFields.fileDescriptorClass =
			reinterpret_cast<jclass>(env->NewGlobalRef(
					env->FindClass("java/io/FileDescriptor")));
	if (gCachedFields.fileDescriptorClass == NULL) {
		abort();
	}

	gCachedFields.fileDescriptorCtor = env->GetMethodID(
			gCachedFields.fileDescriptorClass, "<init>", "()V");
	if (gCachedFields.fileDescriptorCtor == NULL) {
		abort();
	}

	gCachedFields.descriptorField = env->GetFieldID(
			gCachedFields.fileDescriptorClass, "descriptor", "I");
	if (gCachedFields.descriptorField == NULL) {
		abort();
	}
*/
	jclass jcl = env->FindClass("com/mediatransxw/MediaTransXW");
	if(NULL == jcl)
	{
		MTXW_LOGE("\n jcl is NULL");
		return -1;
	}
	
	g_javaclass = (jclass)env->NewGlobalRef(jcl);
	
	env->DeleteLocalRef(jcl);

	return JNI_VERSION_1_6;
}


//It will enter this function when lib.so is uninstalled
JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved)
 {
     MTXW_LOGA("MTXW_LIB OnUnload");
     
     return;
 }


/*
 * Class:     com_mediatransxw_MediaTransXW
 * Method:    mtxwNtvCreateInstance
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvCreateInstance
  (JNIEnv *env, jclass javaclass, jint type,jint callId){

       MTXW_FUNC_TRACE();
       
  	if(0 == type)
  	{
  		return MTXW_CB::CreateInstance(true,callId);
  	}
  	else if(1 == type)
  	{
  		return MTXW_CB::CreateInstance(false,callId);
  	}
  	else
  	{
  	    MTXW_LOGE("mtxwCreateInstance type:%d unknow!Please input 0 or 1.",type);
  	    return -1;
  	}
	
  }

/*
 * Class:     com_mediatransxw_MediaTransXW
 * Method:    mtxwNtvDestroyInstance
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvDestroyInstance
  (JNIEnv *, jclass, jint instanceid){
        MTXW_FUNC_TRACE();

        return MTXW_CB::DestroyInstance(instanceid);
  }

/*
 * Class:     com_mediatransxw_MediaTransXW
 * Method:    mtxwNtvStart
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvStart
  (JNIEnv *, jclass, jint instanceid){

        MTXW_FUNC_TRACE();
        return MTXW_CB::Start(instanceid);
  }

/*
 * Class:     com_mediatransxw_MediaTransXW
 * Method:    mtxwNtvStop
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvStop
  (JNIEnv *, jclass, jint instanceid){
  
        MTXW_FUNC_TRACE();

        return MTXW_CB::Stop(instanceid);
  }

/*
 * Class:     com_mediatransxw_MediaTransXW
 * Method:    mtxwNtvSetLocalAddress
 * Signature: (ILjava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvSetLocalAddress
  (JNIEnv *env, jclass, jint instanceid, jstring ip, jint port){
  
        MTXW_FUNC_TRACE();
        
        char* charipdata;
        int ret;
        charipdata = (char*)((env)->GetStringUTFChars(ip,0));
        std::string strip = charipdata;
        ret = MTXW_CB::SetLocalAddress(instanceid, strip, port);
        (env)->ReleaseStringUTFChars(ip, charipdata);

        return ret;
  }

/*
 * Class:     com_mediatransxw_MediaTransXW
 * Method:    mtxwNtvSetRemoteAddress
 * Signature: (ILjava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvSetRemoteAddress
  (JNIEnv *env, jclass, jint instanceid, jstring ip, jint port){
  
        MTXW_FUNC_TRACE();
        
        char* charipdata;
        int ret;
        charipdata = (char*)((env)->GetStringUTFChars(ip,0));
        std::string strip = charipdata;
        ret = MTXW_CB::SetRemoteAddress(instanceid, strip, port);
        (env)->ReleaseStringUTFChars(ip, charipdata);

        return ret;
  }

/*
 * Class:     com_mediatransxw_MediaTransXW
 * Method:    mtxwNtvSetInterface
 * Signature: (ILjava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvSetInterface
  (JNIEnv *env, jclass, jint instanceid, jstring strInterface){
  
        MTXW_FUNC_TRACE();
        
        char* charItfdata;
        int ret;
        charItfdata = (char*)((env)->GetStringUTFChars(strInterface,0));
        std::string strItf = charItfdata;
        ret = MTXW_CB::SetInterface(instanceid, strItf);
        (env)->ReleaseStringUTFChars(strInterface, charItfdata);

        return ret;
  }
/*
 * Class:     com_mediatransxw_MediaTransXW
 * Method:    mtxwNtvSetFecParam
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvSetFecParam
  (JNIEnv *, jclass, jint instanceId, jint FecSwitch, jint InputGDLI,jint InputRank,jint SimulationDropRate,jint adaptiveSwitch){
        MTXW_FUNC_TRACE();
         MTXW_LOGD("file:%s line:%d  function:%s instanceid:%d FecSwitch:%d InputGDLI:%d InputRank:%d SimulationDropRate:%d adaptiveSwitch:%d",
                         __FILE__,__LINE__,__FUNCTION__,instanceId,FecSwitch,InputGDLI,InputRank,SimulationDropRate,adaptiveSwitch);
        return MTXW_CB::SetFecParam(instanceId,FecSwitch,InputGDLI,InputRank,SimulationDropRate,adaptiveSwitch);
  }
  
/*
 * Class:     com_mediatransxw_MediaTransXW
 * Method:    mtxwNtvSetDirection
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvSetDirection
  (JNIEnv *, jclass, jint instanceid, jint direction){
  
        MTXW_FUNC_TRACE();

        MTXW_LOGD("file:%s line:%d  function:%s instanceid:%d direction:%d",
                         __FILE__,__LINE__,__FUNCTION__,instanceid,direction);

        if(MTXW_DIRECTION_SEND_RECEIVE > direction
        ||MTXW_DIRECTION_RECEIVE_ONLY < direction)
        {
            MTXW_LOGE("mtxwSetDirection direction:%d unknow!",direction);
            return -1;
        }

        return MTXW_CB::SetDirection(instanceid,(MTXW_ENUM_DIRECTION)direction);
  }

/*
 * Class:     com_mediatransxw_MediaTransXW
 * Method:    mtxwNtvSetAudioCodecType
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvSetAudioCodecType
  (JNIEnv *, jclass, jint instanceid, jint type){
  
        MTXW_FUNC_TRACE();

        if(MTXW_AUDIO_AMR > type||MTXW_AUDIO_MAX <= type)
        {
            MTXW_LOGE("mtxwSetAudioCodecType type:%d unknow!",type);
            return -1;
        }

        return MTXW_CB::SetAudioCodecType(instanceid,(MTXW_ENUM_AUDIO_CODEC_TYPE)type);
  }

/*
 * Class:     com_mediatransxw_MediaTransXW
 * Method:    mtxwNtvSetAudioSampleRate
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvSetAudioSampleRate
  (JNIEnv *, jclass, jint instanceid, jint sampleRate){
        MTXW_FUNC_TRACE();
        return MTXW_CB::SetAudioSampleRate(instanceid, sampleRate);
  }

/*
 * Class:     com_mediatransxw_MediaTransXW
 * Method:    mtxwNtvSetG729Param
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvSetG729Param
  (JNIEnv *, jclass, jint intanceId, jint frameTime){
        MTXW_FUNC_TRACE();
        return MTXW_CB::SetG729Param(intanceId, frameTime);
  }

/*
 * Class:     com_mediatransxw_MediaTransXW
 * Method:    mtxwNtvSetAmrParam
 * Signature: (IIII)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvSetAmrParam
  (JNIEnv *, jclass, jint instanceId, jint bitRate, jint frameTime, jint payloadFormat){
        MTXW_FUNC_TRACE();
        return MTXW_CB::SetAmrParam(instanceId,bitRate,frameTime,payloadFormat);
  }

/*
 * Class:     com_mediatransxw_MediaTransXW
 * Method:    mtxwNtvSetVideoCodecType
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvSetVideoCodecType
  (JNIEnv *, jclass, jint instanceId, jint type){
        MTXW_FUNC_TRACE();
        return MTXW_CB::SetVideoCodecType(instanceId,(MTXW_ENUM_VIDEO_CODEC_TYPE)type);
  }

/*
 * Class:     com_mediatransxw_MediaTransXW
 * Method:    mtxwNtvSetH264param
 * Signature: (IIIIIII)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvSetH264param
  (JNIEnv *, jclass, jint instanceId, jint frameRate,jint rtpRcvBufferSize,jint frameRcvBufferSize,jint initPlayFactor,jint discardflag,jint playYuvflag){
        MTXW_FUNC_TRACE();
        return MTXW_CB::SetH264param(instanceId,frameRate,rtpRcvBufferSize,frameRcvBufferSize,initPlayFactor,discardflag,playYuvflag);
  }

/*
 * Class:     com_mediatransxw_MediaTransXW
 * Method:    mtxwNtvSendAudioData
 * Signature: (I[BI)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvSendAudioData
  (JNIEnv *env, jclass, jint instanceid, jbyteArray data, jint length){
        MTXW_FUNC_TRACE();
        jbyte* bBuffer = (env)->GetByteArrayElements(data,0);
        int ret = MTXW_CB::SendAudioData(instanceid, (UINT8 *) bBuffer, length);
        (env)-> ReleaseByteArrayElements(data, bBuffer, 0);
        return ret;
  }

/*
 * Class:     com_mediatransxw_MediaTransXW
 * Method:    mtxwNtvSendVideoData
 * Signature: (I[BII)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvSendVideoData
  (JNIEnv *env, jclass, jint instanceid, jbyteArray data, jint  length, jint rotatedAngle){
        MTXW_FUNC_TRACE();
         jbyte* bBuffer = (env)->GetByteArrayElements(data,0);
        int ret = MTXW_CB::SendVideoData(instanceid, (UINT8 *) bBuffer, length,rotatedAngle);
        (env)-> ReleaseByteArrayElements(data, bBuffer, 0);
        return ret;
  }

  /*
 * Class:     com_mediatransxw_MediaTransXW
 * Method:    mtxwNtvSetSpeechEnLibDelayTime
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvSetSpeechEnLibDelayTime
  (JNIEnv *, jclass, jint time){
        MTXW_FUNC_TRACE();
        g_MTXWSpeechEnLibDTime = time;
  }
  
/*
 * Class:     com_mediatransxw_MediaTransXW
 * Method:    mtxwNtvSetLogLevel
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvSetLogLevel
  (JNIEnv *, jclass , jint level){
        MTXW_FUNC_TRACE();
        return MTXW_CB::SetLogLevel(level);
  }

/*
 * Class:     com_mediatransxw_MediaTransXW
 * Method:    mtxwNtvActiveSpeechLib
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvActiveSpeechLib
  (JNIEnv *, jclass , jint flag){
        MTXW_FUNC_TRACE();
        if(0 == flag)
        {
            g_MTXWActiveSpeechLib = false;
        }
        else
        {
            g_MTXWActiveSpeechLib = true;
        }
        return 0;
  }
  
  
/*
 * Class:     com_mediatransxw_MediaTransXW
 * Method:    mtxwNtvGetInstanceState
 * Signature: (I)I
 */
JNIEXPORT int JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvGetInstanceState
  (JNIEnv *, jclass , jint instanceid){
        MTXW_FUNC_TRACE();
        
        return MTXW_CB::GetInstanceState(instanceid);
  }
  

JNIEXPORT jint JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvSetRcvBuffer
  (JNIEnv *env, jclass, jint instanceid, jobject buffer, jint len)
{
    MTXW_FUNC_TRACE();
    unsigned char *pBuffer = (unsigned char*) (env)->GetDirectBufferAddress(buffer);
    MTXW_LOGE("Java_com_mediatransxw_MediaTransXW_mtxwNtvSetRcvBuffer() pBuff = %x len =%d",(unsigned int)pBuffer,len);
    if(pBuffer==NULL){
        return -1;
    }else{
        MTXW_CB::SetRcvBuff(instanceid, pBuffer, len);
        return 0;
    }
}


/*
 * Class:     com_mediatransxw_MediaTransXW
 * Method:    mtxwNtvGetVersionName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_mediatransxw_MediaTransXW_mtxwNtvGetVersionName
  (JNIEnv *env, jclass){
        MTXW_FUNC_TRACE();
        return env->NewStringUTF(MTXW_CB::GetVersionName().c_str());
  }

INT32 MTXW_PlayAudioPcmData(INT32 instId, const UINT8 *pData, UINT32 uLength)
{
    MTXW_FUNC_TRACE();
    
    jmethodID javaCallback;
    JNIEnv *env;	

    if (g_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK)
    {
    	g_jvm->AttachCurrentThread(&env, NULL);
    }

    if(NULL == env )
    {
        MTXW_LOGE("\n env is NULL");
        return -1;
    }

    javaCallback = env->GetStaticMethodID(g_javaclass, "playaudio","(I[BI)I");

    if(NULL == javaCallback)
    {
        MTXW_LOGE("\n javaCallback is NULL");
        return -1;
    }

    if(pData!= 0)
    {

        jbyteArray byteArray = env->NewByteArray(uLength);

        if(NULL == byteArray)
        {
            MTXW_LOGE("\n byteArray is NULL uLength:%d",uLength);
            g_jvm->DetachCurrentThread(); 
            return -1;
        }

        env->SetByteArrayRegion(byteArray,0,uLength,(jbyte*)pData);

        env->CallStaticIntMethod(g_javaclass, javaCallback, instId,byteArray,uLength);

        env->DeleteLocalRef(byteArray);
    }
    else
    {
        //MTXW_LOGE("MTXW_PlayAudioPcmData");
        env->CallStaticIntMethod(g_javaclass, javaCallback, instId,NULL,uLength);
    }
    
    g_jvm->DetachCurrentThread(); 

    return 0;
}

INT32 MTXW_PlayVedioH264Data(INT32 instId, const UINT8 *pData, UINT32 uLength,int rotatedAngle,int uWidth,int uHeight,int uSsrc)
{
    MTXW_FUNC_TRACE();
    
    jmethodID javaCallback;
    JNIEnv *env;	

    if (g_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK)
    {
    	g_jvm->AttachCurrentThread(&env, NULL);
    }

    if(NULL == env )
    {
        MTXW_LOGE("\n env is NULL");
        return -1;
    }

    javaCallback = env->GetStaticMethodID(g_javaclass, "playvedio","(I[BIIIII)I");

    if(NULL == javaCallback)
    {
        MTXW_LOGE("\n javaCallback is NULL");
        return -1;
    }

    if(pData!=0)
    {

        jbyteArray byteArray = env->NewByteArray(uLength);

        if(NULL == byteArray)
        {
            MTXW_LOGE("\n byteArray is NULL uLength:%d",uLength);
            g_jvm->DetachCurrentThread(); 
            return -1;
        }

        env->SetByteArrayRegion(byteArray,0,uLength,(jbyte*)pData);

        env->CallStaticIntMethod(g_javaclass, javaCallback, instId,byteArray,uLength,rotatedAngle,uWidth,uHeight,uSsrc);

        env->DeleteLocalRef(byteArray);
    }
    else
    {
        env->CallStaticIntMethod(g_javaclass, javaCallback, instId,NULL,uLength,rotatedAngle,uWidth,uHeight,uSsrc);
    }
    
    g_jvm->DetachCurrentThread(); 

    return 0;
}

INT32 MTXW_PlayVedioYUV(INT32 instId, const UINT8 *pData, UINT32 uLength,int rotatedAngle,int uWidth,int uHeight,int uSsrc)
{
    MTXW_FUNC_TRACE();
    
    jmethodID javaCallback;
    JNIEnv *env;	

    if (g_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK)
    {
    	g_jvm->AttachCurrentThread(&env, NULL);
    }

    if(NULL == env )
    {
        MTXW_LOGE("\n env is NULL");
        return -1;
    }

    javaCallback = env->GetStaticMethodID(g_javaclass, "playvedioyuv","(I[BIIIII)I");

    if(NULL == javaCallback)
    {
        MTXW_LOGE("\n javaCallback is NULL");
        return -1;
    }

    if(pData!=0)
    {
        jbyteArray byteArray = env->NewByteArray(uLength);

        if(NULL == byteArray)
        {
            MTXW_LOGE("\n byteArray is NULL uLength:%d",uLength);
            g_jvm->DetachCurrentThread(); 
            return -1;
        }

        env->SetByteArrayRegion(byteArray,0,uLength,(jbyte*)pData);

        env->CallStaticIntMethod(g_javaclass, javaCallback, instId,byteArray,uLength,rotatedAngle,uWidth,uHeight,uSsrc);

       // env-> ReleaseByteArrayElements(byteArray, (jbyte*)pData, 0);
        
        env->DeleteLocalRef(byteArray);
    }
    else
    {
        env->CallStaticIntMethod(g_javaclass, javaCallback, instId,NULL,uLength,rotatedAngle,uWidth,uHeight,uSsrc);
    }
    
    g_jvm->DetachCurrentThread(); 

    return 0;
}


INT32 MTXW_UpdateStatistic(INT32 instId, int statisticId, double statisticValue)
{
    MTXW_FUNC_TRACE();

    jmethodID javaCallback;
    JNIEnv *env;

    if (g_jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK)
    {
    	g_jvm->AttachCurrentThread(&env, NULL);
    }

    if(NULL == env )
    {
        MTXW_LOGE("\n env is NULL");
        return -1;
    }

    javaCallback = env->GetStaticMethodID(g_javaclass, "updatestatistic","(IID)I");

    if(NULL == javaCallback)
    {
        MTXW_LOGE("\n javaCallback is NULL");
        return -1;
    }

    env->CallStaticIntMethod(g_javaclass, javaCallback, instId,statisticId,statisticValue);

    g_jvm->DetachCurrentThread();

    return 0;
}


#ifdef __cplusplus
}

#endif
