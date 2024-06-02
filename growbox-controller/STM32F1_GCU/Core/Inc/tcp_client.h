#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__

#include "socket.h"
#include "stm32f1xx_hal.h"

// Stellt eine TCP-Verbindung zu einem Server her
int TCP_Connect(uint8_t sock, uint8_t *ip, uint16_t port);

// Sendet Daten über eine TCP-Verbindung
int TCP_Send(uint8_t sock, uint8_t *data, uint16_t len);

// Empfängt Daten über eine TCP-Verbindung
int TCP_Receive(uint8_t sock, uint8_t *buf, uint16_t len);

// Schließt die TCP-Verbindung
void TCP_Close(uint8_t sock);

// Überprüft, ob die TCP-Verbindung besteht
int TCP_CheckConnection(uint8_t sock);

#endif // __TCP_CLIENT_H__
