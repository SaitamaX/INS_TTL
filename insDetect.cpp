// insDetect.cpp : 定义控制台应用程序的入口点。
//
#include "stdafx.h"
#include <iostream>
#include <Windows.h>
#include <string>
#include <thread>  
#include <math.h>
#include <vector>
#include <time.h>
#define PI 3.1415926
#define DATASIZE 55
using namespace std;
double a[3], w[3], Angle[3], myAngle[2], T;
int i = 0;
vector<double> a_avg = { 0,0,0 };
double start = 0, end_ = 0;
int MODE_ = -1, iter_size = 0;//选择模式并通过不同模式决定迭代次数

HANDLE InitTTL(char *szStr) {
	WCHAR wszClassName[5];
	memset(wszClassName, 0, sizeof(wszClassName));
	MultiByteToWideChar(CP_ACP, 0, szStr, strlen(szStr) + 1, wszClassName,
		sizeof(wszClassName) / sizeof(wszClassName[0]));
	HANDLE hCom1 = CreateFile(wszClassName,//COM3口
		GENERIC_READ , //允许读
		0, //独占方式
		NULL,
		OPEN_EXISTING, //打开而不是创建
		0, //同步方式
		NULL);

	if (hCom1 == INVALID_HANDLE_VALUE)
	{
		printf("打开COM失败!\n");
		return INVALID_HANDLE_VALUE;
	}
	else
	{
		printf("COM打开成功！\n");
	}

	SetupComm(hCom1, 1024, 1024); //输入缓冲区和输出缓冲区的大小都是1024
	COMMTIMEOUTS TimeOuts;
	//设定读超时
	TimeOuts.ReadIntervalTimeout = 100;
	TimeOuts.ReadTotalTimeoutMultiplier = 5000;
	TimeOuts.ReadTotalTimeoutConstant = 5000;
	//设定写超时
	TimeOuts.WriteTotalTimeoutMultiplier = 500;
	TimeOuts.WriteTotalTimeoutConstant = 2000;
	SetCommTimeouts(hCom1, &TimeOuts); //设置超时
	DCB dcb;
	GetCommState(hCom1, &dcb);
	dcb.BaudRate = 115200; //波特率
	dcb.ByteSize = 8; //每个字节有8位
	dcb.Parity = NOPARITY; //无奇偶校验位
	dcb.StopBits = ONESTOPBIT; //1个停止位
	SetCommState(hCom1, &dcb);
	return hCom1;
}

bool checkData(unsigned char chrTemp[]) {
	unsigned char sum = 0x00;
	for(int i = 0; i < 5; i++)
	{
		for(int j = 0; j < 10; j++)
		{
			sum += chrTemp[i * 11 + j];
		}
		if (sum != chrTemp[i * 11 + 10])
			return false;
		sum = 0x00;
	}
	return true;
}

