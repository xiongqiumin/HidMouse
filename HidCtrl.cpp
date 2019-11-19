#include "HidCtrl.h"

#pragma comment(lib,"./hid/hid.lib") 
#pragma comment(lib,"./hid/setupapi.lib")

const int SHORT_TIME_CARRY = 1200;

//HdiMouseDefine
HdiMouseDefine::HdiMouseDefine()
{
	Clear();
}

void HdiMouseDefine::Clear()
{	
    repeat = 1;
    mRecord = false;
    mAutoDelay = false;
	oldTick = 0;

    keyList.clear();
}

void HdiMouseDefine::StartRecord(bool auto_delay)
{
	Clear();		
    mAutoDelay = auto_delay;
	mRecord = true;	
}

bool HdiMouseDefine::AddKey(int hid,int type)
{
	assert(mRecord);
	set<int> need_key = needUpKey();
	if(type == 1)
	{			
		if(need_key.find(hid) == need_key.end())	
			return false;	
	}
	else
	{
		if(need_key.find(hid) != need_key.end())	
			return false;	
	}

	int curTime = GetTickCount();			
	HdiMouseDefine::keyComp comp;			
	comp.keyHid = hid;	
	comp.type = type;	

	if(keyList.size() == 0)
		oldTick = GetTickCount();	
	else
	{
        if(mAutoDelay)
		    keyList.back().time = curTime - oldTick;				
		oldTick = curTime;	
	}
	keyList.push_back(comp);					
	return true;
}

void HdiMouseDefine::StopRecord()
{
    int curTime = GetTickCount();
    if(mAutoDelay)
        keyList.back().time = curTime - oldTick;
	oldTick = curTime;

    //����̧���
	set<int> need_key = needUpKey();
	set<int>::iterator it = need_key.begin();
	while(it != need_key.end())
	{
		HdiMouseDefine::keyComp comp;			
		comp.keyHid = *it;	
		comp.type = 1;	
		comp.time = 0;
		keyList.push_back(comp);
		
		it++;
	}
	mRecord = false;
	oldTick = 0;
}

//�ж��з��գ�����ӳٺܳ��Ļ��ᳬ��
bool HdiMouseDefine::CheckSpace()
{
    BYTE buffer[KEY_DEFINE_LENGTH];
    int need_size = ToBuffer(buffer);

    set<int> need_key = needUpKey();
    return (need_size + need_key.size() * 3 + 8 < 128);
}

bool HdiMouseDefine::IsAutoDelay()
{
    return mAutoDelay;
}

set<int> HdiMouseDefine::needUpKey()
{
	set<int> need_key;
	for(unsigned int i = 0; i < keyList.size(); i++)
	{
		if(keyList[i].type == 0)
			need_key.insert(keyList[i].keyHid);
		else
			need_key.erase(keyList[i].keyHid);
	}

	return need_key;
}

int HdiMouseDefine::ToBuffer(BYTE *buffer) const
{	
    memset(buffer,0,KEY_DEFINE_LENGTH);

	int idx = 0;
	buffer[idx++] = 0x0;
	buffer[idx++] = repeat;
	for(unsigned int i = 0; i < keyList.size(); i++)
	{
		int long_time  = (keyList[i].time > SHORT_TIME_CARRY)? (keyList[i].time - SHORT_TIME_CARRY)/100:0;
		int short_time = (keyList[i].time - long_time * 100)/10;
		short_time = short_time > 0? short_time:1;
		long_time = long_time <= 0xFF? long_time:0xFF;

		if((idx > (KEY_DEFINE_LENGTH-2) && long_time == 0) || (idx > (KEY_DEFINE_LENGTH-4) && long_time != 0))
			return 0;		
		
		buffer[idx] = short_time;
		if(keyList[i].type == 1)
			buffer[idx] |= 0x80;		

		idx++;
		buffer[idx++] = keyList[i].keyHid;		
		if(long_time > 0)
		{
			buffer[idx++] = 0x00;
			buffer[idx++] = long_time;
		}				
	}

	return idx;
}

bool HdiMouseDefine::FromBuffer(const BYTE *buffer)
{	
	BYTE zero[16] = {0};
	keyList.clear();

	repeat = buffer[1];
	int idx = 2;
	while(idx < KEY_DEFINE_LENGTH)
	{
		keyComp comp;
		if(memcmp(buffer + idx,zero,2) == 0)
			break;
		
		int short_time = buffer[idx] & 0x7F;
		int long_time = 0;
		int key_type = ((buffer[idx] & 0x80) != 0);
		idx++;

		int key_hid  = buffer[idx++];
		if(idx < (KEY_DEFINE_LENGTH-2) && buffer[idx+1] == 0)
		{
			long_time = buffer[idx+2];
			idx+=2;
		}

		comp.type = key_type;
		comp.keyHid = key_hid;
		comp.time = long_time * 100 + short_time * 10;

		keyList.push_back(comp);
	}

	return true;
}

