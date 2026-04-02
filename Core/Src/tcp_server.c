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
#include "gpio.h"

#define TCP_SERVER_SOCKET   0
#define TCP_SERVER_PORT     5000
#define BUFFER_SIZE         256

uint8_t buffer[BUFFER_SIZE];

// Global LED state
int led_state = 0;  // 0 = OFF, 1 = ON

void tcp_server(void)
{
  int32_t ret;
  uint8_t status;
  uint16_t available_data;

  // Initialize LED
  LED_init();
  LED_OFF();  // Start with LED off

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

  USART1_SendString("TCP Server with LED Control on port ");
  USART1_SendNumber(TCP_SERVER_PORT);
  USART1_SendString("\r\n");
  USART1_SendString("Commands: ON, OFF, TOGGLE, STATUS\r\n");
  USART1_SendString("Waiting for client...\r\n");

  // Main loop
  while(1)
  {
    status = getSn_SR(TCP_SERVER_SOCKET);

    switch(status)
    {
      case SOCK_ESTABLISHED:
        // Check for data
        available_data = getSn_RX_RSR(TCP_SERVER_SOCKET);

        if(available_data > 0)
        {
          ret = recv(TCP_SERVER_SOCKET, buffer, (available_data < BUFFER_SIZE) ? available_data : BUFFER_SIZE);

          if(ret > 0)
          {
            buffer[ret] = '\0';
            USART1_SendString("Received: ");
            USART1_SendString((char*) buffer);
            USART1_SendString("\r\n");

            // Remove newline characters for comparison
            for(int i = 0; i < ret; i++)
            {
              if(buffer[i] == '\n' || buffer[i] == '\r')
              {
                buffer[i] = '\0';
                break;
              }
            }

            // Parse command and control LED
            if(strncmp((char*) buffer, "ON", 2) == 0 || strncmp((char*) buffer, "on", 2) == 0)
            {
              LED_ON();
              led_state = 1;
              strcpy((char*) buffer, "LED Turned ON\r\n");
              USART1_SendString("LED Turned ON\r\n");
            }
            else if(strncmp((char*) buffer, "OFF", 3) == 0 || strncmp((char*) buffer, "off", 3) == 0)
            {
              LED_OFF();
              led_state = 0;
              strcpy((char*) buffer, "LED Turned OFF\r\n");
              USART1_SendString("LED Turned OFF\r\n");
            }
            else if(strncmp((char*) buffer, "TOGGLE", 6) == 0 || strncmp((char*) buffer, "toggle", 6) == 0)
            {
              LED_Toggle();
              led_state = !led_state;
              strcpy((char*) buffer, led_state ? "LED Toggled ON\r\n" : "LED Toggled OFF\r\n");
              USART1_SendString((char*) buffer);
            }
            else if(strncmp((char*) buffer, "STATUS", 6) == 0 || strncmp((char*) buffer, "status", 6) == 0)
            {
              if(led_state)
                strcpy((char*) buffer, "LED is ON\r\n");
              else
                strcpy((char*) buffer, "LED is OFF\r\n");
              USART1_SendString((char*) buffer);
            }
            else
            {
              strcpy((char*) buffer, "Unknown command. Try: ON, OFF, TOGGLE, STATUS\r\n");
              USART1_SendString((char*) buffer);
            }

            // Send response back to client
            send(TCP_SERVER_SOCKET, buffer, strlen((char*) buffer));
          }
        }

        static uint32_t last_blink = 0;
        if(HAL_GetTick() - last_blink > 1000)
        {
          // Just a heartbeat indicator
          last_blink = HAL_GetTick();
          USART1_SendString(".");
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

    HAL_Delay(10);
  }
}
