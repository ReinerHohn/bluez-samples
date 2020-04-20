#ifndef COMMON_H
#define COMMON_H

#define BUF_SIZE 32768
#define ALIGN_SIZE 64

int set_l2cap_mtu( int sock, uint16_t mtu ) ;
void aligned_free(void * ptr);
void * aligned_malloc(size_t align, size_t size);

#endif // COMMON_H