//CHidCtrl
CHidCtrl::CHidCtrl(void)
{
	mHandleIn  = INVALID_HANDLE_VALUE;
	mHandleOut = INVALID_HANDLE_VALUE;
}

CHidCtrl::~CHidCtrl(void)
{
	DeInit();
}
/*
#include "dbt.h"
void RegisterForDeviceNotifications()
{

	// Request to receive messages when a device is attached or removed.
	// Also see WM_DEVICECHANGE in BEGIN_MESSAGE_MAP(CPCRProjectDlg, CDialog).

	DEV_BROADCAST_DEVICEINTERFACE DevBroadcastDeviceInterface;
	HDEVNOTIFY DeviceNotificationHandle;

	DevBroadcastDeviceInterface.dbcc_size = sizeof(DevBroadcastDeviceInterface);
	DevBroadcastDeviceInterface.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	DevBroadcastDeviceInterface.dbcc_classguid = HidGuid;

	DeviceNotificationHandle =
		RegisterDeviceNotification(m_hWnd, &DevBroadcastDeviceInterface, DEVICE_NOTIFY_WINDOW_HANDLE);

}
*/

bool CHidCtrl::GetDevice(bool isCheck)
{    
	GUID HidGuid;  
	//��ȡ��ϵͳ��HID���GUID��ʶ
	HidD_GetHidGuid(&HidGuid);  
	HDEVINFO hDevInfo;
	HANDLE hHidHandle;  
    int device_num = 0;

	mError = "";
	// ׼�����ҷ���HID�淶��USB�豸
	hDevInfo=SetupDiGetClassDevs  
		(&HidGuid,  
		NULL,  
		NULL,  
		DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);

	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		mError = "����HID�淶��USB�豸��������";
		SetupDiDestroyDeviceInfoList(hDevInfo);
		return false;
	}

	DWORD MemberIndex = 0;
	BOOL bSuccess = false;
	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
	DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);

	do
	{
		bSuccess = SetupDiEnumDeviceInterfaces
			(hDevInfo, //�Ѱ�װ��HID�豸����Ϣ���ľ��
			NULL,
			&HidGuid, //HID���豸��GUID
			MemberIndex, //��ʼ�����ţ��������Զ�����
			&DeviceInterfaceData); //���Ա��浥���豸����Ϣ

		if ((!bSuccess) && (GetLastError() == ERROR_NO_MORE_ITEMS))
		{		
			mError = "δ�ҵ��豸";
			hHidHandle = NULL;			
			break;
		}

		//���ҵ���һ��USB�豸,���ȡ���豸��ϸ����Ϣ
		PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceInterfaceDetailData;
		DWORD Length = 0;

		SetupDiGetDeviceInterfaceDetail(hDevInfo,
			&DeviceInterfaceData,
			NULL,
			0,
			&Length,
			NULL);

		pDeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(Length);
		pDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA); //MUST BE!!!

		if (!SetupDiGetDeviceInterfaceDetail(hDevInfo,
			&DeviceInterfaceData,
			pDeviceInterfaceDetailData,
			Length,
			NULL,
			NULL))
		{
			mError = "����·���豸ʱ����!";				  			
		}
		else
		{
			//���Բ�ѯ�ķ�ʽ���豸���
			hHidHandle = CreateFile(pDeviceInterfaceDetailData->DevicePath ,
			0, //0Ϊ��ѯ��ʽ��GENERIC_READ | GENERIC_WRITEΪ��д
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);
		
			if (hHidHandle == INVALID_HANDLE_VALUE)
			{
				mError = "�޷����豸";									
			}
			else
			{
				HIDD_ATTRIBUTES Attributes;
				HidD_GetAttributes(hHidHandle,&Attributes);

				//���йظ��豸�ı�ʶ��ʾ����
				//VID��1d57 PID��1302
				HIDP_CAPS hidPCaps;
				int VendorID = 0x1d57;
				int ProductID = 0x1302;
				if(Attributes.VendorID == VendorID && Attributes.ProductID == ProductID)
				{ 
                    if(!isCheck)
                    {
					    //��ȡ�豸��Ȩ����ϢCapabilities.
					    PHIDP_PREPARSED_DATA pHidpPreparsedData;
					    /*****************************************************
					    API����: HidD_GetPreparsedData
					    ����: һ��ָ�򻺴���ָ�룬�û������������豸��Ȩ����Ϣ
					    ����: ��CreateFile���صľ��.
					    ����Ҫֱ�ӷ��ʻ���������HidP_GetCaps������API������Ҫһ��.
					    ********************************************************/

					    if (!HidD_GetPreparsedData(hHidHandle,&pHidpPreparsedData))
					    {
						    mError = "��ȡ HID PREPARED DATA ʧ��!";															
					    }
					    NTSTATUS status = HidP_GetCaps(pHidpPreparsedData,&hidPCaps);
					    if (status == HIDP_STATUS_SUCCESS)
					    {
						    //��UsagePage��Usage ID�жϣ������Ҫ�ȶ�����
						    int UsagePage = 0xff00;
						    int UsageID = 0x1;
						    if( hidPCaps.UsagePage== UsagePage && hidPCaps.Usage == UsageID)
						    {
							    //�ҵ��豸�����Զ�д�ķ�ʽ���Խ��в���						
							    HANDLE *pHandle;
							    DWORD dwDesiredAccess,dwShareMode;
							    if(hidPCaps.InputReportByteLength > 0)
							    {
								    pHandle = &mHandleIn;
								    dwDesiredAccess = GENERIC_READ;
								    dwShareMode = FILE_SHARE_READ;
							    }
							    else
							    {
								    pHandle = &mHandleOut;
								    dwDesiredAccess = GENERIC_WRITE | GENERIC_READ;
								    dwShareMode = FILE_SHARE_WRITE | FILE_SHARE_READ;
							    }

							    if(*pHandle == INVALID_HANDLE_VALUE)
							    {
								    *pHandle = CreateFile(pDeviceInterfaceDetailData->DevicePath ,
									    dwDesiredAccess, //0Ϊ��ѯ��ʽ��GENERIC_READ | GENERIC_WRITEΪ��д
									    dwShareMode,
									    NULL,
									    OPEN_EXISTING,
									    0,
									    NULL);

                                    device_num++;
							    }						
							    /*
							    �˴����в���
							    */												
						    } 
					    }
                    }
                    else
                        device_num++;
				}
				//�ͷž����Դ
				CloseHandle(hHidHandle);			
			}
		}
		free(pDeviceInterfaceDetailData);
        if(!mError.empty())
			break;
		if(device_num == 2)
			break;

		MemberIndex++;
	} while(bSuccess);
	SetupDiDestroyDeviceInfoList(hDevInfo);

	return mError.empty();
}

