#include "stdafx.h"
#include <WinSock2.h>
#include <stdio.h>
#include <GdiPlus.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"gdiPlus.lib")

extern HINSTANCE hInst;		// ��ǰʵ��
extern HWND hwnd;
int gPort = 5050;
using namespace Gdiplus;
struct _Remote_MSG
{
	HWND hWnd; UINT message; WPARAM wParam; LPARAM lParam;
};
typedef struct _Remote_MSG Remote_MSG;

DWORD WINAPI ClientThread(LPVOID lpParam);
DWORD WINAPI send_screen(LPVOID lpParam);
void capscreen(BYTE *image_buffer);

//LoadWinsock����װ�غͳ�ʼ��Winsock���󶨱��ص�ַ����������socket���Ⱥ�ͻ�������
DWORD WINAPI LoadWinsock(LPVOID lpParam)
{
	int					nZero = 0;

	int					iAddrSize;
	HANDLE				hThread;
	DWORD				dwThreadId;
	TCHAR				szClientIP[81];
	TCHAR				szString[255];
	struct	sockaddr_in	local, client;

	WSADATA				wsd;
	SOCKET Listen, Socket;

	if (WSAStartup(0x202, &wsd) != 0)
	{
		MessageBox(NULL, L"hehe", L"Client Socket Error", MB_OK);
		return 1;
	}
	// �������ǵļ���socket
	Listen = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, SOCK_STREAM);
	if (Listen == SOCKET_ERROR)
	{
		wsprintf(szString, L"socket() failed: %d", WSAGetLastError());
		MessageBox(NULL, szString, L"Remote Server", MB_OK);
		return 1;
	}

	// ����server����Ϣ
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	local.sin_family = AF_INET;
	local.sin_port = htons(gPort);

	// �󶨵�socket
	if (bind(Listen, (struct sockaddr *)&local, sizeof(local)) == SOCKET_ERROR)
	{
		wsprintf(szString, L"bind() failed: %d\n", WSAGetLastError());
		MessageBox(NULL, szString, L"Remote Server", MB_OK);
		return 1;
	}

	//Ϊ�˼�СCPU�������ʣ���ֹ��socket�Ͻ����ݷ��͵����塣����SO_SNDBUFΪ0,
	//�Ӷ�ʹwinsockֱ�ӷ������ݵ��ͻ��ˣ������ǽ����ݻ���ŷ��͡�
	nZero = 0;
	setsockopt(Listen, SOL_SOCKET, SO_SNDBUF, (char *)&nZero, sizeof(nZero));
	//��ʼ����
	listen(Listen, SOMAXCONN);

	iAddrSize = sizeof(client);
	while (TRUE)
	{
		// ������ʽ�Ľ��տͻ��˵����ӣ�����Ϊ����һ���̺߳����������û�����е�����
		Socket = accept(Listen, (struct sockaddr *)&client, &iAddrSize);
		if (Socket != INVALID_SOCKET)
		{
			//�ҳ��ͻ��˵�IP��ַ
			memset(szClientIP, '\0', sizeof(szClientIP));
			wsprintf(szClientIP, L"%s", inet_ntoa(client.sin_addr));
			// Ϊÿһ���ͻ��˴���һ���߳�
			hThread = CreateThread(NULL, 0, ClientThread, &Socket, 0, &dwThreadId);
			if (hThread)
			{
				//�ر��߳̾��
				CloseHandle(hThread);
			}
		}
		else
			return 1;
	}

	return 0;
}


