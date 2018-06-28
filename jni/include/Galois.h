#ifndef __GALOIS_H__
#define __GALOIS_H__





//Ù¤ÂÞ»ªÓò¼Ó/¼õ²Ù×÷
#define glByteAdd(v1,v2) (v1^v2)

//Ù¤ÂÞ»ªÓò³Ë
unsigned char glByteMul(unsigned char v1,unsigned char v2);

//Ù¤ÂÞ»ªÓò³ý v1/v2
unsigned char glByteDiv(unsigned char v1,unsigned char v2);

extern unsigned char alphaTo[256];
extern unsigned char expOf[256];


#endif//__GALOIS_H__
