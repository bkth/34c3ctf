#ifndef KCOMPRESSOR_H
#define KCOMPRESSOR_H



int try_inplace_edit(char* compressed_buf, char* new_buf, size_t offset, size_t len);
char* deflate(char* buf, uint32_t size, uint8_t* compressed, uint64_t* real_length);
char* inflate(char* buf);


#endif
