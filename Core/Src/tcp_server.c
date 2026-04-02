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

#define TCP_SERVER_SOCKET   0
#define TCP_SERVER_PORT     5000
#define BUFFER_SIZE         256

uint8_t buffer[BUFFER_SIZE];

void tcp_server(void)
{
  int32_t ret;
  uint8_t status;
  uint16_t available_data;

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

  USART1_SendString("Non-blocking TCP Echo Server on port ");
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
        // Check how many bytes are waiting
        available_data = getSn_RX_RSR(TCP_SERVER_SOCKET);

        if(available_data > 0)
        {
          // Only read if data is available
          ret = recv(TCP_SERVER_SOCKET, buffer, (available_data < BUFFER_SIZE) ? available_data : BUFFER_SIZE);

          if(ret > 0)
          {
            // Print received data
            buffer[ret] = '\0';
            USART1_SendString("Received (");
            USART1_SendNumber(ret);
            USART1_SendString(" bytes): ");
            USART1_SendString((char*) buffer);
            USART1_SendString("\r\n");

            // ECHO BACK
            send(TCP_SERVER_SOCKET, buffer, ret);
            USART1_SendString("Echoed back\r\n");
          }
        }

        static int counter = 0;
        counter++;
        if(counter >= 100)
        {
          counter = 0;
          USART1_SendString("Other Tasks \r\n");  // Show that CPU is still alive
        }

        break;

      case SOCK_CLOSE_WAIT:
        USART1_SendString("\r\nClient disconnected\r\n");
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

    // Small delay to prevent CPU hogging
    HAL_Delay(10);
  }
}
