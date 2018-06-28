/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_mediatransxw_PcmMix */
#include "PcmMix.h"
#ifndef _Included_com_mediatransxw_PcmMix
#define _Included_com_mediatransxw_PcmMix
#ifdef __cplusplus
extern "C" {
#endif

#include "com_mediatransxw_PcmMix.h"
/*
 * Class:     com_mediatransxw_PcmMix
 * Method:    Mix
 * Signature: ([BI[BI[BI)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_PcmMix_Mix
  (JNIEnv *env, jclass clazz, jbyteArray pcm1, jint len1, jbyteArray pcm2, jint len2, jbyteArray outBuff, jint outBuffLen)
{
	jbyte* inPcm1 = (env)->GetByteArrayElements(pcm1,0);
	jbyte* inPcm2 = (env)->GetByteArrayElements(pcm2,0);
	jbyte* inOutBuff = (env)->GetByteArrayElements(outBuff,0);
	
	int rslt = Mix((char*)inPcm1,len1,(char*)inPcm2,len2,(char*)inOutBuff,outBuffLen);
	
	(env)-> ReleaseByteArrayElements(pcm1, inPcm1, 0);
	(env)-> ReleaseByteArrayElements(pcm2, inPcm2, 0);
	(env)-> ReleaseByteArrayElements(outBuff, inOutBuff, 0);
	
	return rslt;
		  
}

#ifdef __cplusplus
}
#endif
#endif
