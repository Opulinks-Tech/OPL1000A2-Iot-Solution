#include <stdio.h>
#include <string.h>
#include "tcp_client.h"

uintptr_t tcp_Establish(const char *host, uint16_t port)
{
    struct addrinfo hints;
    struct addrinfo *addrInfoList = NULL;
    struct addrinfo *cur = NULL;
    int fd = 0;
    int rc = -1;
    char service[6];
    int sockopt = 1;

    memset(&hints, 0, sizeof(hints));

    printf("establish tcp connection with server(host=%s port=%u)\n", host, port);

    hints.ai_family = AF_INET; /* only IPv4 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    sprintf(service, "%u", port);

    if ((rc = getaddrinfo(host, service, &hints, &addrInfoList)) != 0) {
        printf("getaddrinfo error=%d, errno=%d, %s\n", rc, errno, strerror(errno));
        return (uintptr_t)(-1);
    }

    for (cur = addrInfoList; cur != NULL; cur = cur->ai_next) {
        if (cur->ai_family != AF_INET) {
            printf("socket type error\n");
            rc = -1;
            continue;
        }

        fd = socket(cur->ai_family, cur->ai_socktype, cur->ai_protocol);
        if (fd < 0) {
            printf("create socket error(fd=%d), errno=%d, %s\n", fd, errno, strerror(errno));
            rc = -1;
            continue;
        }

        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
        setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &sockopt, sizeof(sockopt));

        if (connect(fd, cur->ai_addr, cur->ai_addrlen) == 0) {
            rc = fd;
            break;
        }

        close(fd);
        printf("connect error");
        rc = -1;
    }

    if (-1 == rc) {
        printf("fail to establish tcp\n");
    } else {
        printf("success to establish tcp, fd=%d\n", rc);
    }

if (addrInfoList != NULL)
    freeaddrinfo(addrInfoList);

    return (uintptr_t)rc;
}

int tcp_Disconnect(uintptr_t fd)
{
    int rc;
    printf("tcp_disconnect\n");

    /* Shutdown both send and receive operations. */
    if (fd>0 || fd==0){
#if 0
        rc = shutdown((int) fd, 2);
        if (0 != rc) {
            printf("shutdown error: fd[%d] rc[%d] %s\n", fd, rc, strerror(errno));
//            return -1;
        }
#endif
        rc = close((int) fd);
        if (0 != rc) {
            printf("closesocket error: fd[%d] rc[%d] %s\n", fd, rc, strerror(errno));
            return -1;
        }
    }

    return 0;
}

int tcp_Send(uintptr_t fd, const char *buf, uint32_t len, uint32_t timeout_ms)
{
    int ret;
    uint32_t len_sent;
    int net_err = 0;
    struct timeval timeout;

    timeout.tv_sec = timeout_ms/1000;
    timeout.tv_usec = (timeout_ms - (timeout.tv_sec * 1000)) * 1000;

    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        printf("failed to set socket sending timeout\r\n");
        net_err = -2;
        return net_err;
    }

    ret = send(fd, buf, len, 0);
    if (ret > 0)
    {
        len_sent += ret;
    }
    else if (0 == ret)
    {
        printf("No data be sent\n");
    }
    else
    {
        printf("send fail: fd[%d] ret[%d] %s\n", fd, ret, strerror(errno));
        net_err = 1;
    }

    if (net_err)
    {
        return -1;
    }
    else
    {
        return len_sent;
    }
}

int tcp_Recv(uintptr_t fd, char *buf, uint32_t len, uint32_t timeout_ms)
{
    int ret, err_code;
    uint32_t len_recv;
    fd_set sets;
    struct timeval timeout;

    len_recv = 0;
    err_code = 0;

    FD_ZERO(&sets);
    FD_SET(fd, &sets);

    timeout.tv_sec = timeout_ms/1000;
    timeout.tv_usec = (timeout_ms - (timeout.tv_sec * 1000)) * 1000;

    ret = select(fd + 1, &sets, NULL, NULL, &timeout);
    if (ret > 0)
    {
        if (FD_ISSET(fd, &sets))
        {
            ret = recv(fd, buf, len, 0);
            if (ret > 0) {
                len_recv += ret;
            } else if (0 == ret) {
                printf("connection is closed\n");
                err_code = -1;
            } else {
                printf("recv fail: fd[%d] ret[%d] %s\n", fd, ret, strerror(errno));
                err_code = -2;
            }
        }
    } else if (0 == ret) {
//            printf("select-recv return 0\n");  //select timeout, nothing to do
    } else {
        //if (EINTR == errno) {
        //    continue;
        //}
        printf("select-recv fail: fd[%d] ret[%d] %s\n", fd, ret, strerror(errno));
        err_code = -2;
    }

    /* priority to return data bytes if any data be received from TCP connection. */
    /* It will get error code on next calling */
    return (0 != len_recv) ? len_recv : err_code;
}
