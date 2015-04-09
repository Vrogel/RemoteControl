#include "stdafx.h"
#include "libjpeg\jmemsrc.h"

//解码器数据重定向到内存时必须重载的：
void init_mem_src(j_decompress_ptr pInfo)
{
}

boolean fill_mem_input_buffer(j_decompress_ptr pInfo)
{
	return TRUE;
}

void skip_mem_input_data(j_decompress_ptr pInfo, long num_bytes)
{
	(pInfo->src->bytes_in_buffer)-=num_bytes;
	(pInfo->src->next_input_byte)+=num_bytes;
}

void term_mem_src(j_decompress_ptr pInfo)
{
}
