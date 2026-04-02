/*
 * tcp_server.c
 *
 *  Created on: Apr 2, 2026
 *      Author: Rubin Khadka
 */

#include "main.h"
#include "socket.h"
#include "wizchip_conf.h"
#include "stdio.h"
#include "string.h"

#define TCP_SERVER_SOCKET   0
#define TCP_SERVER_PORT     5000

uint8_t rx_buf[512];
uint8_t tx_buf[512];

int ledState = 0;
extern void led_Control(int value);

void tcp_server(void)
{
  int32_t ret;
  uint8_t status;

  // Step 1: Open socket in TCP mode
  if(socket(TCP_SERVER_SOCKET, Sn_MR_TCP, TCP_SERVER_PORT, 0) != TCP_SERVER_SOCKET)
  {
    printf("Socket open failed\r\n");
    return;
  }

  // Step 2: Listen for incoming connections
  if(listen(TCP_SERVER_SOCKET) != SOCK_OK)
  {
    printf("Socket listen failed\r\n");
    close(TCP_SERVER_SOCKET);
    return;
  }

  printf("TCP Server listening on port %d\r\n", TCP_SERVER_PORT);

  // Step 3: Loop forever
  while(1)
  {
    status = getSn_SR(TCP_SERVER_SOCKET);

    switch(status)
    {
      case SOCK_ESTABLISHED:
        if(getSn_IR(TCP_SERVER_SOCKET) & Sn_IR_CON)
        {
          printf("Client connected!\r\n");
          setSn_IR(TCP_SERVER_SOCKET, Sn_IR_CON);  // clear flag
        }

        // Step 4: Receive data
        ret = recv(TCP_SERVER_SOCKET, rx_buf, sizeof(rx_buf));
        if(ret > 0)
        {
          rx_buf[ret] = '\0';
          printf("Received (%ld bytes): %s\r\n", ret, rx_buf);

          if(strncmp((char*) rx_buf, "ON", 2) == 0)
          {
            ledState = 0;
          }
          else if(strncmp((char*) rx_buf, "OFF", 3) == 0)
          {
            ledState = 1;
          }
          led_Control(ledState);
          int len = sprintf((char*) tx_buf, "LED Turned %s\r\n", ledState ? "OFF" : "ON");
          send(TCP_SERVER_SOCKET, tx_buf, len);

//                // Step 5: Echo data back
//                send(TCP_SERVER_SOCKET, rx_buf, ret);
//                printf("Echoed back\r\n");
        }
        break;

      case SOCK_CLOSE_WAIT:
        printf("Client disconnected\r\n");
        disconnect(TCP_SERVER_SOCKET);
        break;

      case SOCK_CLOSED:
        printf("Socket closed, reopening...\r\n");
        if(socket(TCP_SERVER_SOCKET, Sn_MR_TCP, TCP_SERVER_PORT, 0) == TCP_SERVER_SOCKET)
        {
          listen(TCP_SERVER_SOCKET);
          printf("TCP Server listening on port %d\r\n", TCP_SERVER_PORT);
        }
        break;

      default:
        break;
    }

    HAL_Delay(10);
  }
}

