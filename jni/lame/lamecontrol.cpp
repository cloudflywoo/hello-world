#include "lamecontrol.h"
#include "lamecontrol_define.h"

using namespace std;

//---静态成员变量初始化---------------------------------------------------------------
map<int, LameControl*> LameControl::gMapInstance = map<int, LameControl*>();

int LameControl::CreateInstance()
{
    //__android_log_print(ANDROID_LOG_INFO, "file:%s line:%d function:%s", __FILE__, __LINE__, __FUNCTION__);

    if(LAME_INSTANCE_MAX <= gMapInstance.size())
    {
        __android_log_print(ANDROID_LOG_ERROR, "CreateInstance", "gMapInstance.size() >= LAME_INSTANCE_MAX! gMapInstance.size():%d", gMapInstance.size());
        return -1;
    }

    LameControl* pInstance = new(std::nothrow) LameControl();
    if(!pInstance){
        __android_log_print(ANDROID_LOG_ERROR, "CreateInstance", "Cannot new a LameControl instance.");
        return -1;
    }

    int key = (int)pInstance;
    gMapInstance.insert(pair<int,LameControl*>(key, pInstance));

    __android_log_print(ANDROID_LOG_INFO, "CreateInstance", "gMapInstance.size(): %d", gMapInstance.size());

    return key;
}

int LameControl::DestroyInstance(int instanceId)
{
    //__android_log_print(ANDROID_LOG_INFO, "file:%s line:%d function:%s", __FILE__, __LINE__, __FUNCTION__);

    map<int, LameControl*>::iterator  iter = gMapInstance.end();
    LameControl* pInstance = 0;

    iter = gMapInstance.find(instanceId);

    if(gMapInstance.end() == iter)
    {
        __android_log_print(ANDROID_LOG_ERROR, "DestroyInstance", "DestroyInstance instanceId %d not found!", instanceId);
        return -1;
    }
    pInstance = iter->second;
    gMapInstance.erase(iter);//从媒体实例列表删除

    int ret = 0;
    if (pInstance->lame_context) {
        ret = lame_close(pInstance->lame_context);
        pInstance->lame_context = NULL;
        __android_log_print(ANDROID_LOG_DEBUG, "DestroyInstance", "lame_close with code %d", ret);
    }

    if (pInstance->hip_context) {
        ret = hip_decode_exit(pInstance->hip_context);
        pInstance->hip_context = NULL;
        free(pInstance->mp3data);
        pInstance->mp3data = NULL;
        pInstance->enc_delay = -1;
        pInstance->enc_padding = -1;
        __android_log_print(ANDROID_LOG_DEBUG, "DestroyInstance", "hip_decode_exit with code %d", ret);
    }

    delete pInstance;
    __android_log_print(ANDROID_LOG_INFO, "DestroyInstance", "instanceId: %d gMapInstance.size(): %d", instanceId, gMapInstance.size());
    return  ret;
}

int LameControl::initializeEncoder(int instanceId, int sampleRate, int numChannels)
{
    //__android_log_print(ANDROID_LOG_INFO, "file:%s line:%d function:%s", __FILE__, __LINE__, __FUNCTION__);

    __android_log_print(ANDROID_LOG_INFO, "initializeEncoder", "instanceId: %d sampleRate: %d numChannels: %d", instanceId, sampleRate, numChannels);

    LameControl* pInstance = 0;

    map<int, LameControl*>::iterator iter = gMapInstance.end();
    iter = gMapInstance.find(instanceId);
    if(gMapInstance.end() == iter)
    {
        __android_log_print(ANDROID_LOG_ERROR, "initializeEncoder", "initializeEncoder instanceId %d not found!", instanceId);
        return -1;
    }
    pInstance = iter->second;

    pInstance->lame_context = lame_init();
    if (pInstance->lame_context) {
        lame_set_in_samplerate(pInstance->lame_context, sampleRate);
        lame_set_num_channels(pInstance->lame_context, numChannels);
        int ret = lame_init_params(pInstance->lame_context);
        __android_log_print(ANDROID_LOG_DEBUG, "initializeEncoder", "initialized lame with code %d", ret);
        return ret;
    }

    return -1;
}

