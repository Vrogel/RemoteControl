#include "stdafx.h"
#include <WinSock2.h>
#include <stdio.h>
#include <GdiPlus.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"gdiPlus.lib")

extern HINSTANCE hInst;		// 当前实例
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

//LoadWinsock用来装载和初始化Winsock，绑定本地地址，创建监听socket，等候客户端连接
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
	// 创建我们的监听socket
	Listen = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, SOCK_STREAM);
	if (Listen == SOCKET_ERROR)
	{
		wsprintf(szString, L"socket() failed: %d", WSAGetLastError());
		MessageBox(NULL, szString, L"Remote Server", MB_OK);
		return 1;
	}

	// 设置server端信息
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	local.sin_family = AF_INET;
	local.sin_port = htons(gPort);

	// 绑定到socket
	if (bind(Listen, (struct sockaddr *)&local, sizeof(local)) == SOCKET_ERROR)
	{
		wsprintf(szString, L"bind() failed: %d\n", WSAGetLastError());
		MessageBox(NULL, szString, L"Remote Server", MB_OK);
		return 1;
	}

	//为了减小CPU的利用率，禁止在socket上将数据发送到缓冲。设置SO_SNDBUF为0,
	//从而使winsock直接发送数据到客户端，而不是将数据缓冲才发送。
	nZero = 0;
	setsockopt(Listen, SOL_SOCKET, SO_SNDBUF, (char *)&nZero, sizeof(nZero));
	//开始监听
	listen(Listen, SOMAXCONN);

	iAddrSize = sizeof(client);
	while (TRUE)
	{
		// 阻塞方式的接收客户端的连接，但因为这是一个线程函数，所以用户不会感到阻塞
		Socket = accept(Listen, (struct sockaddr *)&client, &iAddrSize);
		if (Socket != INVALID_SOCKET)
		{
			//找出客户端的IP地址
			memset(szClientIP, '\0', sizeof(szClientIP));
			wsprintf(szClientIP, L"%s", inet_ntoa(client.sin_addr));
			// 为每一个客户端创建一个线程
			hThread = CreateThread(NULL, 0, ClientThread, &Socket, 0, &dwThreadId);
			if (hThread)
			{
				//关闭线程句柄
				CloseHandle(hThread);
			}
		}
		else
			return 1;
	}

	return 0;
}


//客户端线程函数，这个函数等候从客户端程序发送过来的消息，
//如果这个消息是"REFRESH",那么它发送当前的桌面图片
//如果这个消息是"DISCONNECT",那么它结束和客户端的连接
//如果这个消息以"WM_"开头，那么它就根据消息类型，在服务器端执行该消息
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
	// 设置超时值
	timeout.tv_sec = 0;		// 秒
	timeout.tv_usec = 0;	// 微秒

	// 设置Socket集合
	SocketSet.fd_count = 1;
	SocketSet.fd_array[1] = MySocket;

	// 轮询sockets
	while (TRUE)
	{
		// 等候发送过来的数据直到超时
		iRet = select(0, &SocketSet, NULL, NULL, &timeout);
		if (iRet != 0)
		{
			//初始化缓冲
			memset(szMessage, '\0', sizeof(szMessage));
			// 阻塞方式调用recv()
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

	// 通过内存DC复制屏幕到DDB位图
	HDC hdcWnd = GetDC(NULL);
	HBITMAP hbmWnd = CreateCompatibleBitmap(hdcWnd, width, height);
	HDC hdcMem = CreateCompatibleDC(hdcWnd);
	SelectObject(hdcMem, hbmWnd);
	BitBlt(hdcMem, 0, 0, width, height, hdcWnd, 0, 0, SRCCOPY);

	BITMAP bmp;
	GetObject(hdcMem, sizeof(BITMAP), &bmp);
	// 把窗口DDB转为DIB
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = width;
	bi.biHeight = height;
	bi.biPlanes = 1;
	bi.biBitCount = 32; // 按照每个像素用32bits表示转换
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
	float costTime;            //用时,*1000000 = 微秒, 1秒=1000000

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

	void Start()            // 开始计时
	{
		QueryPerformanceCounter(&_begin);
	}

	void End()                // 结束计时
	{
		QueryPerformanceCounter(&_end);
		costTime = ((_end.QuadPart - _begin.QuadPart)*1.0f / _freq);
	}

	void Reset()            // 计时清0
	{
		costTime = 0;
	}
};


DWORD WINAPI send_screen(LPVOID lpParam)
{	
	SOCKET socket = *(SOCKET *)lpParam;
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR pGdiToken;
	GdiplusStartup(&pGdiToken, &gdiplusStartupInput, NULL);//初始化GDI+

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

		char *send_buf = (char*)malloc(sizeof(MSG_SCREEN)+len); // 像素位指针
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

// 将当前屏幕保存成为jpg图片       
// 参数xs图象x轴大小 ys图象y轴大小 quality图象质量       
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
		BYTE *pbyjpg = (BYTE *)GlobalLock(hMemJpg);//这里的pbyjpg就是转换后的JPG数据可直接通过网络发送

		*len = GlobalSize(hMemJpg);//这就是转为JPG后文件的大小了

		*pBuf = new char[*len];
		memcpy(*pBuf, pbyjpg, *len);//可以保存此数据,任你操作哦

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
