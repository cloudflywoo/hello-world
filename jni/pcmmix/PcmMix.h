#ifdef __cplusplus
extern "C" {
#endif

#define SIZE_AUDIO_FRAME 2
/****************************************************************************
*��һ������(����Ӧ��Ȩ�����㷨)
*��Դ: http://blog.csdn.net/dancing_night/article/details/53080819
*
****************************************************************************/
void _Mix(char sourseFile[10][SIZE_AUDIO_FRAME],int number,char *objectFile);
/****************************************************************
*newlc�ϵ�һ������PCM����������Ƶ�źŵĻ���ʵ��
*
*��Դ: http://blog.csdn.net/dancing_night/article/details/53080819
****************************************************************/
short _Mix2(short data1,short data2);

int Mix(char * pcm1,int len1,char *pcm2,int len2,char *outBuff, int outBuffLen);
int Mix2(char * pcm1,int len1,char *pcm2,int len2,char *outBuff, int outBuffLen);

#ifdef __cplusplus
}
#endif