//�ͻ����̺߳�������������Ⱥ�ӿͻ��˳����͹�������Ϣ��
//��������Ϣ��"REFRESH",��ô�����͵�ǰ������ͼƬ
//��������Ϣ��"DISCONNECT",��ô�������Ϳͻ��˵�����
//��������Ϣ��"WM_"��ͷ����ô���͸�����Ϣ���ͣ��ڷ�������ִ�и���Ϣ
DWORD WINAPI ClientThread(LPVOID lpParam)
{
	SOCKET	MySocket = *(SOCKET *)lpParam;
	FD_SET	SocketSet;
	struct	timeval	timeout;
	char	szMessage[2049];
	int		iRecv;
	DWORD	iRet;
	Remote_MSG* msg;
	DWORD	dwThreadId;
	// TCHAR	mess[128];
	//	MSG		amsg;
	// HDC		hdc;

	CreateThread(NULL, 0, send_screen, &MySocket, 0, &dwThreadId);
	// ���ó�ʱֵ
	timeout.tv_sec = 0;		// ��
	timeout.tv_usec = 0;	// ΢��

	// ����Socket����
	SocketSet.fd_count = 1;
	SocketSet.fd_array[1] = MySocket;

	// ��ѯsockets
	while (TRUE)
	{
		// �Ⱥ��͹���������ֱ����ʱ
		iRet = select(0, &SocketSet, NULL, NULL, &timeout);
		if (iRet != 0)
		{
			//��ʼ������
			memset(szMessage, '\0', sizeof(szMessage));
			// ������ʽ����recv()
			iRecv = recv(MySocket, szMessage, 2048, 0);
			if (iRecv == -1)
				break;
			msg = (Remote_MSG*)szMessage;
			
			INPUT inputs;
		//	SetCursorPos(LOWORD(msg->lParam), HIWORD(msg->lParam));
			switch (msg->message)
			{
			case WM_KEYDOWN:
				inputs.type = INPUT_KEYBOARD;
				inputs.ki.wVk = msg->wParam;
				inputs.ki.wScan = 0;
				inputs.ki.time = 0;
				inputs.ki.dwExtraInfo = GetMessageExtraInfo();
				inputs.ki.dwFlags = 0;
				SendInput(1, &inputs, sizeof(INPUT));
				break;
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
				break;
			case WM_KEYUP:
				inputs.type = INPUT_KEYBOARD;
				inputs.ki.wVk = msg->wParam;
				inputs.ki.wScan = 0;
				inputs.ki.time = 0;
				inputs.ki.dwExtraInfo = GetMessageExtraInfo();
				inputs.ki.dwFlags = KEYEVENTF_KEYUP;
				SendInput(1, &inputs, sizeof(INPUT));
				break;
			case WM_MOUSEWHEEL:
				inputs.type = INPUT_MOUSE;
				inputs.mi.dx = inputs.mi.dy = 0;
				inputs.mi.mouseData = GET_WHEEL_DELTA_WPARAM(msg->wParam);;
				inputs.mi.time = 0;
				inputs.mi.dwExtraInfo = GetMessageExtraInfo();
				inputs.mi.dwFlags = MOUSEEVENTF_WHEEL;
				SendInput(1, &inputs, sizeof(INPUT));
				break;
			case WM_MOUSEMOVE:SetCursorPos(LOWORD(msg->lParam), HIWORD(msg->lParam));
			//	inputs.type = INPUT_MOUSE;
			//	inputs.mi.dx = LOWORD(msg->lParam);
			//	inputs.mi.dy = HIWORD(msg->lParam);
			//	inputs.mi.mouseData = inputs.mi.time = 0;
			//	inputs.mi.dwExtraInfo = GetMessageExtraInfo();
			//	inputs.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
			//	SendInput(1, &inputs, sizeof(INPUT));
			//	mouse_event(MOUSEEVENTF_MOVE, 
			//		LOWORD(msg->lParam), HIWORD(msg->lParam), 0, 0);
				break;
			case WM_LBUTTONDOWN:
				inputs.type = INPUT_MOUSE;
				inputs.mi.dx = inputs.mi.dy = 0;
				inputs.mi.mouseData = inputs.mi.time = 0;
				inputs.mi.dwExtraInfo = GetMessageExtraInfo();
				inputs.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
				SendInput(1, &inputs, sizeof(INPUT));
			//	mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
				break;
			case WM_LBUTTONUP:
				inputs.type = INPUT_MOUSE;
				inputs.mi.dx = inputs.mi.dy = 0;
				inputs.mi.mouseData = inputs.mi.time = 0;
				inputs.mi.dwExtraInfo = GetMessageExtraInfo();
				inputs.mi.dwFlags = MOUSEEVENTF_LEFTUP;
				SendInput(1, &inputs, sizeof(INPUT));
				break;
			case WM_LBUTTONDBLCLK:
				mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
				mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
				break;
			case WM_RBUTTONDOWN:
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
				break;
			case WM_RBUTTONUP:
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
				break;
			case WM_RBUTTONDBLCLK:
				mouse_event(MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
				mouse_event(MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
				break;
			default:
				break;
			}
		//	PostMessage(hwnd, msg->message, msg->wParam, msg->lParam);
		}
	}
	closesocket(MySocket);
	return 0;
}


void capscreen(BYTE *image_buffer, BITMAPINFOHEADER &bi)
{
	int width = GetSystemMetrics(SM_CXSCREEN);
	int height = GetSystemMetrics(SM_CYSCREEN);

	// ͨ���ڴ�DC������Ļ��DDBλͼ
	HDC hdcWnd = GetDC(NULL);
	HBITMAP hbmWnd = CreateCompatibleBitmap(hdcWnd, width, height);
	HDC hdcMem = CreateCompatibleDC(hdcWnd);
	SelectObject(hdcMem, hbmWnd);
	BitBlt(hdcMem, 0, 0, width, height, hdcWnd, 0, 0, SRCCOPY);

	BITMAP bmp;
	GetObject(hdcMem, sizeof(BITMAP), &bmp);
	// �Ѵ���DDBתΪDIB
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = width;
	bi.biHeight = height;
	bi.biPlanes = 1;
	bi.biBitCount = 32; // ����ÿ��������32bits��ʾת��
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = bi.biYPelsPerMeter = 0;
	bi.biClrUsed = bi.biClrImportant = 0;

	GetDIBits(hdcMem, hbmWnd, 0, (UINT)height, NULL, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

	GetDIBits(hdcMem, hbmWnd, 0, (UINT)height,
		image_buffer,
		(BITMAPINFO*)&bi,
		DIB_RGB_COLORS);

	DeleteDC(hdcMem);
	DeleteObject(hbmWnd);

	BITMAPFILEHEADER bfh = { 0 };
	bfh.bfOffBits = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
	bfh.bfSize = bfh.bfOffBits + bmp.bmWidthBytes * bmp.bmHeight;
	bfh.bfType = (WORD)0x4d42;

	FILE *fp;
	fopen_s(&fp, "lala.bmp", "w+b");
	fwrite(&bfh, 1, sizeof(BITMAPFILEHEADER), fp);
	fwrite(&bi, 1, sizeof(BITMAPINFOHEADER), fp);
	fwrite(image_buffer, 1, 1600 * 900 * 4, fp);
	fclose(fp);
}

typedef struct _MSG_SCREEN
{
	int dwBmpSize;
}MSG_SCREEN;

void SaveCurScreenJpg(int xs, int ys, int quality, char **pBuf, int *len);


class StopWatch
{
private:
	int _freq;
	LARGE_INTEGER _begin;
	LARGE_INTEGER _end;

public:
	float costTime;            //��ʱ,*1000000 = ΢��, 1��=1000000

	StopWatch(void)
	{
		LARGE_INTEGER tmp;
		QueryPerformanceFrequency(&tmp);
		_freq = tmp.QuadPart;
		costTime = 0;
	}

	~StopWatch(void)
	{

	}

	void Start()            // ��ʼ��ʱ
	{
		QueryPerformanceCounter(&_begin);
	}

	void End()                // ������ʱ
	{
		QueryPerformanceCounter(&_end);
		costTime = ((_end.QuadPart - _begin.QuadPart)*1.0f / _freq);
	}

	void Reset()            // ��ʱ��0
	{
		costTime = 0;
	}
};


DWORD WINAPI send_screen(LPVOID lpParam)
{	
	SOCKET socket = *(SOCKET *)lpParam;
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR pGdiToken;
	GdiplusStartup(&pGdiToken, &gdiplusStartupInput, NULL);//��ʼ��GDI+

	LARGE_INTEGER startCount;
	LARGE_INTEGER endCount;
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&startCount);

	char m[100];
	int t_begin, t_screen, t_send;
	FILE *fp = fopen("log.log", "a+");
	while (true)
	{
		QueryPerformanceCounter(&endCount);
		t_begin = (double)(endCount.QuadPart - startCount.QuadPart) / freq.QuadPart * 1000;

		char *pbuf;
		int len;
		SaveCurScreenJpg(1366, 768, 100, &pbuf, &len);

		QueryPerformanceCounter(&endCount);
		t_screen = (double)(endCount.QuadPart - startCount.QuadPart) / freq.QuadPart * 1000;

		char *send_buf = (char*)malloc(sizeof(MSG_SCREEN)+len); // ����λָ��
		MSG_SCREEN *msg_head = (MSG_SCREEN *)send_buf;
		msg_head->dwBmpSize = htonl(len);
		memcpy(send_buf + sizeof(MSG_SCREEN), pbuf, len);
		int count = send(socket, send_buf, sizeof(MSG_SCREEN)+len, 0);

		QueryPerformanceCounter(&endCount);
		t_send = (double)(endCount.QuadPart - startCount.QuadPart) / freq.QuadPart * 1000;

		sprintf(m, "Begin %d After SaveScreen %d Fin %d\t\t%d\n", 
			t_begin, t_screen, t_send, len);
		fwrite(m, strlen(m), 1, fp);
	}
	GdiplusShutdown(pGdiToken);
	fclose(fp);
	return 0;
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT num = 0;                     // number of image encoders   
	UINT size = 0;                   // size of the image encoder array in bytes   
	ImageCodecInfo *pImageCodecInfo = NULL;
	GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;     //   Failure   

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;     //   Failure   

	GetImageEncoders(num, size, pImageCodecInfo);
	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;     //   Success   
		}
	}
	free(pImageCodecInfo);
	return -1;     //   Failure   
}

