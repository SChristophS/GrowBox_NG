#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include <stdint.h>
#include "socket.h"

typedef struct {
    int8_t sock;
    const char *server_ip;
    uint16_t server_port;
    uint16_t local_port;
} SocketNetwork;

void socket_network_init(SocketNetwork *n, const char *server_ip, uint16_t server_port, uint16_t local_port);
void socket_connect(SocketNetwork *n);
void socket_send(SocketNetwork *n, const uint8_t *data, uint16_t len);
void socket_receive(SocketNetwork *n, uint8_t *buffer, uint16_t len);
void socket_loop(SocketNetwork *n);

#endif // TCP_CLIENT_H
