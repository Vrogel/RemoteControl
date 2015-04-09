#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"

//解码器数据重定向到内存时必须重载的：
void init_mem_src(j_decompress_ptr pInfo);
boolean fill_mem_input_buffer(j_decompress_ptr pInfo);
void skip_mem_input_data(j_decompress_ptr pInfo, long num_bytes);
//boolean resync_mem_to_restart(j_decompress_ptr pInfo, int desired);
void term_mem_src(j_decompress_ptr pInfo);