bool CHidCtrl::Init()
{
    return GetDevice(false);
}

void CHidCtrl::DeInit()
{
	if(mHandleIn != INVALID_HANDLE_VALUE)
	{
		CloseHandle(mHandleIn);
		mHandleIn = INVALID_HANDLE_VALUE;
	}

	if(mHandleOut != INVALID_HANDLE_VALUE)
	{
		CloseHandle(mHandleOut);	
		mHandleOut = INVALID_HANDLE_VALUE;
	}
}

bool CHidCtrl::CheckDevice()
{
    if(mHandleIn != INVALID_HANDLE_VALUE)
    {
        bool ret = GetDevice(true);
        if(!ret)
            DeInit();

        return ret;
    }
    else
        return GetDevice(false);
}

string CHidCtrl::GetError()
{
	return mError;
}

bool CHidCtrl::ReadInData(BYTE *buffer,int time)
{
	BYTE readBuffer[33] = {0};
	DWORD bytes;

	for(int i = 0; i < time; i++)
	{
		if(!ReadFile(mHandleIn, readBuffer, 33, &bytes, NULL))
			return false; 
		memcpy(buffer + i * 32,readBuffer+1,32);
	}	

	return true;
}

bool CHidCtrl::SetPara(const HdiMousePara &para)
{
	BYTE featureBuf[9] = {0x00,0x0e,0x01,0x01,0x40,0x00,0x00,0x00,0x00};		
	BYTE paramBuf[65] = {0};
	DWORD bytes;

	if(!HidD_SetFeature(mHandleOut,featureBuf,9))
		return false; 

	memcpy(paramBuf + 1,para.value,64);
	if(!WriteFile(mHandleOut,paramBuf,65,&bytes,NULL))
		return false;

    Sleep(40);
	return true;
}

