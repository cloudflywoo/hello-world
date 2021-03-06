/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_mediatransxw_Lame */

#ifndef _Included_com_mediatransxw_Lame
#define _Included_com_mediatransxw_Lame
#ifdef __cplusplus
extern "C" {
#endif
#undef com_mediatransxw_Lame_MP3_BUFFER_SIZE
#define com_mediatransxw_Lame_MP3_BUFFER_SIZE 1024L
#undef com_mediatransxw_Lame_LAME_PRESET_DEFAULT
#define com_mediatransxw_Lame_LAME_PRESET_DEFAULT 0L
#undef com_mediatransxw_Lame_LAME_PRESET_MEDIUM
#define com_mediatransxw_Lame_LAME_PRESET_MEDIUM 1L
#undef com_mediatransxw_Lame_LAME_PRESET_STANDARD
#define com_mediatransxw_Lame_LAME_PRESET_STANDARD 2L
#undef com_mediatransxw_Lame_LAME_PRESET_EXTREME
#define com_mediatransxw_Lame_LAME_PRESET_EXTREME 3L

JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_CreateInstance
        (JNIEnv *env, jclass clazz);

JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_DestroyInstance
        (JNIEnv *env, jclass clazz, jint instancId);
/*
 * Class:     com_mediatransxw_Lame
 * Method:    initializeEncoder
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_initializeEncoder
  (JNIEnv *, jclass, jint, jint, jint);

/*
 * Class:     com_mediatransxw_Lame
 * Method:    setEncoderPreset
 * Signature: (I)V
 */
JNIEXPORT int JNICALL Java_com_mediatransxw_Lame_setEncoderPreset
  (JNIEnv *, jclass, jint, jint);

/*
 * Class:     com_mediatransxw_Lame
 * Method:    encode
 * Signature: ([S[SI[BI)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_encode
  (JNIEnv *, jclass, jint, jshortArray, jshortArray, jint, jbyteArray, jint);

JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_encode2
        (JNIEnv *, jclass, jint, jbyteArray, jbyteArray, jint, jbyteArray, jint);
/*
 * Class:     com_mediatransxw_Lame
 * Method:    flushEncoder
 * Signature: ([BI)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_flushEncoder
  (JNIEnv *, jclass, jint, jbyteArray, jint);

/*
 * Class:     com_mediatransxw_Lame
 * Method:    closeEncoder
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_closeEncoder
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_mediatransxw_Lame
 * Method:    initializeDecoder
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_initializeDecoder
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_mediatransxw_Lame
 * Method:    getDecoderSampleRate
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_getDecoderSampleRate
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_mediatransxw_Lame
 * Method:    getDecoderChannels
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_getDecoderChannels
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_mediatransxw_Lame
 * Method:    getDecoderDelay
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_getDecoderDelay
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_mediatransxw_Lame
 * Method:    getDecoderPadding
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_getDecoderPadding
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_mediatransxw_Lame
 * Method:    getDecoderTotalFrames
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_getDecoderTotalFrames
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_mediatransxw_Lame
 * Method:    getDecoderFrameSize
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_getDecoderFrameSize
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_mediatransxw_Lame
 * Method:    getDecoderBitrate
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_getDecoderBitrate
  (JNIEnv *, jclass, jint);

/*
 * Class:     com_mediatransxw_Lame
 * Method:    nativeConfigureDecoder
 * Signature: ([BI)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_nativeConfigureDecoder
  (JNIEnv *, jclass, jint, jbyteArray, jint);

/*
 * Class:     com_mediatransxw_Lame
 * Method:    nativeDecodeFrame
 * Signature: ([BI[S[S)I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_nativeDecodeFrame
  (JNIEnv *, jclass, jint, jbyteArray, jint, jshortArray, jshortArray);

/*
 * Class:     com_mediatransxw_Lame
 * Method:    closeDecoder
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_closeDecoder
  (JNIEnv *, jclass, jint);

#ifdef __cplusplus
}
#endif
#endif