// ����ǰ��Ļ�����ΪjpgͼƬ       
// ����xsͼ��x���С ysͼ��y���С qualityͼ������       
void SaveCurScreenJpg(int xs, int ys, int quality, char **pBuf, int *len)
{

	LARGE_INTEGER startCount;
	LARGE_INTEGER endCount;
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&startCount);

	char m[100];
	int t_begin, t_screen, t_copy, t_send;
	FILE *fp = fopen("jpg.log", "a+");


	HWND hwnd = ::GetDesktopWindow();
	HDC hdc = GetWindowDC(NULL);
	int x = GetDeviceCaps(hdc, HORZRES);
	int y = GetDeviceCaps(hdc, VERTRES);
	HBITMAP hbmp = ::CreateCompatibleBitmap(hdc, x, y), hold;
	HDC hmemdc = ::CreateCompatibleDC(hdc);
	hold = (HBITMAP)::SelectObject(hmemdc, hbmp);

	QueryPerformanceCounter(&endCount);
	t_begin = (double)(endCount.QuadPart - startCount.QuadPart) / freq.QuadPart * 1000;

	BitBlt(hmemdc, 0, 0, x, y, hdc, 0, 0, SRCCOPY);


	QueryPerformanceCounter(&endCount);
	t_screen = (double)(endCount.QuadPart - startCount.QuadPart) / freq.QuadPart * 1000;

	SelectObject(hmemdc, hold);
	
	Bitmap bit(hbmp, NULL);
