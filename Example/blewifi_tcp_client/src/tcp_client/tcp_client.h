#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__


#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

uintptr_t tcp_Establish(const char *host, uint16_t port);
int tcp_Disconnect(uintptr_t fd);
int tcp_Send(uintptr_t fd, const char *buf, uint32_t len, uint32_t timeout_ms);
int tcp_Recv(uintptr_t fd, char *buf, uint32_t len, uint32_t timeout_ms);


#endif
