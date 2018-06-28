#include <stdlib.h>
#include <stdio.h>
#include "typedef.h"
#include "basic_op.h"
#include "ld8a.h"
#include "g729a.h"

static Word32 encodersize;
static void* hEncoder;
static Word32 decodersize;
static void *hDecoder;

/*Default bit streem size*/
#define	BITSTREAM_SIZE	10
/*Fixed rtp header size*/
#define	RTP_HDR_SIZE	12

#define BLOCK_LEN       160

static int codec_open = 0;


int codecs_G729_open()
{
    
    Flag en=0;
    Flag de=0;

    if(codec_open)
    {
        return 0;
    }
    /*--------------------------------------------------------------------------*
     * Initialization of the coder.                                             *
     *--------------------------------------------------------------------------*/

    encodersize = g729a_enc_mem_size();
    hEncoder = malloc(encodersize*sizeof(UWord8));

    if(hEncoder == NULL)
    {
        //ALOGD("%s", "Cannot create encoder");
        //printf("Cannot create encoder\n");
        return -1;
    }

    en=g729a_enc_init(hEncoder);
    if(en == 0)
    {	
        //ALOGD("%s", "Cannot create encoder");
        //printf("Cannot create encoder\n");
        free(hEncoder);
        return -1;
     }
/*-----------------------------------------------------------------*
 *           Initialization of decoder                             *
 *-----------------------------------------------------------------*/
        decodersize = g729a_dec_mem_size();
        hDecoder = malloc( decodersize * sizeof(UWord8));
        if (hDecoder == NULL)
        {	
            //ALOGD("%s", "Cannot create decoder");
            //printf("Cannot create decoder\n");
            free(hEncoder);
            return -1;
        }
        
        de=g729a_dec_init(hDecoder);
        if (de == 0)
        {	 
            //ALOGD("%s", "Cannot create decoder");
            //printf("Cannot create decoder\n");
            free(hEncoder);
            free(hDecoder);
            return -1;
        }
        codec_open = 1;
        
        return 0;

	     
}

int codecs_G729_encode(short *pcm, unsigned char *G729Data, int lengthOfPcm)
{
	int i = 0;
    if (!codec_open){ return -1;}

    for (i = 0; i *L_FRAME< lengthOfPcm; i++)
     {
        g729a_enc_process(hEncoder, pcm+i*L_FRAME, (unsigned char*)G729Data+i*BITSTREAM_SIZE);
    }

    return i*BITSTREAM_SIZE;
}

int codecs_G729_decode(unsigned char *G729Data,  short *pcm, int lengthofG729)
{

		int i =0;
        if (!codec_open){return -1;}
        

        for (i=0; i*BITSTREAM_SIZE<lengthofG729 ; i++)
        {
            g729a_dec_process(hDecoder, (unsigned char*)G729Data+i*BITSTREAM_SIZE, pcm+i*L_FRAME, 0);
        }
        
        return i*L_FRAME;
}

int codecs_G729_close()
{
    	if (!codec_open)
		return -1;

	codec_open = 0;

	/*encoder closed*/
	g729a_enc_deinit(hEncoder);
	free(hEncoder);
	/*decoder closed*/
	g729a_dec_deinit(hDecoder);
	free(hDecoder);

	return 0;
}


