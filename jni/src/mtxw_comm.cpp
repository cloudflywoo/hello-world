
#include <mtxw_comm.h>
#include <stdlib.h>

//--引入gMemCnt，用于简单的内存泄露检测。
static UINT32 gMemCnt = 0;

//----内存操作函数----------------//
void *mtxwMemAlloc(UINT32 usize)
{
	void *p=0;
	p = malloc(usize);
	if(p)
	{
		gMemCnt++;
	}
	return p;
}
void mtxwMemFree(void* p)
{
	if(p==0){return;}
	
	free(p);
	gMemCnt--;
}
INT32 mtxwGetMemCnt()
{
    return gMemCnt;
}

UINT16 mtxwHtons(UINT16 hostShort)
{
      UINT8 magic[]={0x11,0x22};
      UINT16 *pusVal = (UINT16*)&magic[0];

      if(*pusVal==0x1122)
      {
           //本机为大端字节序
           return hostShort;
      }
      else
      {
          //本机为小段字节序
          return (hostShort<<8) | ((hostShort&0xFF00)>>8);
      }
     	  
}
UINT16 mtxwNtohs(UINT16 netShort)
{
       return mtxwHtons(netShort);
}

UINT32 mtxwHtonl(UINT32 hostLong)
{
	UINT8 magic[] = {0x11, 0x22, 0x33, 0x44};
	UINT32 *pusVal = (UINT32*) &magic[0];

	if(*pusVal == 0x11223344)
	{
		return hostLong;
	}
	else
	{
		return (hostLong<<24)|((hostLong<<8)&0x00FF0000)|((hostLong>>8)&0x0000FF00)|(hostLong>>24);
	}
	
}
UINT32 mtxwNtohl(UINT32 netLong)
{
	return mtxwHtonl(netLong);
}


std::string mtxwGetBuildTime()
{

		char buff[1024]={0};
		const char *pattern = ": ";
		std::vector<std::string> vecSubStr;
		
		sprintf(buff,"%s %s",__DATE__,__TIME__ );

		char *ptr = strtok(buff, pattern);
		while(ptr){    
            vecSubStr.push_back(ptr); 
            ptr = strtok(0, pattern);  
        }

		if(vecSubStr.size()!=6)
		{
			return std::string("unknowntime");
		}


		if(vecSubStr[0]=="Jan"){vecSubStr[0]="01";}
		else if(vecSubStr[0]=="Feb"){vecSubStr[0]="02";}
		else if(vecSubStr[0]=="Mar"){vecSubStr[0]="03";}
		else if(vecSubStr[0]=="Apr"){vecSubStr[0]="04";}
		else if(vecSubStr[0]=="May"){vecSubStr[0]="05";}
		else if(vecSubStr[0]=="Jun"){vecSubStr[0]="06";}
		else if(vecSubStr[0]=="Jul"){vecSubStr[0]="07";}
		else if(vecSubStr[0]=="Aug"){vecSubStr[0]="08";}
		else if(vecSubStr[0]=="Sep"){vecSubStr[0]="09";}
		else if(vecSubStr[0]=="Oct"){vecSubStr[0]="10";}
		else if(vecSubStr[0]=="Nov"){vecSubStr[0]="11";}
		else if(vecSubStr[0]=="Dec"){vecSubStr[0]="12";}
		
		std::string BUILDTIME = vecSubStr[2]+vecSubStr[0]+vecSubStr[1]+vecSubStr[3]+vecSubStr[4]+vecSubStr[5];
		
		return BUILDTIME;

}