bool CHidCtrl::GetPara(int &sensor_type,int &mouse_id,HdiMousePara &para)
{
	BYTE setReport[9] = {0x00,0x8e,0x03,0x01,0x00,0x00,0x00,0x00,0x00};		   
	BYTE getFeature[9] = {0};	

	if(!HidD_SetFeature(mHandleOut,setReport,9))
		return false; 

	if(!HidD_GetFeature(mHandleOut,getFeature,9))	
		return false; 
	
	if(getFeature[5] == 0x30 && getFeature[6] == 0x50)
		sensor_type = SENSOR_ADNS3050;
	else if(getFeature[5] == 0x50 && getFeature[6] == 0x50)
		sensor_type = SENSOR_ADNS5050;
	else if(getFeature[5] == 0x30 && getFeature[6] == 0x00)
		sensor_type = SENSOR_ADNS3000;
	else
		sensor_type = SENSOR_INVAILD;

    mouse_id = getFeature[7];	
	if(!ReadInData(para.value,2))
		return false; 
	
	return true;
}

bool CHidCtrl::SetMap(const HdiMouseMap &para)
{
	BYTE featureBuf[9] = {0x00,0x0C,0x01,0x00,0x40,0x00,0x00,0x00,0x00};	
	BYTE paramBuf[65] = {0};
	DWORD bytes;

	if(!HidD_SetFeature(mHandleOut,featureBuf,9))
		return false; 

	memcpy(paramBuf + 1,para.value,64);
	if(!WriteFile(mHandleOut,paramBuf,65,&bytes,NULL))
		return false;

    Sleep(40);
	return true;
}

bool CHidCtrl::GetMap(HdiMouseMap &para)
{
	BYTE featureBuf[9] = {0x00,0x8C,0x01,0x00,0x00,0x00,0x00,0x00,0x00};		   
	BYTE getFeature[9] = {0};	

	if(!HidD_SetFeature(mHandleOut,featureBuf,9))
		return false; 

	if(!HidD_GetFeature(mHandleOut,getFeature,9))	
		return false; 
	
	if(!ReadInData(para.value,2))
		return false; 

	return true;
}

bool CHidCtrl::SetDefine(int seq,const HdiMouseDefine &para)
{
	assert(seq >= 1 && seq <= 0xC);

	BYTE featureBuf[9] = {0x00,0x0d,0x01,0x01,0x40,0x00,0x00,0x00,0x00};	
	BYTE paramBuf[KEY_DEFINE_LENGTH] = {0};
	BYTE wirteBuf[65] = {0};
	DWORD bytes;

	featureBuf[1 + 2] = seq;
	if(!HidD_SetFeature(mHandleOut,featureBuf,9))
		return false; 

	para.ToBuffer(paramBuf);

	memcpy(wirteBuf + 1,paramBuf,64);
	if(!WriteFile(mHandleOut,wirteBuf,65,&bytes,NULL))
		return false;
	
	Sleep(40);

	memcpy(wirteBuf + 1,paramBuf + 64,64);
	if(!WriteFile(mHandleOut,wirteBuf,65,&bytes,NULL))
		return false;	

    Sleep(40);
	return true;
}

bool CHidCtrl::GetDefine(int seq,HdiMouseDefine &para)
{
	assert(seq >= 0 && seq <= 0xC);

	BYTE setReport[9] = {0x00,0x8d,0x01,0x01,0x00,0x00,0x00,0x00,0x00};		   
	BYTE getFeature[9] = {0};	
	BYTE paraBuffer[KEY_DEFINE_LENGTH] = {0};	

	setReport[1 + 2] = seq;
	if(!HidD_SetFeature(mHandleOut,setReport,9))
		return false; 

	if(!HidD_GetFeature(mHandleOut,getFeature,9))	
		return false; 
	
	if(!ReadInData(paraBuffer,KEY_DEFINE_LENGTH/32))
		return false; 

	para.FromBuffer(paraBuffer);
	return true;
}

bool CHidCtrl::SetFinish()
{
	BYTE featureBuf[9] = {0x00,0x08,0x00,0x02,0x00,0x00,0x00,0x00,0x00};		
	if(!HidD_SetFeature(mHandleOut,featureBuf,9))
		return false; 

    Sleep(40);
	return true;
}

bool CHidCtrl::SetLedReportGap(int gap)
{
	assert(gap >= 0 && gap < 4);

	BYTE featureBuf[9] = {0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00};	
	featureBuf[2] = 1 << gap;
	if(!HidD_SetFeature(mHandleOut,featureBuf,9))
		return false; 

    Sleep(40);
	return true;
}

bool CHidCtrl::SetLedOnOff(bool on)
{
	BYTE featureBuf[9] = {0x00,0x02,0x01,0x00,0x00,0x00,0x00,0x00,0x00};	
	featureBuf[2] = on? 0x01 : 0x0;
	if(!HidD_SetFeature(mHandleOut,featureBuf,9))
		return false; 

    Sleep(40);
	return true;
}