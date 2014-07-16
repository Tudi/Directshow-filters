#ifndef _WMSOURCE_READER_DEFINES_H_
#define _WMSOURCE_READER_DEFINES_H_

//Curses. These types should come from including "media.h" and "descriptions.h". But they have ohter includes conflicting...
#ifndef TIME_Unknown
	typedef __int64 MTime;
	#define TIME_Unknown ((MTime)-1)
#endif

enum SYNC_METHOD_NAMES
{
	OPEN_METHOD = 0,
	START_METHOD,
	STOP_METHOD,
	CLOSE_METHOD,
	SYNC_METHOD_MAX
};

enum E_INPUT_SUBTYPE
{
	NON_SPECIFIED,
	RGB24,
	IYUV,
	YV12
};

typedef enum _FILTER_STATE_INTERNAL
{
	INState_Stop = 0,
	INState_Pause_From_Stop = 1,
	INState_Run				= 2, 
	INState_Pause_From_Run	= 3, 
}FILTER_STATE_INTERNAL;

#ifndef SAFE_RELEASE
	#define SAFE_RELEASE(x) if(x != NULL){x->Release();x = NULL;}
#endif

#ifndef SAFE_DELETE_ARRAY
	#define SAFE_DELETE_ARRAY(p) { if(p) { delete [] (p); (p)=NULL; } }
#endif

#define ISM_SOURCE_OPENED								0x00000001
#define ISM_SOURCE_CLOSED								0x00000002
#define ISM_SOURCE_STARTED								0x00000003
#define ISM_SOURCE_START_BUFFERING						0x00000004
#define ISM_SOURCE_STOP_BUFFERING						0x00000005
#define ISM_SOURCE_STOPPED								0x00000006
#define ISM_SOURCE_SWITCH								0x00000007
#define ISM_SOURCE_TIMESTAMP_RESET						0x00000008
#define ISM_SOURCE_FRAMERATE_RESET						0x00000009
#define ISM_SOURCE_ERROR								0x0000000a
#define ISM_SOURCE_EOF									0x0000000b

#define SOURCE_MEDIATYPE_UNKNOWN	0x00
#define SOURCE_MEDIATYPE_VIDEO		0x01
#define SOURCE_MEDIATYPE_AUDIO		0x02
#define SOURCE_MEDIATYPE_BOTH		0x03

#endif