//	Bitmap bit(xs, ys), bit2(hbmp, NULL);
//	Graphics g(&bit);
//	g.ScaleTransform((float)xs / x, (float)ys / y);
//	g.DrawImage(&bit2, 0, 0);

	QueryPerformanceCounter(&endCount);
	t_copy = (double)(endCount.QuadPart - startCount.QuadPart) / freq.QuadPart * 1000;

	CLSID               encoderClsid;
	EncoderParameters   encoderParameters;

	encoderParameters.Count = 1;
	encoderParameters.Parameter[0].Guid = EncoderQuality;
	encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
	encoderParameters.Parameter[0].NumberOfValues = 1;
	encoderParameters.Parameter[0].Value = &quality;

	GetEncoderClsid(L"image/jpeg", &encoderClsid);




	{
		HGLOBAL hMemJpg = GlobalAlloc(GMEM_MOVEABLE, 0);

		IStream *pStmImage = NULL;
		CreateStreamOnHGlobal(hMemJpg, FALSE, &pStmImage);
		bit.Save(pStmImage, &encoderClsid);

		LARGE_INTEGER liBegin = { 0 };
		pStmImage->Seek(liBegin, STREAM_SEEK_SET, NULL);
		BYTE *pbyjpg = (BYTE *)GlobalLock(hMemJpg);//�����pbyjpg����ת�����JPG���ݿ�ֱ��ͨ�����緢��

		*len = GlobalSize(hMemJpg);//�����תΪJPG���ļ��Ĵ�С��

		*pBuf = new char[*len];
		memcpy(*pBuf, pbyjpg, *len);//���Ա��������,�������Ŷ

		GlobalUnlock(hMemJpg);
		GlobalFree(hMemJpg);
	}

	QueryPerformanceCounter(&endCount);
	t_send = (double)(endCount.QuadPart - startCount.QuadPart) / freq.QuadPart * 1000;

	sprintf(m, "CreCompDC %d\t BitBlt %d\t DrawImage %d\t Save %d\t\t%d\n", 
		t_begin, t_screen, t_copy, t_send, *len);
	fwrite(m, strlen(m), 1, fp);
	fclose(fp);
	::DeleteObject(hbmp);
	::DeleteObject(hmemdc);
	return;
}
