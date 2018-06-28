/* Lame.java
   A port of LAME for Android

   Copyright (c) 2010 Ethan Chen

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/*****************************************************************************/
#include "lame-interface.h"
#include "lamecontrol.h"
#include "lame.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_CreateInstance
        (JNIEnv *env, jclass clazz)
{
    return LameControl::CreateInstance();
}

JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_DestroyInstance
        (JNIEnv *env, jclass clazz, jint instancId)
{
    return LameControl::DestroyInstance(instancId);
}

JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_initializeEncoder
  (JNIEnv *env, jclass clazz, jint instancId, jint sampleRate, jint numChannels)
{
    return LameControl::initializeEncoder(instancId, sampleRate, numChannels);
}

JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_setEncoderPreset
  (JNIEnv *env, jclass clazz, jint instanceId, jint preset)
{
  return LameControl::setEncoderPreset(instanceId, preset);
}


JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_encode
  (JNIEnv *env, jclass clazz, jint instancId, jshortArray leftChannel, jshortArray rightChannel,
		  jint channelSamples, jbyteArray mp3Buffer, jint bufferSize)
{
  int encoded_samples;
  short *left_buf, *right_buf;
  jbyte* mp3_buf;

  left_buf = (env)->GetShortArrayElements(leftChannel, NULL);
  right_buf = (env)->GetShortArrayElements(rightChannel, NULL);
  mp3_buf = (env)->GetByteArrayElements(mp3Buffer, NULL);

  encoded_samples = LameControl::encode(instancId, left_buf, right_buf, channelSamples, (unsigned char*)mp3_buf, bufferSize);

  // mode 0 means free left/right buf, write changes back to left/rightChannel
  (env)->ReleaseShortArrayElements(leftChannel, left_buf, 0);
  (env)->ReleaseShortArrayElements(rightChannel, right_buf, 0);

  if (encoded_samples < 0) {
    // don't propagate changes back up if we failed
    (env)->ReleaseByteArrayElements(mp3Buffer, mp3_buf, JNI_ABORT);
    return -1;
  }

  (env)->ReleaseByteArrayElements(mp3Buffer, mp3_buf, 0);
  return encoded_samples;
}

JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_encode2
        (JNIEnv *env, jclass clazz, jint instancId, jbyteArray leftChannel, jbyteArray rightChannel,
         jint channelSamples, jbyteArray mp3Buffer, jint bufferSize)
{
    int encoded_samples;
    jbyte *left_buf, *right_buf;
    jbyte* mp3_buf;

    int bufferLength = channelSamples / 2;
    short* bufLeft = new short[bufferLength];
    short* bufRight = new short[bufferLength];

    left_buf = (env)->GetByteArrayElements(leftChannel, NULL);
    right_buf = (env)->GetByteArrayElements(rightChannel, NULL);
    mp3_buf = (env)->GetByteArrayElements(mp3Buffer, NULL);

    for(int i=0, j=0; i<channelSamples; i+=2, j++){
        bufLeft[j] = (short)(((short)left_buf[i+1])<<8);
        bufLeft[j] |= ((short)left_buf[i] & 0x00ff);

        bufRight[j] = (short)(((short)right_buf[i+1])<<8);
        bufRight[j] |= ((short)right_buf[i] & 0x00ff);
    }

    encoded_samples = LameControl::encode(instancId, bufLeft, bufRight, bufferLength, (unsigned char*)mp3_buf, bufferSize);

    // mode 0 means free left/right buf, write changes back to left/rightChannel
    (env)->ReleaseByteArrayElements(leftChannel, left_buf, 0);
    (env)->ReleaseByteArrayElements(rightChannel, right_buf, 0);

    delete[] bufLeft;
    delete[] bufRight;

    if (encoded_samples < 0) {
        // don't propagate changes back up if we failed
        (env)->ReleaseByteArrayElements(mp3Buffer, mp3_buf, JNI_ABORT);
        return -1;
    }

    (env)->ReleaseByteArrayElements(mp3Buffer, mp3_buf, 0);
    return encoded_samples;
}
JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_flushEncoder
  (JNIEnv *env, jclass clazz, jint instancId, jbyteArray mp3Buffer, jint bufferSize)
{
  // call lame_encode_flush when near the end of pcm buffer
  int num_bytes;
  jbyte *mp3_buf;

  mp3_buf = (env)->GetByteArrayElements(mp3Buffer, NULL);

  num_bytes = LameControl::flushEncoder(instancId, (unsigned char*)mp3_buf, bufferSize);
  if (num_bytes < 0) {
    // some kind of error occurred, don't propagate changes to buffer
	(env)->ReleaseByteArrayElements(mp3Buffer, mp3_buf, JNI_ABORT);
    return num_bytes;
  }

  (env)->ReleaseByteArrayElements(mp3Buffer, mp3_buf, 0);
  return num_bytes;
}


JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_closeEncoder
  (JNIEnv *env, jclass clazz, jint instancId)
{
  return LameControl::closeEncoder(instancId);
}


JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_initializeDecoder
  (JNIEnv *env, jclass clazz, jint instanceId)
{
  return LameControl::initializeDecoder(instanceId);
}


JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_nativeConfigureDecoder
  (JNIEnv *env, jclass clazz, jint instanceId, jbyteArray mp3Buffer, jint bufferSize)
{
  int ret = -1;
  jbyte *mp3_buf;
  mp3_buf = (env)->GetByteArrayElements(mp3Buffer, NULL);

  ret = LameControl::nativeConfigureDecoder(instanceId, (unsigned char*)mp3_buf, bufferSize);
  (env)->ReleaseByteArrayElements(mp3Buffer, mp3_buf, 0);
  return ret;
}


JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_getDecoderChannels
  (JNIEnv *env, jclass clazz, jint instanceId)
{
  return LameControl::getDecoderChannels(instanceId);
}


JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_getDecoderSampleRate
  (JNIEnv *env, jclass clazz, jint instanceId)
{
  return LameControl::getDecoderSampleRate(instanceId);
}


JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_getDecoderDelay
  (JNIEnv *env, jclass clazz, jint instanceId)
{
  return LameControl::getDecoderDelay(instanceId);
}


JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_getDecoderPadding
  (JNIEnv *env, jclass clazz, jint instanceId)
{
  return LameControl::getDecoderPadding(instanceId);
}


JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_getDecoderTotalFrames
  (JNIEnv *env, jclass clazz, jint instanceId)
{
  return LameControl::getDecoderTotalFrames(instanceId);
}


JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_getDecoderFrameSize
  (JNIEnv *env, jclass clazz, jint instanceId)
{
  return LameControl::getDecoderFrameSize(instanceId);
}


JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_getDecoderBitrate
  (JNIEnv *env, jclass clazz, jint instanceId)
{
  return LameControl::getDecoderBitrate(instanceId);
}


JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_nativeDecodeFrame
  (JNIEnv *env, jclass clazz, jint instanceId, jbyteArray mp3Buffer, jint bufferSize,
		  jshortArray rightChannel, jshortArray leftChannel)
{
  int samples_read;
  short *left_buf, *right_buf;
  jbyte *mp3_buf;

  left_buf = (env)->GetShortArrayElements(leftChannel, NULL);
  right_buf = (env)->GetShortArrayElements(rightChannel, NULL);
  mp3_buf = (env)->GetByteArrayElements(mp3Buffer, NULL);

  samples_read = LameControl::nativeDecodeFrame(instanceId, (unsigned char*)mp3_buf, bufferSize, right_buf, left_buf);

  (env)->ReleaseByteArrayElements(mp3Buffer, mp3_buf, 0);

  if (samples_read < 0) {
    // some sort of error occurred, don't propagate changes to buffers
	(env)->ReleaseShortArrayElements(leftChannel, left_buf, JNI_ABORT);
	(env)->ReleaseShortArrayElements(rightChannel, right_buf, JNI_ABORT);
    return samples_read;
  }

  (env)->ReleaseShortArrayElements(leftChannel, left_buf, 0);
  (env)->ReleaseShortArrayElements(rightChannel, right_buf, 0);

  return samples_read;
}


JNIEXPORT jint JNICALL Java_com_mediatransxw_Lame_closeDecoder
  (JNIEnv *env, jclass clazz, jint instanceId)
{
  return LameControl::closeDecoder(instanceId);
}
