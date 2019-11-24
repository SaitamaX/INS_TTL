// insDetect.cpp : �������̨Ӧ�ó������ڵ㡣
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
int MODE_ = -1, iter_size = 0;//ѡ��ģʽ��ͨ����ͬģʽ������������

HANDLE InitTTL(char *szStr) {
	WCHAR wszClassName[5];
	memset(wszClassName, 0, sizeof(wszClassName));
	MultiByteToWideChar(CP_ACP, 0, szStr, strlen(szStr) + 1, wszClassName,
		sizeof(wszClassName) / sizeof(wszClassName[0]));
	HANDLE hCom1 = CreateFile(wszClassName,//COM3��
		GENERIC_READ , //�����
		0, //��ռ��ʽ
		NULL,
		OPEN_EXISTING, //�򿪶����Ǵ���
		0, //ͬ����ʽ
		NULL);

	if (hCom1 == INVALID_HANDLE_VALUE)
	{
		printf("��COMʧ��!\n");
		return INVALID_HANDLE_VALUE;
	}
	else
	{
		printf("COM�򿪳ɹ���\n");
	}

	SetupComm(hCom1, 1024, 1024); //���뻺����������������Ĵ�С����1024
	COMMTIMEOUTS TimeOuts;
	//�趨����ʱ
	TimeOuts.ReadIntervalTimeout = 100;
	TimeOuts.ReadTotalTimeoutMultiplier = 5000;
	TimeOuts.ReadTotalTimeoutConstant = 5000;
	//�趨д��ʱ
	TimeOuts.WriteTotalTimeoutMultiplier = 500;
	TimeOuts.WriteTotalTimeoutConstant = 2000;
	SetCommTimeouts(hCom1, &TimeOuts); //���ó�ʱ
	DCB dcb;
	GetCommState(hCom1, &dcb);
	dcb.BaudRate = 115200; //������
	dcb.ByteSize = 8; //ÿ���ֽ���8λ
	dcb.Parity = NOPARITY; //����żУ��λ
	dcb.StopBits = ONESTOPBIT; //1��ֹͣλ
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
			//printf("\rmyAngel = x�᣺%4.2f��\t y��%4.2f��\tz��a��%4.2f\tx��a��%4.2f\ty��a��%4.2f\t", myAngle[0], myAngle[1], a_avg[2] - 0.02, a_avg[0] - 0.05, a_avg[1]);
			//printf("myAngel = x�᣺%4.2f��\t y��%4.2f��\n", myAngle[0], myAngle[1]);
			//printf("\rz��ԭʼa��%4.2f z�����a��%4.2f", a_avg[2], a[2]);
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
	printf("0:��̬����\n1:��̬����\n");
	printf("����������ѡ��ģʽ:");
	while (true) {
		try {
			cin >> MODE_;
			if (MODE_ != 0 && MODE_ != 1) {
				printf("\n��������,����������\n");
			}
			else
				break;
		}
		catch (exception e) {
			printf("\n������������������\n");
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
