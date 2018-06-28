#ifndef __CRC_8__
#define __CRC_8__


/****************************************
  У���λ��:  8λ
  ���ɶ���ʽ:  X^8+x^5+x^4+1  
  ����:        0x31
  ������ʼֵ:  0x00
  ������ֵ:  0x00

****************************************/
class CRC8
{
private:

	unsigned char initReg; //������ʼֵ
	unsigned char rsltXOR; //������ֵ
	unsigned char divisor; //����

	static unsigned char crc_0x31_table[256];


public:
	CRC8()
	{
		initReg = 0x00;
		rsltXOR = 0x00;
		divisor = 0x31;//���ɶ���ʽ:  X^8+x^5+x^4+1 
	}
	virtual ~CRC8(){}
	unsigned char calc(unsigned char *pData, unsigned int len);

};

#endif //__CRC_8__