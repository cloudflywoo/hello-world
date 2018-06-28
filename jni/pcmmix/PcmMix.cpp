#include "pcmmix.h"




/****************************************************************************
*归一化混音(自适应加权混音算法)
*来源: http://blog.csdn.net/dancing_night/article/details/53080819
*
****************************************************************************/
void _Mix(char sourseFile[10][SIZE_AUDIO_FRAME],int number,char *objectFile)  
{  
    //归一化混音  
    int const MAX=32767;  
    int const MIN=-32768;  

    double f=1;  
    int output;  
    int i = 0,j = 0;  
    for (i=0;i<SIZE_AUDIO_FRAME/2;i++)  
    {  
        int temp=0;  
        for (j=0;j<number;j++)  
        {  
            temp+=*(short*)(sourseFile[j]+i*2);  
        }                  
        output=(int)(temp*f);  
        if (output>MAX)  
        {  
            f=(double)MAX/(double)(output);  
            output=MAX;  
        }  
        if (output<MIN)  
        {  
            f=(double)MIN/(double)(output);  
            output=MIN;  
        }  
        if (f<1)  
        {  
            f+=((double)1-f)/(double)32;  
        }  
        *(short*)(objectFile+i*2)=(short)output;  
    }  
}  

/****************************************************************
*newlc上的一个关于PCM脉冲编码的音频信号的混音实现
*
*来源: http://blog.csdn.net/dancing_night/article/details/53080819
****************************************************************/
short _Mix2(short data1,short data2)
{
	int const MAX = 32767;//pow(2,16-1)-1)
	short data_mix;
    if( data1 < 0 && data2 < 0)  
        data_mix = data1+data2 - (data1 * data2 / -MAX);  
    else  
        data_mix = data1+data2 - (data1 * data2 / MAX); 

	return data_mix;
}

int Mix(char * pcm1,int len1,char *pcm2,int len2,char *outBuff, int outBuffLen)
{
	char sourseFile[10][2]; 
	short data1,data2,data_mix;  

	int maxInputDataLen = len1>=len2?len1:len2;

	int maxOutPutDataLen = maxInputDataLen<=outBuffLen?maxInputDataLen:outBuffLen;

	int i;
	for(i=0;i<maxOutPutDataLen;i+=2)
	{
		if(i<len1 && i<len2){
		    data1 = *(short*)(pcm1+i);
		    data2 = *(short*)(pcm2+i);
		    *(short*) sourseFile[0] = data1;  
            *(short*) sourseFile[1] = data2;  
			_Mix(sourseFile,2,(char *)&data_mix);  
		}else if(i<len1){
			data_mix = *(short*)(pcm1+i);
		}else{
			data_mix = *(short*)(pcm2+i);
		}

		*(short*)(outBuff+i) = data_mix;

		
	}

	return maxOutPutDataLen;


}

int Mix2(char * pcm1,int len1,char *pcm2,int len2,char *outBuff, int outBuffLen)
{

	short data1,data2,data_mix;  

	int maxInputDataLen = len1>=len2?len1:len2;

	int maxOutPutDataLen = maxInputDataLen<=outBuffLen?maxInputDataLen:outBuffLen;

	int i;
	for(i=0;i<maxOutPutDataLen;i+=2)
	{
		if(i<len1 && i<len2){
		    data1 = *(short*)(pcm1+i);
		    data2 = *(short*)(pcm2+i); 
			data_mix = _Mix2(data1,data2);  
		}else if(i<len1){
			data_mix = *(short*)(pcm1+i);
		}else{
			data_mix = *(short*)(pcm2+i);
		}

		*(short*)(outBuff+i) = data_mix;

		
	}

	return maxOutPutDataLen;


}

