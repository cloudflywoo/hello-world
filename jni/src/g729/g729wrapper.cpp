
#include "g729wrapper.h"
#include "wrapper.h"

int G729_Codec::Cnt = 0;

G729_Codec::G729_Codec()
{
	  if(Cnt==0)
	  {
		  ::codecs_G729_open(); 
	  }
	  Cnt++;
}
G729_Codec::~G729_Codec()
{
	if(Cnt<=0){return;}
	  
	Cnt--;
	if(Cnt==0)
	{
		::codecs_G729_close() ;
	}
}

/*******************************************************
pcm:   [in] short array, which contains pcm data of each frame(20 ms)
lengthOfPcm:  [in] length of array pcm, (normally is 160)
G729Data: [out]

return: length of G729Data (normally is 20 bytes)
*******************************************************/
int G729_Codec::encode(short *pcm, unsigned char *G729Data, int lengthOfPcm)
{
	return (::codecs_G729_encode(pcm, G729Data, lengthOfPcm));
}

/*******************************************************
G729Data:   [in] short array, which contains g729 data of each frame
lengthofG729: [in] length of array G729Data, (normally is 20)
pcm: [out] short array, contains the decoded  pcm data

return: length of array PCM  (normally is 160)
*******************************************************/
int G729_Codec::decode(unsigned char *G729Data,  short *pcm, int lengthofG729)
{
	return (::codecs_G729_decode(G729Data,  pcm, lengthofG729));
}

