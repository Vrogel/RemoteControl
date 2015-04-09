/* this is not a core library module, so it doesn't define JPEG_INTERNALS */
#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"

/* Expanded data destination object for memio output */
typedef struct{
  struct jpeg_destination_mgr pub; /* public fields */

  unsigned char* outdata;//新加变量，数据接收缓冲的首址
  //FILE * outfile;		/* target stream */
  unsigned long* pSize;//新加变量，数据接收缓冲的大小，压缩完后返回图像大小
  unsigned long nOutOffset;//新加变量，当前已写入数据的偏移量
  JOCTET * buffer;		//当前内部压缩输出的部分数据的临时缓冲首址
} my_dest_mgr;
typedef my_dest_mgr *my_dest_mgrp;

#define OUTPUT_BUF_SIZE  4096	/* choose an efficiently fwrite'able size */

GLOBAL(void) jpeg_memio_dest(j_compress_ptr cinfo, unsigned char* outdata, unsigned long* pSize/*NOT NULL*/);
//EXTERN(void) jpeg_memio_dest JPP((j_compress_ptr cinfo, unsigned char* outdata, unsigned long* pSize));//如要生成Lib.

//被重载的三个回调函数：
void init_mem_dest(j_compress_ptr cinfo);
boolean empty_mem_output_buffer(j_compress_ptr cinfo);
void term_mem_dest(j_compress_ptr cinfo);