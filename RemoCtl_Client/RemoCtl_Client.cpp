// RemoCtl_Client.cpp : ����Ӧ�ó������ڵ㡣
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

// ȫ�ֱ���: 
HINSTANCE hInst;								// ��ǰʵ��
TCHAR szTitle[MAX_LOADSTRING];					// �������ı�
TCHAR szWindowClass[MAX_LOADSTRING];			// ����������

HWND hwnd;
SOCKET sClient;
struct sockaddr_in server;
int port = 5050;

struct _Remote_MSG
{
	HWND hWnd; UINT message; WPARAM wParam; LPARAM lParam;
};
typedef struct _Remote_MSG Remote_MSG;

// �˴���ģ���а����ĺ�����ǰ������: 
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

 	// TODO:  �ڴ˷��ô��롣
	MSG msg;
	HACCEL hAccelTable;

	// ��ʼ��ȫ���ַ���
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_REMOCTL_CLIENT, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// ִ��Ӧ�ó����ʼ��: 
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_REMOCTL_CLIENT));

	// ����Ϣѭ��: 
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
//  ����:  MyRegisterClass()
//
//  Ŀ��:  ע�ᴰ���ࡣ
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
	wcex.lpszMenuName = NULL;// MAKEINTRESOURCE(IDC_REMOCTL_CLIENT);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   ����:  InitInstance(HINSTANCE, int)
//
//   Ŀ��:  ����ʵ�����������������
//
//   ע��: 
//
//        �ڴ˺����У�������ȫ�ֱ����б���ʵ�������
//        ��������ʾ�����򴰿ڡ�
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // ��ʵ������洢��ȫ�ֱ�����

//   hwnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
//     CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
   hwnd = CreateWindow(szWindowClass, szTitle, WS_POPUP, 0, 0, 
	   GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
	   NULL, NULL, hInstance, NULL);
   if (!hwnd)
   {
      return FALSE;
   }

   ShowWindow(hwnd, nCmdShow);
   UpdateWindow(hwnd);

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

typedef struct _MSG_SCREEN
{
	DWORD dwBmpSize;
}MSG_SCREEN;

BOOL jpg2bmp(BYTE *jpg, size_t len, BYTE **bmp,  HWND hWnd)
{
	HGLOBAL mem_jpg = GlobalAlloc(GPTR, len);
	memcpy(mem_jpg, jpg, len);

	IStream *stm_jpg = NULL;
	CreateStreamOnHGlobal(mem_jpg, FALSE, &stm_jpg);
	Image *im_jpg = Image::FromStream(stm_jpg, FALSE);

	CLSID clsid_bmp;
	GetEncoderClsid(L"image/bmp", &clsid_bmp);//ȡ��BMP����
	
	HGLOBAL mem_bmp = GlobalAlloc(GMEM_MOVEABLE, 0);
	IStream *stm_bmp = NULL;
	CreateStreamOnHGlobal(mem_bmp, FALSE, &stm_bmp);
	im_jpg->Save(stm_bmp, &clsid_bmp);

	int Len = GlobalSize(mem_bmp);//ת����BMP�ļ���С
	BYTE *pbyBmp = (BYTE *)GlobalLock(mem_bmp);
	*bmp = new BYTE[Len];
	memcpy(*bmp, pbyBmp, Len);
	GlobalUnlock(mem_bmp);

	stm_jpg->Release();
	stm_bmp->Release();
	GlobalFree(mem_jpg);
	GlobalFree(mem_bmp);
	return true;
}

DWORD WINAPI Refresh_Screen(LPVOID lpParam)
{
	BYTE *recv_buf = new BYTE[10240000];
	int recv_count;

	while (true)
	{
		recv_count = recv(sClient, (char*)recv_buf, 10240000, 0);
		if (recv_count <= 0)
			continue;
		MSG_SCREEN *msg_screen = (MSG_SCREEN*)recv_buf;
		size_t len = ntohl(msg_screen->dwBmpSize);

		BYTE *bmp;
		jpg2bmp(recv_buf + sizeof(MSG_SCREEN), len, &bmp, hwnd);

		BITMAPINFOHEADER *bmpheader = (BITMAPINFOHEADER*)(bmp + sizeof(BITMAPFILEHEADER));

		HDC hdc = GetDC(hwnd);
		StretchDIBits(hdc,
			0, 0, bmpheader->biWidth, bmpheader->biHeight,
			0, 0, bmpheader->biWidth, bmpheader->biHeight,
			bmp + sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER),
			(BITMAPINFO*)bmpheader,
			DIB_RGB_COLORS,
			SRCCOPY);
		ReleaseDC(hwnd, hdc);
		delete[] bmp;
	}
	delete [] recv_buf;
	return 0;
}

//
//  ����:  WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  Ŀ��:    ���������ڵ���Ϣ��
//
//  WM_COMMAND	- ����Ӧ�ó���˵�
//  WM_PAINT	- ����������
//  WM_DESTROY	- �����˳���Ϣ������
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int			wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC			hdc;
	Remote_MSG	send_msg;
	GdiplusStartupInput gdiplusStartupInput;
	static ULONG_PTR	pGdiToken;
	static DWORD		dwThreadId;

	switch (message)
	{
	case WM_CREATE:
		GdiplusStartup(&pGdiToken, &gdiplusStartupInput, NULL);//��ʼ��GDI+
		LoadWinsock(hWnd, "7.82.237.31");
	//	LoadWinsock(hWnd, "127.0.0.1");
		CreateThread(NULL, 0, Refresh_Screen, NULL, 0, &dwThreadId);
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// �����˵�ѡ��: 
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
		// TODO:  �ڴ���������ͼ����...
		EndPaint(hWnd, &ps);
		break;
	case WM_MOUSEMOVE:
	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MOUSEWHEEL:
		send_msg.hWnd = hWnd;
		send_msg.lParam = lParam;
		send_msg.wParam = wParam;
		send_msg.message = message;
		send(sClient, (char*)&send_msg, sizeof(send_msg), 0);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		GdiplusShutdown(pGdiToken);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// �����ڡ������Ϣ�������
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


// װ��WinsockȻ�����ӷ�����
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