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
#include "string.h"
#include "mq2_sensor.h"
#include <stdio.h>

#define TCP_SERVER_SOCKET   0
#define TCP_SERVER_PORT     5000
#define BUFFER_SIZE         256

uint8_t buffer[BUFFER_SIZE];

// Global LED state
int led_state = 0;  // 0 = OFF, 1 = ON

// Sensor periodic sending
static uint32_t last_sensor_send = 0;
static uint8_t periodic_send_enabled = 1;  // 1 = ON, 0 = OFF

extern ADC_HandleTypeDef hadc1;

void tcp_server(void)
{
  int32_t ret;
  uint8_t status;
  uint16_t available_data;
  uint32_t current_time;
  char sensor_msg[100];

  // Initialize LED
  LED_init();
  LED_OFF();  // Start with LED off

  // Initialize MQ-2 Sensor
  if(MQ2_Init(&hadc1) != HAL_OK)
  {
    USART1_SendString("MQ-2 Init FAILED!\r\n");
  }
  USART1_SendString("MQ-2 Sensor Initialized\r\n");

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

  USART1_SendString("TCP Server with LED Control & Gas Sensor on port ");
  USART1_SendNumber(TCP_SERVER_PORT);
  USART1_SendString("\r\n");
  USART1_SendString("Commands: ON, OFF, TOGGLE, STATUS, GAS, START, STOP, HELP\r\n");
  USART1_SendString("Waiting for client...\r\n");

  last_sensor_send = HAL_GetTick();

  // Main loop
  while(1)
  {
    current_time = HAL_GetTick();
    status = getSn_SR(TCP_SERVER_SOCKET);

    // Send periodic sensor data every 5 seconds if client connected
    if(periodic_send_enabled && status == SOCK_ESTABLISHED)
    {
      if((current_time - last_sensor_send) >= 5000)  // 5 seconds
      {
        float voltage = MQ2_GetVoltage();
        float ppm = MQ2_GetPPM();
        const char *level = MQ2_GetLevelString();

        sprintf(sensor_msg, "[SENSOR] %.2fV | %.0fppm | %s\r\n", voltage, ppm, level);

        // Send to UART for debugging
        USART1_SendString(sensor_msg);

        // Send to TCP client
        send(TCP_SERVER_SOCKET, (uint8_t*) sensor_msg, strlen(sensor_msg));

        last_sensor_send = current_time;
      }
    }

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

            // LED Control
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
            // Gas sensor commands
            else if(strncmp((char*) buffer, "GAS", 3) == 0 || strncmp((char*) buffer, "gas", 3) == 0)
            {
              float voltage = MQ2_GetVoltage();
              float ppm = MQ2_GetPPM();
              const char *level = MQ2_GetLevelString();

              sprintf((char*) buffer, "Gas: %.2fV | %.0fppm | %s\r\n", voltage, ppm, level);
              USART1_SendString((char*) buffer);
            }
            else if(strncmp((char*) buffer, "START", 5) == 0 || strncmp((char*) buffer, "start", 5) == 0)
            {
              periodic_send_enabled = 1;
              last_sensor_send = HAL_GetTick();  // Reset timer
              strcpy((char*) buffer, "Periodic sensor sending STARTED (every 5 sec)\r\n");
              USART1_SendString((char*) buffer);
            }
            else if(strncmp((char*) buffer, "STOP", 4) == 0 || strncmp((char*) buffer, "stop", 4) == 0)
            {
              periodic_send_enabled = 0;
              strcpy((char*) buffer, "Periodic sensor sending STOPPED\r\n");
              USART1_SendString((char*) buffer);
            }
            else if(strncmp((char*) buffer, "HELP", 4) == 0 || strncmp((char*) buffer, "help", 4) == 0)
            {
              const char *help = "Commands:\r\n"
                  "  ON/OFF/TOGGLE - Control LED\r\n"
                  "  STATUS        - Show LED status\r\n"
                  "  GAS           - Get one gas reading\r\n"
                  "  START         - Start periodic sensor reading (every 5 sec)\r\n"
                  "  STOP          - Stop periodic sensor reading\r\n"
                  "  HELP          - Show this help\r\n";
              strncpy((char*) buffer, help, BUFFER_SIZE - 1);
              buffer[BUFFER_SIZE - 1] = '\0';  // Ensure null termination
              USART1_SendString((char*) buffer);
            }
            else
            {
              strcpy((char*) buffer, "Unknown command. Try: ON, OFF, TOGGLE, STATUS, GAS, START, STOP, HELP\r\n");
              USART1_SendString((char*) buffer);
            }

            // Send response back to client
            send(TCP_SERVER_SOCKET, buffer, strlen((char*) buffer));
          }
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
