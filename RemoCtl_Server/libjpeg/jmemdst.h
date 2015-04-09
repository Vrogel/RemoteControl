/* this is not a core library module, so it doesn't define JPEG_INTERNALS */
#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"

/* Expanded data destination object for memio output */
typedef struct{
  struct jpeg_destination_mgr pub; /* public fields */

  unsigned char* outdata;//�¼ӱ��������ݽ��ջ������ַ
  //FILE * outfile;		/* target stream */
  unsigned long* pSize;//�¼ӱ��������ݽ��ջ���Ĵ�С��ѹ����󷵻�ͼ���С
  unsigned long nOutOffset;//�¼ӱ�������ǰ��д�����ݵ�ƫ����
  JOCTET * buffer;		//��ǰ�ڲ�ѹ������Ĳ������ݵ���ʱ������ַ
} my_dest_mgr;
typedef my_dest_mgr *my_dest_mgrp;

#define OUTPUT_BUF_SIZE  4096	/* choose an efficiently fwrite'able size */

GLOBAL(void) jpeg_memio_dest(j_compress_ptr cinfo, unsigned char* outdata, unsigned long* pSize/*NOT NULL*/);
//EXTERN(void) jpeg_memio_dest JPP((j_compress_ptr cinfo, unsigned char* outdata, unsigned long* pSize));//��Ҫ����Lib.

//�����ص������ص�������
void init_mem_dest(j_compress_ptr cinfo);
boolean empty_mem_output_buffer(j_compress_ptr cinfo);
void term_mem_dest(j_compress_ptr cinfo);