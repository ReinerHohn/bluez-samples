#ifndef COMMON_H
#define COMMON_H

#define PACKAGE_SIZE 1008
#define BUF_SIZE 32768

#define USE_L2CAP 1
//#define L2CAP_MTU PACKAGE_SIZE

int set_l2cap_mtu( int sock, uint16_t mtu ) ;

#endif // COMMON_H
