#ifndef __LAMECONTROL_H__
#define __LAMECONTROL_H__

#include <stdio.h>
#include <stdlib.h>
#include "lame.h"
#include "malloc.h"
#include <android/log.h>

#include <map>

class LameControl{
private:
    static std::map<int, LameControl*> gMapInstance;

    lame_global_flags *lame_context;
    hip_t hip_context;
    mp3data_struct *mp3data;
    int enc_delay, enc_padding;

private:
    //构造函数私有化
    LameControl()
    {
        hip_context = NULL;
    }

public:
    static int CreateInstance();
    static int DestroyInstance(int instanceId);
    static int initializeEncoder(int instanceId, int sampleRate, int numChannels);
    static int setEncoderPreset(int instanceId, int present);
    static int encode(int instanceId, short* leftChannel, short* rightChannel, int channelSamples, unsigned char* mp3Buffer, int bufferSize);
    static int flushEncoder(int instanceId, unsigned char* mp3Buffer, int bufferSize);
    static int closeEncoder(int instanceId);

    static int initializeDecoder(int instanceId);
    static int nativeConfigureDecoder(int instanceId, unsigned char* mp3Buffer, int bufferSize);
    static int getDecoderChannels(int instanceId);
    static int getDecoderSampleRate(int instanceId);
    static int getDecoderDelay(int instanceId);
    static int getDecoderPadding(int instanceId);
    static int getDecoderTotalFrames(int instanceId);
    static int getDecoderFrameSize(int instanceId);
    static int getDecoderBitrate(int instanceId);
    static int nativeDecodeFrame(int instanceId, unsigned char* mp3Buffer, int bufferSize, short* rightChannel,
                                 short* leftChannel);
    static int closeDecoder(int instanceId);
};

#endif