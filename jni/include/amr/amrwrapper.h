#ifndef  _AMRWRAPPER_H_
#define  _AMRWRAPPER_H_

#include "interf_enc.h"

class Amr_Codec
{
private :
    void *amr_dec;
    void *amr_enc;
	/*

    void* Decoder_Interface_init(void);
    void Decoder_Interface_Decode(void* state, const unsigned char* g729Data, short* pcm, int bfi);
    void Decoder_Interface_exit(void* state);
    void* Encoder_Interface_init(int dtx);
    int Encoder_Interface_Encode(void* s, enum Mode mode, const short* speech, unsigned char* out, int forceSpeech);
    void Encoder_Interface_exit(void* s);
	*/
    
public:
    Amr_Codec();
    ~Amr_Codec();

    void Decode( const unsigned char* g729Data, short* pcm, int bfi);
    int Encode( enum Mode mode, const short* pcm, unsigned char* g729Data, int forceSpeech);
    
};

#endif


