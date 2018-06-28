#ifdef __cplusplus
extern "C" {
#endif

#define SIZE_AUDIO_FRAME 2
/****************************************************************************
*归一化混音(自适应加权混音算法)
*来源: http://blog.csdn.net/dancing_night/article/details/53080819
*
****************************************************************************/
void _Mix(char sourseFile[10][SIZE_AUDIO_FRAME],int number,char *objectFile);
/****************************************************************
*newlc上的一个关于PCM脉冲编码的音频信号的混音实现
*
*来源: http://blog.csdn.net/dancing_night/article/details/53080819
****************************************************************/
short _Mix2(short data1,short data2);

int Mix(char * pcm1,int len1,char *pcm2,int len2,char *outBuff, int outBuffLen);
int Mix2(char * pcm1,int len1,char *pcm2,int len2,char *outBuff, int outBuffLen);

#ifdef __cplusplus
}
#endif