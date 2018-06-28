#ifndef __LAMECONTROL_DEFINE__
#define __LAMECONTROL_DEFINE__

typedef signed char        INT8;
typedef unsigned char      UINT8;
typedef signed short       INT16;
typedef unsigned short     UINT16;
typedef signed int         INT32;
typedef unsigned int       UINT32;
typedef unsigned int       UINT;

#undef net_sourceforge_lame_Lame_MP3_BUFFER_SIZE
#define net_sourceforge_lame_Lame_MP3_BUFFER_SIZE 1024L
#undef net_sourceforge_lame_Lame_LAME_PRESET_DEFAULT
#define net_sourceforge_lame_Lame_LAME_PRESET_DEFAULT 0L
#undef net_sourceforge_lame_Lame_LAME_PRESET_MEDIUM
#define net_sourceforge_lame_Lame_LAME_PRESET_MEDIUM 1L
#undef net_sourceforge_lame_Lame_LAME_PRESET_STANDARD
#define net_sourceforge_lame_Lame_LAME_PRESET_STANDARD 2L
#undef net_sourceforge_lame_Lame_LAME_PRESET_EXTREME
#define net_sourceforge_lame_Lame_LAME_PRESET_EXTREME 3L

//空指针定义
#define NULLPTR      0

//---规格定义----
#define LAME_INSTANCE_MAX  0xFF

#endif  //__LAMECONTROL_DEFINE__