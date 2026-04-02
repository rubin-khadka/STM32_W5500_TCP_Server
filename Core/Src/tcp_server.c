/*
 * tcp_server.c
 *
 *  Created on: Apr 2, 2026
 *      Author: Rubin Khadka
 */

#include "main.h"
#include "socket.h"
#include "wizchip_conf.h"
#include "usart1.h"
#include "tcp_server.h"

#define BUFFER_SIZE         256

uint8_t buffer[BUFFER_SIZE];

void tcp_server(void)
{
  int32_t ret;
  uint8_t status;

  // Create socket
  if(socket(TCP_SERVER_SOCKET, Sn_MR_TCP, TCP_SERVER_PORT, 0) != TCP_SERVER_SOCKET)
  {
    USART1_SendString("Socket open failed\r\n");
    return;
  }

  // Start listening
  if(listen(TCP_SERVER_SOCKET) != SOCK_OK)
  {
    USART1_SendString("Listen failed\r\n");
    close(TCP_SERVER_SOCKET);
    return;
  }

  USART1_SendString("TCP Echo Server running on port ");
  USART1_SendNumber(TCP_SERVER_PORT);
  USART1_SendString("\r\n");
  USART1_SendString("Waiting for client...\r\n");

  // Main loop
  while(1)
  {
    status = getSn_SR(TCP_SERVER_SOCKET);

    switch(status)
    {
      case SOCK_ESTABLISHED:
        USART1_SendString("Client connected!\r\n");

        // Keep echoing while connected
        while(getSn_SR(TCP_SERVER_SOCKET) == SOCK_ESTABLISHED)
        {
          // BLOCKING: Wait for data
          ret = recv(TCP_SERVER_SOCKET, buffer, BUFFER_SIZE);

          if(ret > 0)
          {
            // Print received data
            buffer[ret] = '\0';
            USART1_SendString("Received (");
            USART1_SendNumber(ret);
            USART1_SendString(" bytes): ");
            USART1_SendString((char*) buffer);
            USART1_SendString("\r\n");

            // ECHO BACK: Send same data to client
            send(TCP_SERVER_SOCKET, buffer, ret);
            USART1_SendString("Echoed back\r\n");
          }
          else if(ret == SOCKERR_TIMEOUT)
          {
            // can add timeout here
          }
        }
        break;

      case SOCK_CLOSE_WAIT:
        USART1_SendString("Client disconnected\r\n");
        disconnect(TCP_SERVER_SOCKET);
        break;

      case SOCK_CLOSED:
        USART1_SendString("Waiting for new client...\r\n");
        if(socket(TCP_SERVER_SOCKET, Sn_MR_TCP, TCP_SERVER_PORT, 0) == TCP_SERVER_SOCKET)
        {
          listen(TCP_SERVER_SOCKET);
        }
        break;

      default:
        break;
    }

    HAL_Delay(10);
  }
}