void DecodeIMUData(unsigned char chrTemp[])
{
	switch (chrTemp[1])
	{
	case 0x51:
		a[0] = (short(chrTemp[3] << 8 | chrTemp[2])) / 32768.0 * 16;
		a[1] = (short(chrTemp[5] << 8 | chrTemp[4])) / 32768.0 * 16;
		a[2] = (short(chrTemp[7] << 8 | chrTemp[6])) / 32768.0 * 16;
		T = (short(chrTemp[9] << 8 | chrTemp[8])) / 340.0 + 36.25;
		if (i == iter_size) {
			for (int j = 0; j < 3; j++)
				a_avg[j] /= iter_size;
			myAngle[0] = atanf(a_avg[1] / (a_avg[2])) / PI * 180;
			myAngle[1] = -atanf((a_avg[0]) / (a_avg[2])) / PI * 180; 
			end_ = clock();
			//printf("\rmyAngel = x轴：%4.2f°\t y轴%4.2f°\tz轴a：%4.2f\tx轴a：%4.2f\ty轴a：%4.2f\t", myAngle[0], myAngle[1], a_avg[2] - 0.02, a_avg[0] - 0.05, a_avg[1]);
			//printf("myAngel = x轴：%4.2f°\t y轴%4.2f°\n", myAngle[0], myAngle[1]);
			//printf("\rz轴原始a：%4.2f z轴计算a：%4.2f", a_avg[2], a[2]);
			cout << "time" << end_ - start << "ms" << endl;
			for (int j = 0; j < 3; j++)
				a_avg[j] = 0;
			i = 0;
		}
		else {
			if (i == 0)
				start = clock();
			for (int j = 0; j < 3; j++)
				a_avg[j] += a[j];
			i++;
		}
		break;
	case 0x52:
		w[0] = (short(chrTemp[3] << 8 | chrTemp[2])) / 32768.0 * 2000;
		w[1] = (short(chrTemp[5] << 8 | chrTemp[4])) / 32768.0 * 2000;
		w[2] = (short(chrTemp[7] << 8 | chrTemp[6])) / 32768.0 * 2000;
		T = (short(chrTemp[9] << 8 | chrTemp[8])) / 340.0 + 36.25;
		break;
	case 0x53:
		Angle[0] = (short(chrTemp[3] << 8 | chrTemp[2])) / 32768.0 * 180;
		Angle[1] = (short(chrTemp[5] << 8 | chrTemp[4])) / 32768.0 * 180;
		Angle[2] = (short(chrTemp[7] << 8 | chrTemp[6])) / 32768.0 * 180;
		T = (short(chrTemp[9] << 8 | chrTemp[8])) / 340.0 + 36.25;
		break;
	}
}

//float InvSqrt(float x) {
//	float xhalf = 0.5f*x;
//	int i = *(int*)&x;        // get bits for floating VALUE
//	i = 0x5f375a86 - (i >> 1); // gives initial guess y0
//	x = *(float*)&i;         // convert bits BACK to float
//	x = x*(1.5f - xhalf*x*x); // Newton step, repeating increases accuracy
//	return x;
//}

int main() {
	HANDLE hCom1 = InitTTL("COM4");
	unsigned char buffer[DATASIZE];
	DWORD readsize;
	unsigned char *p = buffer - 1;
	int off = 1;
	if (hCom1 == INVALID_HANDLE_VALUE) {
		system("pause");
		return -1;
	}
	printf("0:静态测量\n1:动态测量\n");
	printf("请输入数字选择模式:");
	while (true) {
		try {
			cin >> MODE_;
			if (MODE_ != 0 && MODE_ != 1) {
				printf("\n输入有误,请重新输入\n");
			}
			else
				break;
		}
		catch (exception e) {
			printf("\n输入有误，请重新输入\n");
			continue;
		}
	}
	if (MODE_ == 0)
		iter_size = 20;
	else
		iter_size = 5;
	while (true) {
		//start = clock();
		try {
			ReadFile(hCom1, buffer, DATASIZE, &readsize, NULL);
			p = find(buffer, buffer + DATASIZE, 0x55);
			if (p == buffer && *(p + 1) == 0x51) 
			{
				
				if (checkData(buffer)) 
				{
					/*for (int i = 0; i < DATASIZE; i++)
					{
						printf("%02X ", buffer[i]);
					}
					printf("\n\n");*/
					for (int i = 0; i < 5; i++) 
					{
						DecodeIMUData(buffer + i * 11);
					}
				}
			}
			else
			{
				while (p < buffer + DATASIZE - 1 && *(p + 1) != 0x51) 
				{
					p = find(p + 1, buffer + DATASIZE, 0x55);
				}
				int off = p - buffer;
				memmove(buffer, p, DATASIZE - off);
				ReadFile(hCom1, buffer + DATASIZE - off, off, &readsize, NULL);
				if (checkData(buffer)) 
				{
					/*for (int i = 0; i < DATASIZE; i++) 
					{
						printf("%02X ", buffer[i]);
					}
					printf("\n\n");*/
					for (int i = 0; i < 5; i++) 
					{
						DecodeIMUData(buffer + i * 11);
					}
				}
			}
		}
		catch (exception e) {
			continue;
		}
	}
	/*unsigned char t = 0x55, m = 0x51, sum = 0x00;
	sum = t + m;
	printf("%02X ", sum);
	if (sum == 0xa6)
		printf("ok");*/
	system("pause");
	return 0;
}
