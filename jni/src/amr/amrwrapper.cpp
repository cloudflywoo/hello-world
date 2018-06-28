
#include "amrwrapper.h"
#include "interf_dec.h"
#include "interf_enc.h"


Amr_Codec::Amr_Codec()
{
	amr_dec = ::Decoder_Interface_init();
	amr_enc = ::Encoder_Interface_init(0);
}
Amr_Codec::~Amr_Codec()
{
	::Decoder_Interface_exit(amr_dec);
	::Encoder_Interface_exit(amr_enc);
    
    }

void Amr_Codec::Decode( const unsigned char* amrData, short* pcm, int bfi)
{
	::Decoder_Interface_Decode(amr_dec, amrData, pcm, bfi);
}
int Amr_Codec::Encode( enum Mode mode, const short* pcm, unsigned char* amrData, int forceSpeech)
{
	return (::Encoder_Interface_Encode(amr_enc, mode, pcm, amrData, forceSpeech));
}



