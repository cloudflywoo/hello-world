#ifndef  _MTXW_AMRCODEC_H_
#define  _MTXW_AMRCODEC_H_

enum Mode {
	MR475 = 0,/* 4.75 kbps */
	MR515,    /* 5.15 kbps */
	MR59,     /* 5.90 kbps */
	MR67,     /* 6.70 kbps */
	MR74,     /* 7.40 kbps */
	MR795,    /* 7.95 kbps */
	MR102,    /* 10.2 kbps */
	MR122,    /* 12.2 kbps */
	MRDTX,    /* DTX       */
	N_MODES   /* Not Used  */
};

class Amr_Codec
{
public :
    void *amr_dec;
    void *amr_enc;

public:
    Amr_Codec();

    ~Amr_Codec();

    void Decode( const unsigned char* amrData, short* pcm, int bfi);

    int Encode( enum Mode mode, const short* pcm, unsigned char* amrData, int forceSpeech);
    
};

#endif


