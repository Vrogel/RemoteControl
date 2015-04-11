#include "stdafx.h"
#include <WinSock2.h>
#include <stdio.h>
#include <GdiPlus.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"gdiPlus.lib")

extern HINSTANCE hInst;		// 当前实例
extern HWND hwnd;
SOCKET Listen, Socket;
int gPort = 5050;
using namespace Gdiplus;
struct _Remote_MSG
{
	HWND hWnd; UINT message; WPARAM wParam; LPARAM lParam;
};
typedef struct _Remote_MSG Remote_MSG;

DWORD WINAPI ClientThread(LPVOID lpParam);
void GetScreen(HWND hWnd, SOCKET socket);
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
	// TCHAR	mess[128];
	//	MSG		amsg;
	// HDC		hdc;

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
	BITMAPINFOHEADER bi;
	int dwBmpSize;
}MSG_SCREEN;

void SaveCurScreenJpg(LPCWSTR pszFileName, int xs, int ys, int quality, char **pBuf, int *len);
void GetScreen(HWND hWnd, SOCKET socket)
{
	if (hWnd == NULL) return;

	BYTE *image_buffer = (BYTE *)malloc(1600 * 900 * 4);
	BYTE *image_buf = (BYTE *)malloc(1600 * 900 * 3);

	BITMAPINFOHEADER bi;
	capscreen(image_buffer, bi);
	////RGB顺序调整
	//for (int i = 0, j = 0; j < 1600 * 900 * 4; i += 3, j += 4)
	//{
	//	*(image_buf + i) = *(image_buffer + j + 2);
	//	*(image_buf + i + 1) = *(image_buffer + j + 1);
	//	*(image_buf + i + 2) = *(image_buffer + j);
	//}
	size_t length = bi.biSizeImage;

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR pGdiToken;
	GdiplusStartup(&pGdiToken, &gdiplusStartupInput, NULL);//初始化GDI+
	char *pbuf;
	int len;
	SaveCurScreenJpg(L"save.jpg", 1366, 768, 100, &pbuf, &len);
	GdiplusShutdown(pGdiToken);

	char *send_buf = (char*)malloc(sizeof(MSG_SCREEN)+len); // 像素位指针
	MSG_SCREEN *msg_head = (MSG_SCREEN *)send_buf;
	msg_head->dwBmpSize = htonl(len);
	memcpy(&(msg_head->bi), &bi, sizeof(BITMAPINFOHEADER));
	memcpy(send_buf + sizeof(MSG_SCREEN), pbuf, len);

	int count = send(socket, send_buf, sizeof(MSG_SCREEN)+len, 0);

//	savejpeg("ok.jpg", image_buf, 1600, 900, 3);
	return;
	/*{
		BYTE *outdata;
		BYTE *rawdata;
		ULONG nSize;
		compress2jpeg(rawbitmap, bi.biWidth, bi.biHeight, &outdata, &nSize);
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
	}*/
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


/****************BMP转JPG*********用法示例**************************

Bitmap newbitmap(L"d:\\d.bmp");//加载BMP
const unsigned short *pFileName=L"d:\\new.jpg";//保存路径
SaveFile(&newbitmap,pFileName );

************************************************************/

void SaveFile(Bitmap* pImage, const wchar_t* pFileName)//
{
	EncoderParameters encoderParameters;
	CLSID jpgClsid;
	GetEncoderClsid(L"image/jpeg", &jpgClsid);
	encoderParameters.Count = 1;
	encoderParameters.Parameter[0].Guid = EncoderQuality;
	encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
	encoderParameters.Parameter[0].NumberOfValues = 1;

	// Save the image as a JPEG with quality level 100.
	ULONG             quality;
	quality = 100;
	encoderParameters.Parameter[0].Value = &quality;
	Status status = pImage->Save(pFileName, &jpgClsid, &encoderParameters);
	if (status != Ok)
	{
		wprintf(L"%d Attempt to save %s failed.\n", status, pFileName);
	}
}


// 将当前屏幕保存成为jpg图片       
// 参数xs图象x轴大小 ys图象y轴大小 quality图象质量       
void SaveCurScreenJpg(LPCWSTR pszFileName, int xs, int ys, int quality, char **pBuf, int *len)
{
	HWND hwnd = ::GetDesktopWindow();
	HDC hdc = GetWindowDC(NULL);
	int x = GetDeviceCaps(hdc, HORZRES);
	int y = GetDeviceCaps(hdc, VERTRES);
	HBITMAP hbmp = ::CreateCompatibleBitmap(hdc, x, y), hold;
	HDC hmemdc = ::CreateCompatibleDC(hdc);
	hold = (HBITMAP)::SelectObject(hmemdc, hbmp);
	BitBlt(hmemdc, 0, 0, x, y, hdc, 0, 0, SRCCOPY);
	SelectObject(hmemdc, hold);

	Bitmap bit(xs, ys), bit2(hbmp, NULL);
	Graphics g(&bit);
	g.ScaleTransform((float)xs / x, (float)ys / y);
	g.DrawImage(&bit2, 0, 0);

	CLSID               encoderClsid;
	EncoderParameters   encoderParameters;

	encoderParameters.Count = 1;
	encoderParameters.Parameter[0].Guid = EncoderQuality;
	encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
	encoderParameters.Parameter[0].NumberOfValues = 1;
	encoderParameters.Parameter[0].Value = &quality;

	GetEncoderClsid(L"image/jpeg", &encoderClsid);
	bit.Save(pszFileName, &encoderClsid, &encoderParameters);

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


	::DeleteObject(hbmp);
	::DeleteObject(hmemdc);
	return;
}

/*
HBITMAP ReturnHBITMAP(CString FileName)//FileName可能是bmp、dib、png、gif、jpeg/jpg、tiff、emf等文件的文件名 
{
	Bitmap	tempBmp(FileName.AllocSysString());
	Color	backColor;
	HBITMAP	HBitmap;
	tempBmp.GetHBITMAP(backColor, &HBitmap);
	return	HBitmap;
}
*/