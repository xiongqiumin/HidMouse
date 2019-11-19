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
	MODE_KEY = 0x0,     //�������
	MODE_MOUSE = 0x1,   //��갴��
	MODE_MEDIA = 0x3,   //��ý���
    MODE_DPI = 0x7,     //DPI ������
	MODE_DEFINE = 0x9,  //�궨��
	MODE_HIT_KEY = 0xa,  //������������
	MODE_LED_ONOFF = 0xc,//�ƹ⿪��
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
			BYTE Color[4]; //�ֽ�Ϊ�趨�����ɫR��G��B��W(ȡֵΪ0)
			BYTE LedMode;  //00 ����ģʽ 01 ����ģʽ 02 ����ģʽ 03 APM ģʽ
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
		struct              //ӳ���
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