int LameControl::setEncoderPreset(int instanceId, int present)
{
    //__android_log_print(ANDROID_LOG_INFO, "file:%s line:%d function:%s", __FILE__, __LINE__, __FUNCTION__);

    __android_log_print(ANDROID_LOG_INFO, "setEncoderPreset", "instanceId: %d present: %d", instanceId, present);

    LameControl* pInstance = 0;

    map<int, LameControl*>::iterator iter = gMapInstance.end();
    iter = gMapInstance.find(instanceId);
    if(gMapInstance.end() == iter)
    {
        __android_log_print(ANDROID_LOG_ERROR, "setEncoderPreset", "setEncoderPreset instanceId %d not found!", instanceId);
        return -1;
    }
    pInstance = iter->second;

    switch (present) {
        case net_sourceforge_lame_Lame_LAME_PRESET_MEDIUM:
            lame_set_VBR_q(pInstance->lame_context, 4);
            lame_set_VBR(pInstance->lame_context, vbr_rh);
            break;
        case net_sourceforge_lame_Lame_LAME_PRESET_STANDARD:
            lame_set_VBR_q(pInstance->lame_context, 2);
            lame_set_VBR(pInstance->lame_context, vbr_rh);
            break;
        case net_sourceforge_lame_Lame_LAME_PRESET_EXTREME:
            lame_set_VBR_q(pInstance->lame_context, 0);
            lame_set_VBR(pInstance->lame_context, vbr_rh);
            break;
        case net_sourceforge_lame_Lame_LAME_PRESET_DEFAULT:
        default:
            break;
    }

    __android_log_print(ANDROID_LOG_INFO, "setEncoderPreset", "instanceId: %d", instanceId);
    return 0;
}

int LameControl::encode(int instanceId, short* leftChannel, short* rightChannel, int channelSamples, unsigned char* mp3Buffer, int bufferSize)
{
    int encoded_samples;

    //__android_log_print(ANDROID_LOG_INFO, "file:%s line:%d function:%s", __FILE__, __LINE__, __FUNCTION__);

    //__android_log_print(ANDROID_LOG_INFO, "setEncoderPreset", "instanceId: %d channelSamples: %d bufferSize: %d", instanceId, channelSamples, bufferSize);

    LameControl* pInstance = 0;

    map<int, LameControl*>::iterator iter = gMapInstance.end();
    iter = gMapInstance.find(instanceId);
    if(gMapInstance.end() == iter)
    {
        //__android_log_print(ANDROID_LOG_ERROR, "setEncoderPreset", "setEncoderPreset instanceId %d not found!", instanceId);
        return -1;
    }
    pInstance = iter->second;

    encoded_samples = lame_encode_buffer(pInstance->lame_context, leftChannel, rightChannel, channelSamples, mp3Buffer, bufferSize);

    if (encoded_samples < 0) {
        //__android_log_print(ANDROID_LOG_DEBUG, "encode", "encoded_samples: %d", encoded_samples);
    }

    return encoded_samples;
}

int LameControl::flushEncoder(int instanceId, unsigned char* mp3Buffer, int bufferSize)
{
    //__android_log_print(ANDROID_LOG_INFO, "file:%s line:%d function:%s", __FILE__, __LINE__, __FUNCTION__);

    //__android_log_print(ANDROID_LOG_INFO, "setEncoderPreset", "instanceId: %d bufferSize: %d", instanceId, bufferSize);

    LameControl* pInstance = 0;

    map<int, LameControl*>::iterator iter = gMapInstance.end();
    iter = gMapInstance.find(instanceId);
    if(gMapInstance.end() == iter)
    {
        //__android_log_print(ANDROID_LOG_ERROR, "setEncoderPreset", "setEncoderPreset instanceId %d not found!", instanceId);
        return -1;
    }
    pInstance = iter->second;

    int num_bytes;
    num_bytes = lame_encode_flush(pInstance->lame_context, mp3Buffer, bufferSize);
    if (num_bytes < 0) {
        //__android_log_print(ANDROID_LOG_DEBUG, "flushEncoder", "num_bytes: %d", num_bytes);
    }

    return num_bytes;
}

