#ifndef __CRC_8__
#define __CRC_8__


/****************************************
  校验和位宽:  8位
  生成多项式:  X^8+x^5+x^4+1  
  除数:        0x31
  余数初始值:  0x00
  结果异或值:  0x00

****************************************/
class CRC8
{
private:

	unsigned char initReg; //余数初始值
	unsigned char rsltXOR; //结果异或值
	unsigned char divisor; //除数

	static unsigned char crc_0x31_table[256];


public:
	CRC8()
	{
		initReg = 0x00;
		rsltXOR = 0x00;
		divisor = 0x31;//生成多项式:  X^8+x^5+x^4+1 
	}
	virtual ~CRC8(){}
	unsigned char calc(unsigned char *pData, unsigned int len);

};

#endif //__CRC_8__