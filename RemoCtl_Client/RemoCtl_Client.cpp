// RemoCtl_Client.cpp : 定义应用程序的入口点。
//
#include "stdafx.h"
#include "RemoCtl_Client.h"
#include <WinSock2.h>
#include <stdio.h>
#include <GdiPlus.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"gdiPlus.lib")

using namespace Gdiplus;
#define MAX_LOADSTRING 100

// 全局变量: 
HINSTANCE hInst;								// 当前实例
TCHAR szTitle[MAX_LOADSTRING];					// 标题栏文本
TCHAR szWindowClass[MAX_LOADSTRING];			// 主窗口类名


SOCKET sClient;
struct sockaddr_in server;
int port = 5050;

struct _Remote_MSG
{
	HWND hWnd; UINT message; WPARAM wParam; LPARAM lParam;
};
typedef struct _Remote_MSG Remote_MSG;

// 此代码模块中包含的函数的前向声明: 
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
int LoadWinsock(HWND hWnd, char *szIP);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO:  在此放置代码。
	MSG msg;
	HACCEL hAccelTable;

	// 初始化全局字符串
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_REMOCTL_CLIENT, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 执行应用程序初始化: 
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_REMOCTL_CLIENT));

	// 主消息循环: 
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  函数:  MyRegisterClass()
//
//  目的:  注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_REMOCTL_CLIENT));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_REMOCTL_CLIENT);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   函数:  InitInstance(HINSTANCE, int)
//
//   目的:  保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // 将实例句柄存储在全局变量中

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
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


typedef struct _MSG_SCREEN
{
	DWORD dwBmpSize;
}MSG_SCREEN;

void Refresh_Screen(HWND hWnd)
{
	TCHAR	mess[128];
	char *recv_buf = (char*)malloc(10240000);
	
	int count = recv(sClient, recv_buf, 10240000, 0);

	MSG_SCREEN *msg_screen = (MSG_SCREEN*)recv_buf;
	int len = ntohl(msg_screen->dwBmpSize);

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR pGdiToken;
	GdiplusStartup(&pGdiToken, &gdiplusStartupInput, NULL);//初始化GDI+
	

	HGLOBAL hMemJpg = GlobalAlloc(GPTR, len);
	BYTE *pbyjpg = (BYTE *)GlobalLock(hMemJpg);
	memcpy(pbyjpg, recv_buf + sizeof(MSG_SCREEN), len);
	GlobalUnlock(hMemJpg);

	IStream *pStmJpg = NULL;
	CreateStreamOnHGlobal(hMemJpg, FALSE, &pStmJpg);

	Image *imImage = NULL; //从流中还原图片
	imImage = Image::FromStream(pStmJpg, FALSE);

	CLSID clImageClsid;
	GetEncoderClsid(L"image/bmp", &clImageClsid);//取得BMP编码


	HGLOBAL hMemBmp = GlobalAlloc(GMEM_MOVEABLE, 0);
	IStream *pStmBmp = NULL;
	CreateStreamOnHGlobal(hMemBmp, FALSE, &pStmBmp);
	imImage->Save(pStmBmp, &clImageClsid);

	int Len = GlobalSize(hMemBmp);//转换后BMP文件大小
	BYTE *pbyBmp = (BYTE *)GlobalLock(hMemBmp);//此数据就可直接调用API画出来了
	
	char *pBuf = new char[Len];
	memcpy(pBuf, pbyBmp, Len);

	GlobalUnlock(hMemBmp);

	pStmBmp->Release();
	GlobalFree(hMemBmp);

	GdiplusShutdown(pGdiToken);

	BITMAPINFOHEADER *bmpheader = (BITMAPINFOHEADER*)(pBuf + sizeof(BITMAPFILEHEADER));

	HDC hdc = GetDC(hWnd);
	StretchDIBits(hdc,
		0, 0, bmpheader->biWidth, bmpheader->biHeight,
		0, 0, bmpheader->biWidth, bmpheader->biHeight,
		pBuf + sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER),
		(BITMAPINFO*)bmpheader,
		DIB_RGB_COLORS,
		SRCCOPY);
	ReleaseDC(hWnd, hdc);

	free(recv_buf);
}

//
//  函数:  WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    处理主窗口的消息。
//
//  WM_COMMAND	- 处理应用程序菜单
//  WM_PAINT	- 绘制主窗口
//  WM_DESTROY	- 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	int x, y; 
	Remote_MSG	send_msg;

	switch (message)
	{
	case WM_CREATE:
	//	LoadWinsock(hWnd, "7.82.237.31");
		LoadWinsock(hWnd, "127.0.0.1");
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// 分析菜单选择: 
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO:  在此添加任意绘图代码...
		EndPaint(hWnd, &ps);
		break;
//	case WM_MOUSEMOVE:
//	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
//	case WM_LBUTTONUP:
//	case WM_RBUTTONDOWN:
//	case WM_RBUTTONUP:
		x = LOWORD(lParam);
		y = HIWORD(lParam);
		send_msg.hWnd = hWnd;
		send_msg.lParam = lParam;
		send_msg.wParam = wParam;
		send_msg.message = message;
		send(sClient, (char*)&send_msg, sizeof(send_msg), 0);
		Refresh_Screen(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


// 装入Winsock然后连接服务器
int LoadWinsock(HWND hWnd, char *szIP)
{
	WSADATA				wsd;
	int					nZero;
	TCHAR				szString[81];

	if (WSAStartup(0x202, &wsd) != 0)
	{
		MessageBox(NULL, L"hehe", L"Client Socket Error", MB_OK);
		return 1;
	}
	
	sClient = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, SOCK_STREAM);
	if (sClient == INVALID_SOCKET)
		return 1;

	nZero = 0;
	setsockopt(sClient, SOL_SOCKET, SO_RCVBUF, (char *)&nZero, sizeof(nZero));

	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = inet_addr(szIP);

	if (connect(sClient, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
	{
		memset(szString, '\0', sizeof(szString));
		wsprintf(szString, L"Connect() failed: %d", WSAGetLastError());
		MessageBox(NULL, szString, L"Client Socket Error", MB_OK);
		return 1;
	}
	return 0;
}