int LameControl::closeEncoder(int instanceId)
{
    //__android_log_print(ANDROID_LOG_INFO, "file:%s line:%d function:%s", __FILE__, __LINE__, __FUNCTION__);

    __android_log_print(ANDROID_LOG_INFO, "closeEncoder", "instanceId: %d", instanceId);

    LameControl* pInstance = 0;

    map<int, LameControl*>::iterator iter = gMapInstance.end();
    iter = gMapInstance.find(instanceId);
    if(gMapInstance.end() == iter)
    {
        __android_log_print(ANDROID_LOG_ERROR, "closeEncoder", "closeEncoder instanceId %d not found!", instanceId);
        return -1;
    }
    pInstance = iter->second;

    if (pInstance->lame_context) {
        int ret = lame_close(pInstance->lame_context);
        pInstance->lame_context = NULL;
        __android_log_print(ANDROID_LOG_DEBUG, "closeEncoder", "freed lame with code %d", ret);
        return ret;
    }
    return -1;
}

int LameControl::initializeDecoder(int instanceId)
{
    //__android_log_print(ANDROID_LOG_INFO, "file:%s line:%d function:%s", __FILE__, __LINE__, __FUNCTION__);

    __android_log_print(ANDROID_LOG_INFO, "initializeDecoder", "instanceId: %d", instanceId);

    LameControl* pInstance = 0;

    map<int, LameControl*>::iterator iter = gMapInstance.end();
    iter = gMapInstance.find(instanceId);
    if(gMapInstance.end() == iter)
    {
        __android_log_print(ANDROID_LOG_ERROR, "initializeDecoder", "initializeDecoder instanceId %d not found!", instanceId);
        return -1;
    }
    pInstance = iter->second;

    if (!pInstance->hip_context) {
        pInstance->hip_context = hip_decode_init();
        if (pInstance->hip_context) {
            pInstance->mp3data = (mp3data_struct *) malloc(sizeof(mp3data_struct));
            memset(pInstance->mp3data, 0, sizeof(mp3data_struct));
            pInstance->enc_delay = -1;
            pInstance->enc_padding = -1;
            return 0;
        }
    }
    __android_log_print(ANDROID_LOG_INFO, "initializeDecoder", "initializeDecoder hip_context %d", (int)pInstance->hip_context);
    return -1;
}

int LameControl::nativeConfigureDecoder(int instanceId, unsigned char* mp3Buffer, int bufferSize)
{
    //__android_log_print(ANDROID_LOG_INFO, "file:%s line:%d function:%s", __FILE__, __LINE__, __FUNCTION__);

    __android_log_print(ANDROID_LOG_INFO, "nativeConfigureDecoder", "instanceId: %d", instanceId);

    LameControl* pInstance = 0;

    map<int, LameControl*>::iterator iter = gMapInstance.end();
    iter = gMapInstance.find(instanceId);
    if(gMapInstance.end() == iter)
    {
        __android_log_print(ANDROID_LOG_ERROR, "nativeConfigureDecoder", "nativeConfigureDecoder instanceId %d not found!", instanceId);
        return -1;
    }
    pInstance = iter->second;

    int ret = -1;
    short left_buf[1152], right_buf[1152];

    if (pInstance->mp3data) {
        ret = hip_decode1_headersB(pInstance->hip_context, mp3Buffer, bufferSize,
                                   left_buf, right_buf, pInstance->mp3data, &pInstance->enc_delay, &pInstance->enc_padding);
        if (pInstance->mp3data->header_parsed) {
            pInstance->mp3data->totalframes = pInstance->mp3data->nsamp / pInstance->mp3data->framesize;
            ret = 0;
            __android_log_print(ANDROID_LOG_DEBUG, "nativeConfigureDecoder", "decoder configured successfully");
            __android_log_print(ANDROID_LOG_DEBUG, "nativeConfigureDecoder", "sample rate: %d, channels: %d", pInstance->mp3data->samplerate, pInstance->mp3data->stereo);
            __android_log_print(ANDROID_LOG_DEBUG, "nativeConfigureDecoder", "bitrate: %d, frame size: %d", pInstance->mp3data->bitrate, pInstance->mp3data->framesize);
        } else {
            ret = -1;
        }
    }

    return ret;
}

