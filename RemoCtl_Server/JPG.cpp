//JPG.cpp: implementation of the CJPG class.

#include "stdafx.h"
#include "libjpeg\JPG.h"

CJPG::CJPG()
{

}

CJPG::~CJPG()
{

}

BOOL CJPG::IsJPEG(LPCTSTR strFile)
{
  BOOL bRet=FALSE;
  if(!strFile)
    return bRet;
  FILE* pFile=_tfopen(strFile, TEXT("rb"));
  if(!pFile)
    return bRet;
  DWORD dwFlag=0;
  if(1==fread(&dwFlag, sizeof(dwFlag), 1, pFile)){
    if(0xe0ffd8ff==dwFlag||0xe1ffd8ff==dwFlag)
      bRet=TRUE;
  }
  fclose(pFile);
  return bRet;
}

BOOL CJPG::ReadFromFile(LPCTSTR strFile, LPCOLORREF pArrayRGBA, int& iWidth, int& iHeight)
{
  BOOL bRet=FALSE;
  if(!strFile)
    return bRet;

  my_error_mgr jerr;//建立出错处理对象
  struct jpeg_decompress_struct cinfo;
  cinfo.err=jpeg_std_error(&jerr.pub);//关联出错处理对象
  if(!cinfo.err)
    return bRet;
  jerr.pub.error_exit=my_error_exit;//设定出错处理函数
  
  if(setjmp(jerr.setjmp_buffer))
	{
		jpeg_destroy_decompress(&cinfo);
    return FALSE;
	}
  jpeg_create_decompress(&cinfo);//建立解码器
  unsigned char* g_buf=NULL;
  
  do{
    FILE* infile=_tfopen(strFile, L"rb");
    if(!infile)
      break;
    //获取文件大小：
    fseek(infile, 0, SEEK_END);
    size_t g_buf_length=ftell(infile);
    //根据文件大小建立缓冲：
    g_buf=new unsigned char[g_buf_length];
    if(!g_buf)
      break;
    
    //一次性读入数据：
    fseek(infile, 0, SEEK_SET);
    fread(g_buf, g_buf_length, 1, infile);
    fclose(infile);
    
    //配置解码器从内存源读入数据：
    jpeg_source_mgr my_smgr;
    my_smgr.next_input_byte=g_buf;
    my_smgr.bytes_in_buffer=g_buf_length;
    my_smgr.init_source=init_mem_src;
    my_smgr.fill_input_buffer=fill_mem_input_buffer;
    my_smgr.skip_input_data=skip_mem_input_data;
    my_smgr.resync_to_restart=jpeg_resync_to_restart;
    my_smgr.term_source=term_mem_src;
    cinfo.src=&my_smgr;
    
    if(setjmp(jerr.setjmp_buffer))
      break;
    //读入头部数据：
    (void)jpeg_read_header(&cinfo, TRUE);

    if(setjmp(jerr.setjmp_buffer)){
      if(setjmp(jerr.setjmp_buffer))
        break;
		  (void)jpeg_abort_decompress(&cinfo);
      break;
    }
    //启动解码器：
    (void)jpeg_start_decompress(&cinfo);

    if(cinfo.output_components!=3)
      break;
    iHeight=cinfo.output_height;
    iWidth=cinfo.output_width;
    LPBYTE pOutData=(LPBYTE)pArrayRGBA;
    if(!pOutData){
      bRet=TRUE;
      break;
    }

    /* JSAMPLEs per row in output buffer */
    int row_stride=cinfo.output_width*cinfo.output_components;
    /* Make a one-row-high sample array that will go away when done with image */
    JSAMPARRAY buffer=(*cinfo.mem->alloc_sarray)
      ((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);
    UINT i=0;//cinfo.output_height-1;
    while (cinfo.output_scanline<cinfo.output_height){
      (void)jpeg_read_scanlines(&cinfo, buffer, 1);
      j_putRGBScanline(buffer[0], cinfo.output_width, (BYTE*)((DWORD*)pOutData+i*cinfo.output_width));
      i++;//i--;
    }
    (void)jpeg_finish_decompress(&cinfo);
    bRet=TRUE;
  }while(FALSE);
  if(g_buf){
    delete[] g_buf;
  }
  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress(&cinfo);
  return bRet;
}

void CJPG::j_putRGBScanline(BYTE* jpegline, int widthPix, BYTE* outBuf)
{
	int count=0;
	for(; count<widthPix; count++){
		LPBYTE oAlpha, oRed, oBlu, oGrn;
    oAlpha=outBuf+count*4+3;//outBuf是输出ARGB这4个字节所以一次+4
    oRed=outBuf+count*4+2;
		oGrn=outBuf+count*4+1;
		oBlu=outBuf+count*4;

    *oAlpha=0xff;
		*oRed=*(jpegline+count*3);//libjpeg的内存只有RGB这3个一组
		*oGrn=*(jpegline+count*3+1);
		*oBlu=*(jpegline+count*3+2);
	}
}

BOOL CJPG::ReadFromMemory(const LPBYTE lpSrcBuf, DWORD dwSrcBufSize, LPCOLORREF pArrayRGBA, int& iWidth, int& iHeight)
{
  BOOL bRet=FALSE;
  if(!lpSrcBuf || dwSrcBufSize==0)
    return bRet;

  my_error_mgr jerr;//建立出错处理对象
  struct jpeg_decompress_struct cinfo;
  cinfo.err=jpeg_std_error(&jerr.pub);//关联出错处理对象
  if(!cinfo.err)
    return bRet;
  jerr.pub.error_exit=my_error_exit;//设定出错处理函数
  
  if(setjmp(jerr.setjmp_buffer))
	{
		jpeg_destroy_decompress(&cinfo);
    return FALSE;
	}
  jpeg_create_decompress(&cinfo);//建立解码器
  
  do{
    //配置解码器从内存源读入数据：
    jpeg_source_mgr my_smgr;
    my_smgr.next_input_byte=lpSrcBuf;
    my_smgr.bytes_in_buffer=dwSrcBufSize;
    my_smgr.init_source=init_mem_src;
    my_smgr.fill_input_buffer=fill_mem_input_buffer;
    my_smgr.skip_input_data=skip_mem_input_data;
    my_smgr.resync_to_restart=jpeg_resync_to_restart;
    my_smgr.term_source=term_mem_src;
    cinfo.src=&my_smgr;
    
    if(setjmp(jerr.setjmp_buffer))
      break;
    //读入头部数据：
    (void)jpeg_read_header(&cinfo, TRUE);

    if(setjmp(jerr.setjmp_buffer)){
      if(setjmp(jerr.setjmp_buffer))
        break;
		  (void)jpeg_abort_decompress(&cinfo);
      break;
    }
    //启动解码器：
    (void)jpeg_start_decompress(&cinfo);

    if(cinfo.output_components!=3)
      break;
    iHeight=cinfo.output_height;
    iWidth=cinfo.output_width;
    LPBYTE pOutData=(LPBYTE)pArrayRGBA;
    if(!pOutData){
      bRet=TRUE;
      break;
    }

    /* JSAMPLEs per row in output buffer */
    int row_stride=cinfo.output_width*cinfo.output_components;
    /* Make a one-row-high sample array that will go away when done with image */
    JSAMPARRAY buffer=(*cinfo.mem->alloc_sarray)
      ((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);
    UINT i=0;//cinfo.output_height-1;
    while (cinfo.output_scanline<cinfo.output_height){
      (void)jpeg_read_scanlines(&cinfo, buffer, 1);
      j_putRGBScanline(buffer[0], cinfo.output_width, (BYTE*)((DWORD*)pOutData+i*cinfo.output_width));
      i++;//i--;
    }
    (void)jpeg_finish_decompress(&cinfo);
    bRet=TRUE;
  }while(FALSE);
  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress(&cinfo);
  return bRet;
}

BOOL CJPG::WriteToFile(LPCTSTR strFile, const LPCOLORREF pArrayRGBA, const UINT iWidth, const UINT iHeight, int iQuality/*0..100*/)
{
  BOOL bRet=FALSE;
  if(!strFile || !pArrayRGBA || !(iWidth>0 && iHeight>0))
    return bRet;

  my_error_mgr jerr;//建立出错处理对象
  struct jpeg_compress_struct cinfo;
  cinfo.err = jpeg_std_error(&jerr.pub);//关联出错处理对象
  if(!cinfo.err)
    return bRet;
  jerr.pub.error_exit=my_error_exit;//设定出错处理函数

  if(setjmp(jerr.setjmp_buffer))
	{
    jpeg_destroy_compress(&cinfo);
    return FALSE;
	}
  jpeg_create_compress(&cinfo);//建立编码器

  //配置编码器输出文件：
  FILE* pOutFile=_tfopen(strFile, L"wb");
  if(!pOutFile)
    return bRet;
  jpeg_stdio_dest(&cinfo, pOutFile);

  //设置编码器输入数据信息：
  cinfo.in_color_space=JCS_RGB;//colorspace of input image.
  cinfo.image_width=iWidth;//image width and height, in pixels.
  cinfo.image_height=iHeight;
  cinfo.input_components=3;//color components per pixel.
  jpeg_set_defaults(&cinfo);
  //设置编码质量参数：
  jpeg_set_quality(&cinfo, iQuality, TRUE/*limit to baseline-JPEG values*/);

  LPBYTE pByte=(LPBYTE)pArrayRGBA;
  //BGRA->RGB调整输入数据格式：
  for(ULONG i=0; i<iWidth*iHeight; i++){
    pByte[(i+1)*4-1]=pByte[i*4];//BR交换，使用最后一位暂存B
    pByte[i*3]=pByte[i*4+2];//R
    pByte[i*3+2]=pByte[(i+1)*4-1];//B
    pByte[i*3+1]=pByte[i*4+1];//G本位平移
    pByte[(i+1)*4-1]=0xFF;
  }

  //启动编码器：
  if(setjmp(jerr.setjmp_buffer)){
    if(setjmp(jerr.setjmp_buffer))
      return FALSE;
		(void)jpeg_abort_compress(&cinfo);
    return FALSE;
  }
  jpeg_start_compress(&cinfo, TRUE);

  JSAMPROW row_pointer[1]={NULL};//pointer to JSAMPLE row[s].
  int row_stride=iWidth*cinfo.input_components;//JSAMPLEs per row in image_buffer.
  while(cinfo.next_scanline<cinfo.image_height){
    row_pointer[0]=&pByte[cinfo.next_scanline*row_stride];
    (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }
  jpeg_finish_compress(&cinfo);
  bRet=TRUE;
  fclose(pOutFile);
  jpeg_destroy_compress(&cinfo);
  return bRet;
}

//需要修改：
BOOL CJPG::WriteToMemory(LPBYTE lpDestBuf, DWORD& dwDestBufSize, const LPCOLORREF pArrayRGBA, const UINT iWidth, const UINT iHeight, int iQuality/*0..100*/)
{
  BOOL bRet=FALSE;
  if(!pArrayRGBA || !(iWidth>0 && iHeight>0))
    return bRet;

  my_error_mgr jerr;//建立出错处理对象
  struct jpeg_compress_struct cinfo;
  cinfo.err = jpeg_std_error(&jerr.pub);//关联出错处理对象
  if(!cinfo.err)
    return bRet;
  jerr.pub.error_exit=my_error_exit;//设定出错处理函数

  if(setjmp(jerr.setjmp_buffer))
	{
    jpeg_destroy_compress(&cinfo);
    return FALSE;
	}
  jpeg_create_compress(&cinfo);//建立编码器

  //配置编码器的数据输出内存：
  jpeg_memio_dest(&cinfo, lpDestBuf, &dwDestBufSize);
  //设置编码器输入数据信息：
  cinfo.in_color_space=JCS_RGB;//colorspace of input image.
  cinfo.image_width=iWidth;//image width and height, in pixels.
  cinfo.image_height=iHeight;
  cinfo.input_components=3;//color components per pixel.
  jpeg_set_defaults(&cinfo);
  //设置编码质量参数：
  jpeg_set_quality(&cinfo, iQuality, TRUE/*limit to baseline-JPEG values*/);

  LPBYTE pByte=NULL;
  do 
  {
    pByte=new BYTE[iWidth*iHeight*sizeof(COLORREF)];//防止两次调用改变原数据，因此使用备份。
    if(!pByte)
      break;
    ULONG i;
    for(i=0; i<iWidth*iHeight; i++){
      *(LPCOLORREF(pByte)+i)=pArrayRGBA[i];
    }
    //BGRA->RGB调整输入数据格式：
    for(i=0; i<iWidth*iHeight; i++){
      pByte[(i+1)*4-1]=pByte[i*4];//BR交换，使用最后一位暂存B
      pByte[i*3]=pByte[i*4+2];//R
      pByte[i*3+2]=pByte[(i+1)*4-1];//B
      pByte[i*3+1]=pByte[i*4+1];//G本位平移
      pByte[(i+1)*4-1]=0xFF;
    }
    //启动编码器：
    if(setjmp(jerr.setjmp_buffer)){
      if(setjmp(jerr.setjmp_buffer))
        break;
      (void)jpeg_abort_compress(&cinfo);
      break;
    }
    jpeg_start_compress(&cinfo, TRUE);
    
    
    JSAMPROW row_pointer[1]={NULL};//pointer to JSAMPLE row[s].
    int row_stride=iWidth*cinfo.input_components;//JSAMPLEs per row in image_buffer.
    while(cinfo.next_scanline<cinfo.image_height){
      row_pointer[0]=&pByte[cinfo.next_scanline*row_stride];
      (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    jpeg_finish_compress(&cinfo);
    bRet=TRUE;
  }while(FALSE);
  jpeg_destroy_compress(&cinfo);
  if(pByte){
    delete[] pByte;
  }
  return bRet;
}
