#pragma once

struct Stru_KeyCode
{
	int keyCode;
	unsigned char hid;
	const char *keyName;		
};

const char *HidToText(unsigned char hid);
unsigned char TextToHid(const char *str);
unsigned char KeyToHid(int keyCode);