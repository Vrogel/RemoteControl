#include "stdafx.h"
#include <WinSock2.h>
#include <stdio.h>
#include <GdiPlus.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"gdiPlus.lib")

extern HINSTANCE hInst;		// ��ǰʵ��
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
			hThread = CreateThread(NULL, 0, ClientThread, NULL, 0, &dwThreadId);
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
	////RGB˳�����
	//for (int i = 0, j = 0; j < 1600 * 900 * 4; i += 3, j += 4)
	//{
	//	*(image_buf + i) = *(image_buffer + j + 2);
	//	*(image_buf + i + 1) = *(image_buffer + j + 1);
	//	*(image_buf + i + 2) = *(image_buffer + j);
	//}
	size_t length = bi.biSizeImage;

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR pGdiToken;
	GdiplusStartup(&pGdiToken, &gdiplusStartupInput, NULL);//��ʼ��GDI+
	char *pbuf;
	int len;
	SaveCurScreenJpg(L"save.jpg", 1366, 768, 100, &pbuf, &len);
	GdiplusShutdown(pGdiToken);

	char *send_buf = (char*)malloc(sizeof(MSG_SCREEN)+len); // ����λָ��
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


/****************BMPתJPG*********�÷�ʾ��**************************

Bitmap newbitmap(L"d:\\d.bmp");//����BMP
const unsigned short *pFileName=L"d:\\new.jpg";//����·��
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


// ����ǰ��Ļ�����ΪjpgͼƬ       
// ����xsͼ��x���С ysͼ��y���С qualityͼ������       
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
		BYTE *pbyjpg = (BYTE *)GlobalLock(hMemJpg);//�����pbyjpg����ת�����JPG���ݿ�ֱ��ͨ�����緢��

		*len = GlobalSize(hMemJpg);//�����תΪJPG���ļ��Ĵ�С��

		*pBuf = new char[*len];
		memcpy(*pBuf, pbyjpg, *len);//���Ա��������,�������Ŷ

		GlobalUnlock(hMemJpg);
		GlobalFree(hMemJpg);
	}


	::DeleteObject(hbmp);
	::DeleteObject(hmemdc);
	return;
}

/*
HBITMAP ReturnHBITMAP(CString FileName)//FileName������bmp��dib��png��gif��jpeg/jpg��tiff��emf���ļ����ļ��� 
{
	Bitmap	tempBmp(FileName.AllocSysString());
	Color	backColor;
	HBITMAP	HBitmap;
	tempBmp.GetHBITMAP(backColor, &HBitmap);
	return	HBitmap;
}
*/