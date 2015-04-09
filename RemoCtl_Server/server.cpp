#include "stdafx.h"
#include "libjpeg\JPG.h"
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"jpeg.lib")
#define JDIMENSION UINT
extern HINSTANCE hInst;		// 当前实例
extern HWND hwnd;
SOCKET Listen, Socket;
int gPort = 5050;

struct _Remote_MSG
{
	HWND hWnd; UINT message; WPARAM wParam; LPARAM lParam;
};
typedef struct _Remote_MSG Remote_MSG;

DWORD WINAPI ClientThread(LPVOID lpParam);
void GetScreen(HWND hWnd, SOCKET socket);
void decompress(BYTE raw[], int nSize, BYTE *&data);
BOOL compress2jpeg(BYTE raw[], JDIMENSION width, JDIMENSION height, BYTE **outdata, ULONG *nSize);

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
			hThread = CreateThread(NULL, 0, ClientThread, NULL, 0, &dwThreadId);
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
	SOCKET	MySocket = Socket;
	FD_SET	SocketSet;
	struct	timeval	timeout;
	char	szMessage[2049];
	DWORD	iRecv;
	DWORD	iRet;
	Remote_MSG* msg;
	TCHAR	mess[128];
//	MSG		amsg;
	HDC		hdc;
	
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
			szMessage[iRecv] = '\0';
			msg = (Remote_MSG*)szMessage;
			memset(mess, '\0', sizeof(mess));
			wsprintf(mess, L"x=%d y=%d %d", LOWORD(msg->lParam), HIWORD(msg->lParam), msg->message);
			hdc = GetDC(hwnd);
			TextOut(hdc, 0, 0, mess, 40);
			ReleaseDC(hwnd, hdc);
			SetCursorPos(LOWORD(msg->lParam), HIWORD(msg->lParam));
			GetScreen(hwnd, MySocket);
	//		MessageBox(hwnd, mess, L"MESSAGE", MB_OK);
			//amsg.hwnd = hwnd;
			//amsg.lParam = msg->lParam;
			//amsg.wParam = msg->wParam;
			//amsg.message = msg->message;
			//amsg.time = 0;
			//amsg.pt = { 200, 200 };
			//DispatchMessage(&amsg);
		}
	}
	closesocket(MySocket);
	return 0;
}

typedef struct _MSG_SCREEN
{
	BITMAPINFOHEADER bi;
	int dwBmpSize;
}MSG_SCREEN;

void GetScreen(HWND hWnd, SOCKET socket)
{
	TCHAR	mess[128];
	if (hWnd == NULL) return;

	RECT rectClient;
	GetClientRect(NULL, &rectClient);
	int width = 1366;
	int height = 768;

	// 通过内存DC复制客户区到DDB位图
//	HDC hdcWnd = GetDC(hWnd);
	HDC hdcWnd = GetDC(NULL);
	HBITMAP hbmWnd = CreateCompatibleBitmap(hdcWnd, width, height);
	HDC hdcMem = CreateCompatibleDC(hdcWnd);
	SelectObject(hdcMem, hbmWnd);
	BitBlt(hdcMem, 0, 0, width, height, hdcWnd, 0, 0, SRCCOPY);

	// 把窗口DDB转为DIB
	BITMAP bmpWnd;
	GetObject(hbmWnd, sizeof(BITMAP), &bmpWnd);
	BITMAPINFOHEADER bi; // 信息头
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmpWnd.bmWidth;
	bi.biHeight = bmpWnd.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 32; // 按照每个像素用32bits表示转换
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = bi.biYPelsPerMeter = 0;
	bi.biClrUsed = bi.biClrImportant = 0;
	
	DWORD dwBmpSize = ((bmpWnd.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpWnd.bmHeight; // 每一行像素位32对齐
	BYTE *lpbitmap = (BYTE*)malloc(dwBmpSize); // 像素位指针
	GetDIBits(hdcMem, hbmWnd, 0, (UINT)bmpWnd.bmHeight,
		lpbitmap,
		(BITMAPINFO*)&bi,
		DIB_RGB_COLORS);

	DeleteDC(hdcMem);
	DeleteObject(hbmWnd);
	ReleaseDC(hWnd, hdcWnd);

	char *send_buf = (char*)malloc(sizeof(MSG_SCREEN)+dwBmpSize); // 像素位指针
	MSG_SCREEN *msg_head = (MSG_SCREEN *)send_buf;
	msg_head->dwBmpSize = htonl(dwBmpSize);
	memcpy(&(msg_head->bi), &bi, sizeof(BITMAPINFOHEADER));
	memcpy(send_buf + sizeof(MSG_SCREEN), lpbitmap, dwBmpSize);

//	int count = send(socket, send_buf, sizeof(MSG_SCREEN)+dwBmpSize, 0);
	memset(mess, '\0', sizeof(mess));
	wsprintf(mess, L"width=%d height=%d size=%d %d", msg_head->bi.biWidth, msg_head->bi.biHeight,
		msg_head->dwBmpSize, dwBmpSize);
	HDC hdc = GetDC(hWnd);
	TextOut(hdc, 0, 100, mess, 60);
	ReleaseDC(hWnd, hdc);

	{
		BYTE *outdata;
		BYTE *rawdata;
		ULONG nSize;
		compress2jpeg(lpbitmap, bi.biWidth, bi.biHeight, &outdata, &nSize);
		decompress(outdata, nSize, rawdata);
		HDC hdc = GetDC(hWnd);
		StretchDIBits(hdc,
			0, 0, bi.biWidth, bi.biHeight,
			0, 0, bi.biWidth, bi.biHeight,
			rawdata,
			(BITMAPINFO*)&(bi),
			DIB_RGB_COLORS,
			SRCCOPY);
		ReleaseDC(hWnd, hdc);
	}
}

BOOL compress2jpeg(BYTE raw[], JDIMENSION width, JDIMENSION height, BYTE **outdata, ULONG *nSize)
{
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);

	*outdata = (BYTE*)malloc(4000000); // 用于缓存
	jpeg_mem_dest(&cinfo, outdata, nSize);

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, 80, true);

	jpeg_start_compress(&cinfo, true);
	JSAMPROW row_pointer[1];	// 一行位图
	int row_stride;				// 每一行的字节数

	row_stride = cinfo.image_width * 3;	// 如果不是索引图,此处需要乘以3

	// 对每一行进行压缩
	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = &raw[cinfo.next_scanline * row_stride];
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	return true;
}

void decompress(BYTE raw[], int nSize, BYTE *&data)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row_pointer[1];

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_mem_src(&cinfo, raw, nSize);
	jpeg_read_header(&cinfo, TRUE);
	data = (BYTE*)malloc(sizeof(BYTE)*cinfo.image_width*cinfo.image_height*cinfo.num_components);
	jpeg_start_decompress(&cinfo);

	while (cinfo.output_scanline < cinfo.output_height)
	{
		row_pointer[0] = &data[cinfo.output_scanline*cinfo.image_width*cinfo.num_components];
		jpeg_read_scanlines(&cinfo, row_pointer, 1);
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
}