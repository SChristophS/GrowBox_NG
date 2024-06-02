#include "tcp_client.h"
#include <stdio.h> // Für printf
#include "uart_redirect.h"
#include <string.h>  // Für memcpy

int TCP_Connect(uint8_t sock, uint8_t *ip, uint16_t port) {
    int ret;
    uint8_t destip[4];
    uint16_t destport;

    memcpy(destip, ip, 4);
    destport = port;

    printf("TCP_Connect: Opening socket %d\r\n", sock);
    ret = socket(sock, Sn_MR_TCP, 5000, 0);
    if (ret != sock) {
        printf("TCP_Connect: Failed to open socket %d\r\n", sock);
        return -1;
    }

    printf("TCP_Connect: Connecting to %d.%d.%d.%d:%d\r\n", destip[0], destip[1], destip[2], destip[3], destport);
    ret = connect(sock, destip, destport);
    if (ret != SOCK_OK) {
        printf("TCP_Connect: Failed to connect to %d.%d.%d.%d:%d, ret = %d\r\n", destip[0], destip[1], destip[2], destip[3], destport, ret);
        return -1;
    }

    printf("TCP_Connect: Successfully connected to %d.%d.%d.%d:%d\r\n", destip[0], destip[1], destip[2], destip[3], destport);
    return 0;
}

int TCP_Send(uint8_t sock, uint8_t *data, uint16_t len) {
    int ret;
    printf("TCP_Send: Sending data on socket %d\r\n", sock);
    ret = send(sock, data, len);
    if (ret < 0) {
        printf("TCP_Send: Failed to send data on socket %d, ret = %d\r\n", sock, ret);
        return -1;
    }
    printf("TCP_Send: Successfully sent data on socket %d, len = %d\r\n", sock, len);
    return ret;
}

int TCP_Receive(uint8_t sock, uint8_t *buf, uint16_t len) {
    int ret;
    printf("TCP_Receive: Receiving data on socket %d\r\n", sock);
    ret = recv(sock, buf, len);
    if (ret < 0) {
        printf("TCP_Receive: Failed to receive data on socket %d, ret = %d\r\n", sock, ret);
        return -1;
    }
    printf("TCP_Receive: Successfully received data on socket %d, len = %d\r\n", sock, ret);
    return ret;
}

void TCP_Close(uint8_t sock) {
    printf("TCP_Close: Closing socket %d\r\n", sock);
    disconnect(sock);
    close(sock);
    printf("TCP_Close: Successfully closed socket %d\r\n", sock);
}

int TCP_CheckConnection(uint8_t sock) {
    uint8_t status = getSn_SR(sock);
    printf("TCP_CheckConnection: Socket %d status = %d\r\n", sock, status);
    if (status == SOCK_ESTABLISHED) {
        printf("TCP_CheckConnection: Socket %d is established\r\n", sock);
        return 1; // Verbindung besteht
    } else {
        printf("TCP_CheckConnection: Socket %d is not established\r\n", sock);
        return 0; // Verbindung besteht nicht
    }
}