int LameControl::getDecoderChannels(int instanceId)
{
    //__android_log_print(ANDROID_LOG_INFO, "file:%s line:%d function:%s", __FILE__, __LINE__, __FUNCTION__);

    //__android_log_print(ANDROID_LOG_INFO, "getDecoderChannels", "instanceId: %d", instanceId);

    LameControl* pInstance = 0;

    map<int, LameControl*>::iterator iter = gMapInstance.end();
    iter = gMapInstance.find(instanceId);
    if(gMapInstance.end() == iter)
    {
        //__android_log_print(ANDROID_LOG_ERROR, "getDecoderChannels", "getDecoderChannels instanceId %d not found!", instanceId);
        return -1;
    }
    pInstance = iter->second;

    return pInstance->mp3data->stereo;
}

int LameControl::getDecoderSampleRate(int instanceId)
{
    //__android_log_print(ANDROID_LOG_INFO, "file:%s line:%d function:%s", __FILE__, __LINE__, __FUNCTION__);

    //__android_log_print(ANDROID_LOG_INFO, "getDecoderSampleRate", "instanceId: %d", instanceId);

    LameControl* pInstance = 0;

    map<int, LameControl*>::iterator iter = gMapInstance.end();
    iter = gMapInstance.find(instanceId);
    if(gMapInstance.end() == iter)
    {
        //__android_log_print(ANDROID_LOG_ERROR, "getDecoderSampleRate", "getDecoderSampleRate instanceId %d not found!", instanceId);
        return -1;
    }
    pInstance = iter->second;

    return pInstance->mp3data->samplerate;
}

int LameControl::getDecoderDelay(int instanceId)
{
    //__android_log_print(ANDROID_LOG_INFO, "file:%s line:%d function:%s", __FILE__, __LINE__, __FUNCTION__);

    //__android_log_print(ANDROID_LOG_INFO, "getDecoderDelay", "instanceId: %d", instanceId);

    LameControl* pInstance = 0;

    map<int, LameControl*>::iterator iter = gMapInstance.end();
    iter = gMapInstance.find(instanceId);
    if(gMapInstance.end() == iter)
    {
        //__android_log_print(ANDROID_LOG_ERROR, "getDecoderDelay", "getDecoderDelay instanceId %d not found!", instanceId);
        return -1;
    }
    pInstance = iter->second;

    return pInstance->enc_delay;
}

int LameControl::getDecoderPadding(int instanceId)
{
    //__android_log_print(ANDROID_LOG_INFO, "file:%s line:%d function:%s", __FILE__, __LINE__, __FUNCTION__);

    //__android_log_print(ANDROID_LOG_INFO, "getDecoderPadding", "instanceId: %d", instanceId);

    LameControl* pInstance = 0;

    map<int, LameControl*>::iterator iter = gMapInstance.end();
    iter = gMapInstance.find(instanceId);
    if(gMapInstance.end() == iter)
    {
        //__android_log_print(ANDROID_LOG_ERROR, "getDecoderPadding", "getDecoderPadding instanceId %d not found!", instanceId);
        return -1;
    }
    pInstance = iter->second;

    return pInstance->enc_padding;
}

