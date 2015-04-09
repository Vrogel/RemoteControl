// JPG.h: interface for the CJPG class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_JPG_H__92C9E694_52F0_42EF_8DBA_5EF03C4ABE77__INCLUDED_)
#define AFX_JPG_H__92C9E694_52F0_42EF_8DBA_5EF03C4ABE77__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef __cplusplus
extern "C"{
#endif
#include "jpeglib.h"
#ifdef __cplusplus
};
#endif
#include "jmemdst.h"
#include "jmemsrc.h"
#include "jmyerror.h"

class CJPG  
{
public:
	CJPG();
	virtual ~CJPG();
	BOOL IsJPEG(LPCTSTR strFile);
  BOOL ReadFromFile(LPCTSTR strFile, LPCOLORREF pArrayRGBA, int& iWidth, int& iHeight);
  BOOL WriteToFile(LPCTSTR strFile, const LPCOLORREF pArrayRGBA, const UINT iWidth, const UINT iHeight, int iQuality=100/*0..100*/);
  BOOL ReadFromMemory(const LPBYTE lpSrcBuf, DWORD dwSrcBufSize, LPCOLORREF pArrayRGBA, int& iWidth, int& iHeight);
  
  //ÐèÒªÐÞ¸Ä£º
  BOOL WriteToMemory(LPBYTE lpDestBuf, DWORD& dwDestBufSize, const LPCOLORREF pArrayRGBA, const UINT iWidth, const UINT iHeight, int iQuality=100/*0..100*/);
protected:
  void j_putRGBScanline(BYTE* jpegline, int widthPix, BYTE* outBuf);
};

#endif // !defined(AFX_JPG_H__92C9E694_52F0_42EF_8DBA_5EF03C4ABE77__INCLUDED_)
