#include "stdafx.h"
#include "libjpeg\JPG.h"
#include <WinSock2.h>
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"jpeg.lib")
#define JDIMENSION UINT
extern HINSTANCE hInst;		// ��ǰʵ��
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
	TCHAR	mess[128];
//	MSG		amsg;
	HDC		hdc;
	
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

	// ͨ���ڴ�DC���ƿͻ�����DDBλͼ
//	HDC hdcWnd = GetDC(hWnd);
	HDC hdcWnd = GetDC(NULL);
	HBITMAP hbmWnd = CreateCompatibleBitmap(hdcWnd, width, height);
	HDC hdcMem = CreateCompatibleDC(hdcWnd);
	SelectObject(hdcMem, hbmWnd);
	BitBlt(hdcMem, 0, 0, width, height, hdcWnd, 0, 0, SRCCOPY);

	// �Ѵ���DDBתΪDIB
	BITMAP bmpWnd;
	GetObject(hbmWnd, sizeof(BITMAP), &bmpWnd);
	BITMAPINFOHEADER bi; // ��Ϣͷ
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmpWnd.bmWidth;
	bi.biHeight = bmpWnd.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 32; // ����ÿ��������32bits��ʾת��
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = bi.biYPelsPerMeter = 0;
	bi.biClrUsed = bi.biClrImportant = 0;
	
	DWORD dwBmpSize = ((bmpWnd.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpWnd.bmHeight; // ÿһ������λ32����
	BYTE *lpbitmap = (BYTE*)malloc(dwBmpSize); // ����λָ��
	GetDIBits(hdcMem, hbmWnd, 0, (UINT)bmpWnd.bmHeight,
		lpbitmap,
		(BITMAPINFO*)&bi,
		DIB_RGB_COLORS);

	DeleteDC(hdcMem);
	DeleteObject(hbmWnd);
	ReleaseDC(hWnd, hdcWnd);

	char *send_buf = (char*)malloc(sizeof(MSG_SCREEN)+dwBmpSize); // ����λָ��
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

	*outdata = (BYTE*)malloc(4000000); // ���ڻ���
	jpeg_mem_dest(&cinfo, outdata, nSize);

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, 80, true);

	jpeg_start_compress(&cinfo, true);
	JSAMPROW row_pointer[1];	// һ��λͼ
	int row_stride;				// ÿһ�е��ֽ���

	row_stride = cinfo.image_width * 3;	// �����������ͼ,�˴���Ҫ����3

	// ��ÿһ�н���ѹ��
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