/*
 * eth.h
 *
 *  Created on: Dec 2, 2025
 *      Author: amit
 */

#include <string.h>
#include <stdint.h>
#include "freertos/idf_additions.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_event.h"
#include "esp_eth.h"
#include "esp_netif.h"
#include "ethernet_init.h"
#include "esp_vfs_fat.h"
#include "cmd_system.h"
#include "cmd_ethernet.h"
#include "esp_netif_ip_addr.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include "iperf_cmd.h"

#ifndef ETH_H_
#define ETH_H_

#define RXER_PIN (2)
#define MODBUS_PORT 502

extern char modbus_rx_buffer[256];
//extern PartialPacket pkt;
//extern QueueHandle_t partialQueue;     // 5 threads → combiner
//extern QueueHandle_t fullQueue;        // combiner → SD writer

void initialize_filesystem(void);
void init_ethernet_and_netif(void);
int modbus_connect(const char *ip);
int modbus_write_single(const char *ip, uint16_t addr, uint16_t value);
void modbus_tcp_server_task(void *pvParameters);
void eth(void);

#endif /* ETH_H_ */
