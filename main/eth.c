/*
 * eth.c
 *
 *  Created on: Dec 2, 2025
 *      Author: amit
 */

/*
 * SPDX-FileCopyrightText: 2018-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "eth.h"

#include "main.h"
#include "ring_buffer.h"

//#define RXER_PIN (2)
//#define GPIO_OUTPUT_PIN_SEL  (1ULL<<RXER_PIN)

//#define MODBUS_PORT 502

//static const char *TAG = "eth_example";

static esp_eth_handle_t *s_eth_handles = NULL;
static uint8_t s_eth_port_cnt = 0;
bool s_eth_link_up = false;
int sock_t = 0;

char modbus_rx_buffer[256];

#if CONFIG_EXAMPLE_STORE_HISTORY

#define MOUNT_PATH "/data"
#define HISTORY_PATH MOUNT_PATH "/history.txt"

void initialize_filesystem(void)
{
    static wl_handle_t wl_handle;
    const esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 4,
        .format_if_mount_failed = true
    };
    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(MOUNT_PATH, "storage", &mount_config, &wl_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return;
    }
}
#endif // CONFIG_EXAMPLE_STORE_HISTORY

/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        s_eth_link_up = true;
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        s_eth_link_up = false;
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
}


void init_ethernet_and_netif(void)
{
    // Init TCP/IP stack first
    ESP_ERROR_CHECK(esp_netif_init());

    // Then the default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Init Ethernet driver (from your helper)
    ESP_ERROR_CHECK(example_eth_init(&s_eth_handles, &s_eth_port_cnt));

    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
    esp_netif_config_t cfg_spi = {
        .base = &esp_netif_config,
        .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH
    };

    char if_key_str[10];
    char if_desc_str[10];
    char num_str[3];

    for (int i = 0; i < s_eth_port_cnt; i++) {
        itoa(i, num_str, 10);
        strcat(strcpy(if_key_str, "ETH_"), num_str);
        strcat(strcpy(if_desc_str, "eth"), num_str);
        esp_netif_config.if_key = if_key_str;
        esp_netif_config.if_desc = if_desc_str;
        esp_netif_config.route_prio -= i * 5;

        esp_netif_t *eth_netif = esp_netif_new(&cfg_spi);

        // Attach Ethernet driver to TCP/IP stack
        ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(s_eth_handles[i])));

        // Stop DHCP client before assigning static IP
        ESP_ERROR_CHECK(esp_netif_dhcpc_stop(eth_netif));

        // Assign static IP
        esp_netif_ip_info_t ip_info;
        ESP_ERROR_CHECK(esp_netif_str_to_ip4("192.168.137.2", &ip_info.ip));   // ESP32-P4 IP
        ESP_ERROR_CHECK(esp_netif_str_to_ip4("192.168.137.1", &ip_info.gw));   // PC as gateway
        ESP_ERROR_CHECK(esp_netif_str_to_ip4("255.255.255.0", &ip_info.netmask));
        ESP_ERROR_CHECK(esp_netif_set_ip_info(eth_netif, &ip_info));
    }

    // Register user defined event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    for (int i = 0; i < s_eth_port_cnt; i++) {
        ESP_ERROR_CHECK(esp_eth_start(s_eth_handles[i]));
    }


}

int modbus_connect(const char *ip) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(502);
    inet_aton(ip, &dest.sin_addr);

    if (connect(sock, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
        printf("MODBUS connect failed\n");
        return -1;
    }
    return sock;
}

int modbus_write_single(const char *ip, uint16_t addr, uint16_t value) {
    int sock = modbus_connect(ip);
    if (sock < 0) return -1;

    uint8_t frame[12];
    frame[0] = 0x00; frame[1] = 0x01; // Transaction ID
    frame[2] = 0x00; frame[3] = 0x00; // Protocol ID
    frame[4] = 0x00; frame[5] = 0x06; // Length
    frame[6] = 0x01;                 // Unit ID (slave address)
    frame[7] = 0x06;                 // Function code
    frame[8] = addr >> 8;
    frame[9] = addr & 0xFF;
    frame[10] = value >> 8;
    frame[11] = value & 0xFF;

    send(sock, frame, 12, 0);

    uint8_t response[260];
    recv(sock, response, sizeof(response), 0);

    close(sock);
    return 0;
}

bool eth_is_link_ready(void)
{
    if(s_eth_link_up && (sock_t == 0))
    {
        sock_t = modbus_connect("192.168.137.1");
    }
    else if(!s_eth_link_up)
    {
        close(sock_t);
        sock_t = 0;
    }
    return s_eth_link_up;
}

void modbus_tcp_server_task(void *pvParameters)
{

    //sock_t = modbus_connect("192.168.137.1");
    uint16_t addr = 1;
    uint16_t value = 2;

    float modbus = 0.0f;
    uint32_t raw = 0;

    while(1)
    {
        eth_is_link_ready();
    	memset(modbus_rx_buffer, 0, sizeof(modbus_rx_buffer));
        uint8_t frame[12];
        frame[0] = 0x00; frame[1] = 0x01; // Transaction ID
        frame[2] = 0x00; frame[3] = 0x00; // Protocol ID
        frame[4] = 0x00; frame[5] = 0x06; // Length
        frame[6] = 0x01;                 // Unit ID (slave address)
        frame[7] = 0x03;                 // Function code
        frame[8] = addr >> 8;
        frame[9] = addr & 0xFF;
        frame[10] = value >> 8;
        frame[11] = value & 0xFF;

        send(sock_t, frame, 12, 0);

        uint8_t response[260];
        int len = recv(sock_t, response, sizeof(response), 0);

        // if (len > 0) {
        //     printf("Received %d bytes:\n", len);
        //     for (int i = 0; i < len; i++) {
        //         printf("%02X ", response[i]);
        //     }
        //     printf("\n");
        // }

        raw = ( (uint32_t)response[9] << 24 )
                 | ( (uint32_t)response[10] << 16 )
                 | ( (uint32_t)response[11] << 8  )
                 | ( (uint32_t)response[12] );

        memcpy(&modbus, &raw, sizeof(modbus));
       // memcpy(modbus_rx_buffer, &raw, sizeof(raw));
        //ESP_LOGI("MODBUS TCP/IP", "Extracted float value = %f", modbus);

        //snprintf(pkt.data, sizeof(pkt.data), "%f", modbus);
		pkt.thread_id = 8;
        pkt.tcp_data  = modbus;
        //if (xSemaphoreTake(myMutex, portMAX_DELAY) == pdTRUE) {

        //xQueueSend(partialQueue, &pkt, portMAX_DELAY);
        ring_buffer_push(&g_modbus_tcp_ring, &pkt);

        //xSemaphoreGive(myMutex);
        //}

        vTaskDelay(pdMS_TO_TICKS(2000));

    }

}



void eth(void)
{
	//gpio_init_u();
	gpio_set_level(RXER_PIN, 1);
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    //esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
#if CONFIG_EXAMPLE_STORE_HISTORY
    initialize_filesystem();
    repl_config.history_save_path = HISTORY_PATH;
#endif
    repl_config.prompt = "iperf>";
    // init console REPL environment
    //ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));

    // init Ethernet and netif
    init_ethernet_and_netif();

    /* Register commands */
    register_system_common();
    app_register_iperf_commands();
    register_ethernet_commands();



    xTaskCreate(modbus_tcp_server_task, "modbus_tcp", 4096, NULL, 8, NULL);

   
}

