#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <set>
#include <Setupapi.h>
#include <assert.h>

extern "C"
{
#include "hidsdi.h"
#include "hidpi.h"
}

using namespace std;

const int KEY_DEFINE_LENGTH = 128;

enum{
	SENSOR_INVAILD = -1,
	SENSOR_ADNS3050 = 0,
	SENSOR_ADNS5050,
	SENSOR_ADNS3000,
};

enum{
	MODE_KEY = 0x0,     //键盘组合
	MODE_MOUSE = 0x1,   //鼠标按键
	MODE_MEDIA = 0x3,   //多媒体键
    MODE_DPI = 0x7,     //DPI 键功能
	MODE_DEFINE = 0x9,  //宏定义
	MODE_HIT_KEY = 0xa,  //连击键键码连
	MODE_LED_ONOFF = 0xc,//灯光开关
    MODE_DISABLE = 0xf,
};

#pragma pack(push)
#pragma pack(1)

struct HdiMousePara
{
	HdiMousePara()
	{
		memset(value,0,sizeof(value));			
	}

	union
	{
		BYTE value[64];
		struct
		{
			BYTE CPI[8];
			BYTE CPIColor[8][3];
			BYTE Color[4]; //字节为设定鼠标颜色R、G、B、W(取值为0)
			BYTE LedMode;  //00 常亮模式 01 呼吸模式 02 光谱模式 03 APM 模式
			BYTE LedSpeed; //1~32
		};
	};	
};

struct HdiMouseMap
{	
	HdiMouseMap()
	{
		memset(value,0,sizeof(value));			
	}

	union
	{
		BYTE value[64];
		struct              //映射表
		{
			BYTE keyMap[16][4];
		};
	};	
};

struct HdiMouseDefine
{
	struct keyComp
	{
		keyComp()
		{
			type = 0;
			keyHid = 0;
			time = 0;
		}

		int type;    //0 down,1 up
		int keyHid;  //hid		
		int time;
	};

	HdiMouseDefine();
	void Clear();
	int ToBuffer(BYTE *buffer) const;
	bool FromBuffer(const BYTE *buffer);	

	void StartRecord(bool delay);
	bool AddKey(int code,int type);	
	void StopRecord();
    bool CheckSpace();

    bool IsAutoDelay();    

	int repeat;
	vector<keyComp> keyList;

private:
	set<int> needUpKey();
    
	bool mRecord;    
    bool mAutoDelay;
	int oldTick;    
};

#pragma pack(pop)

class CHidCtrl
{
public:
	CHidCtrl(void);
	~CHidCtrl(void);

    bool GetDevice(bool isCheck);
    bool CheckDevice();

	bool Init();    
	void DeInit();	        

	bool SetPara(const HdiMousePara &para);
	bool GetPara(int &sensor_type,int &mouse_id,HdiMousePara &para);
	bool SetMap(const HdiMouseMap &para);
	bool GetMap(HdiMouseMap &para);
	bool SetDefine(int seq,const HdiMouseDefine &para);
	bool GetDefine(int seq,HdiMouseDefine &para);

	bool SetFinish();
	bool SetLedReportGap(int gap);
	bool SetLedOnOff(bool on);

	string GetError();
private:	
	bool ReadInData(BYTE *buffer,int time);

	HANDLE mHandleIn;
	HANDLE mHandleOut;
	string mError;
};