int LameControl::getDecoderTotalFrames(int instanceId)
{
    //__android_log_print(ANDROID_LOG_INFO, "file:%s line:%d function:%s", __FILE__, __LINE__, __FUNCTION__);

    //__android_log_print(ANDROID_LOG_INFO, "getDecoderTotalFrames", "instanceId: %d", instanceId);

    LameControl* pInstance = 0;

    map<int, LameControl*>::iterator iter = gMapInstance.end();
    iter = gMapInstance.find(instanceId);
    if(gMapInstance.end() == iter)
    {
        //__android_log_print(ANDROID_LOG_ERROR, "getDecoderTotalFrames", "getDecoderTotalFrames instanceId %d not found!", instanceId);
        return -1;
    }
    pInstance = iter->second;

    return pInstance->mp3data->totalframes;
}
int LameControl::getDecoderFrameSize(int instanceId)
{
    //__android_log_print(ANDROID_LOG_INFO, "file:%s line:%d function:%s", __FILE__, __LINE__, __FUNCTION__);

    //__android_log_print(ANDROID_LOG_INFO, "getDecoderFrameSize", "instanceId: %d", instanceId);

    LameControl* pInstance = 0;

    map<int, LameControl*>::iterator iter = gMapInstance.end();
    iter = gMapInstance.find(instanceId);
    if(gMapInstance.end() == iter)
    {
        //__android_log_print(ANDROID_LOG_ERROR, "getDecoderFrameSize", "getDecoderFrameSize instanceId %d not found!", instanceId);
        return -1;
    }
    pInstance = iter->second;

    return pInstance->mp3data->framesize;
}

int LameControl::getDecoderBitrate(int instanceId)
{
    //__android_log_print(ANDROID_LOG_INFO, "file:%s line:%d function:%s", __FILE__, __LINE__, __FUNCTION__);

    __android_log_print(ANDROID_LOG_INFO, "getDecoderBitrate", "instanceId: %d", instanceId);

    LameControl* pInstance = 0;

    map<int, LameControl*>::iterator iter = gMapInstance.end();
    iter = gMapInstance.find(instanceId);
    if(gMapInstance.end() == iter)
    {
        __android_log_print(ANDROID_LOG_ERROR, "getDecoderBitrate", "getDecoderBitrate instanceId %d not found!", instanceId);
        return -1;
    }
    pInstance = iter->second;

    return pInstance->mp3data->bitrate;
}

int LameControl::nativeDecodeFrame(int instanceId, unsigned char* mp3Buffer, int bufferSize, short* rightChannel,
                             short* leftChannel)
{
    //__android_log_print(ANDROID_LOG_INFO, "file:%s line:%d function:%s", __FILE__, __LINE__, __FUNCTION__);

    //__android_log_print(ANDROID_LOG_INFO, "nativeDecodeFrame", "instanceId: %d", instanceId);

    LameControl* pInstance = 0;

    map<int, LameControl*>::iterator iter = gMapInstance.end();
    iter = gMapInstance.find(instanceId);
    if(gMapInstance.end() == iter)
    {
        //__android_log_print(ANDROID_LOG_ERROR, "nativeDecodeFrame", "nativeDecodeFrame instanceId %d not found!", instanceId);
        return -1;
    }
    pInstance = iter->second;

    int samples_read;
    samples_read = hip_decode1_headers(pInstance->hip_context, mp3Buffer, bufferSize, leftChannel, rightChannel, pInstance->mp3data);

    if (samples_read < 0) {
        //__android_log_print(ANDROID_LOG_DEBUG, "nativeDecodeFrame", "samples_read: %d", samples_read);
    }

    return samples_read;
}

int LameControl::closeDecoder(int instanceId)
{
    //__android_log_print(ANDROID_LOG_INFO, "file:%s line:%d function:%s", __FILE__, __LINE__, __FUNCTION__);

    __android_log_print(ANDROID_LOG_INFO, "closeDecoder", "instanceId: %d", instanceId);

    LameControl* pInstance = 0;

    map<int, LameControl*>::iterator iter = gMapInstance.end();
    iter = gMapInstance.find(instanceId);
    if(gMapInstance.end() == iter)
    {
        __android_log_print(ANDROID_LOG_ERROR, "closeDecoder", "closeDecoder instanceId %d not found!", instanceId);
        return -1;
    }
    pInstance = iter->second;

    if (pInstance->hip_context) {
        int ret = hip_decode_exit(pInstance->hip_context);
        pInstance->hip_context = NULL;
        free(pInstance->mp3data);
        pInstance->mp3data = NULL;
        pInstance->enc_delay = -1;
        pInstance->enc_padding = -1;
        return ret;
    }
    return -1;
}