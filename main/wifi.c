#include "wifi.h"


extern bool ip_attach;
extern bool scan_done;
extern char all_scan_results[512];

#define SERVER_IP "ec2-35-154-171-134.ap-south-1.compute.amazonaws.com"
#define SERVER_PORT "41234"


void wifi_task(void *pvParameters)
{
    // const char *prefix = "Node Online 03";
    // const char *json_str =
    //     "{\n"
    //     "\"device\":\"abb9\",\n"
    //     "\"chip\":  \"xG28\",\n"
    //     "\"parent\":\"abb2\",\n"
    //     "\"running\":     \"  0-00:03:03\",\n"
    //     "\"connected\":   \"  0-00:02:09\",\n"
    //     "\"disconnected\":\"no\",\n"
    //     "\"connections\": \"1\",\n"
    //     "\"availability\":\"100.00\",\n"
    //     "\"connected_total\":   \"  0-00:02:09\",\n"
    //     "\"disconnected_total\":\"  0-00:00:00\",\n"
    //     "\"Wisun_Data\":\"WiSUN-Board-1\",\n"
    //     "\"neighbor_info\": {\"rsl_in\":-39,\"rsl_out\":-37,\"is_lfn\":0}\n"
    //     "}";
    
    // char buff[1024];  // make sure this is large enough
    // snprintf(buff, sizeof(buff), "%s%s", prefix, json_str);

    // int sock;
    // //char buff[20] = "Hello from P4";
    // struct addrinfo hints, *res;
    // int ret;
    printf("In wifi task\n");

    

    while(1)
    {
        if(scan_done) 
        {
            PartialPacket pkt;
            pkt.thread_id = 32;

            //printf("sending to ring_buffer\n%s\n",all_scan_results);
            snprintf(pkt.data, sizeof(pkt.data), "%s",all_scan_results);
            scan_done = false;
            ring_buffer_push(&g_modbus_tcp_ring, &pkt);
            memset(all_scan_results, 0, sizeof(all_scan_results));
            wifi_scan_config_t scan_config = {0};
            esp_err_t err = esp_wifi_scan_start(&scan_config, false);
        }

        // if(ip_attach && scan_start) 
        // {
        //     //printf("Ip address received");

        //     memset(&hints, 0, sizeof(hints));
        //     hints.ai_family = AF_UNSPEC;
        //     hints.ai_socktype = SOCK_DGRAM;

        //     ret = getaddrinfo(SERVER_IP, SERVER_PORT, &hints, &res);
        //     if (ret != 0 || res == NULL) {
        //         printf("getaddr cmd fail: %d\n",ret);
        //         vTaskDelete(NULL);
        //         return;
        //     }

        //     sock = socket(res->ai_family, res->ai_socktype, 0);
        //     if (sock < 0) {
        //         printf("socket error\n");
        //         freeaddrinfo(res);
        //         vTaskDelete(NULL);
        //         return;
        //     }

        //     ret = sendto(sock, buff, strlen(buff), 0, res->ai_addr, res->ai_addrlen);
        //     if (ret < 0) {
        //         printf("sendto failed\n");
        //     } else {
        //         //printf("message sent success : %s\n",buff);
        //     }
        //     close(sock);
        //     freeaddrinfo(res);
        // }
        
        vTaskDelay(pdMS_TO_TICKS(5000));
        
    }
    
    vTaskDelete(NULL);
}

void wifi_main(void)
{
#if defined(CONFIG_ESP_EXT_CONN_ENABLE) && defined(CONFIG_ESP_HOST_WIFI_ENABLED)
    esp_extconn_config_t ext_config = ESP_EXTCONN_CONFIG_DEFAULT();
    esp_extconn_init(&ext_config);
#endif

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    /*
    NOTE(#15855): wifi_cmd_initialize_wifi is a basic function to start wifi, set handlers and set wifi-cmd status.
    For advanced usage, please refer to wifi_cmd.h or the document of wifi-cmd component: https://components.espressif.com/components/esp-qa/wifi-cmd

    example:
        wifi_cmd_wifi_init();
        my_function();  // <---- more configs before wifi start
        wifi_cmd_wifi_start();

    Please note that some wifi commands such as "wifi start/restart" may not work as expected if "wifi_cmd_initialize_wifi" was not used.
    */
    /* initialise wifi and set wifi-cmd status */
    wifi_cmd_initialize_wifi(NULL);
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

#if CONFIG_ESP_WIFI_ENABLE_WIFI_RX_STATS
#if CONFIG_ESP_WIFI_ENABLE_WIFI_RX_MU_STATS
    esp_wifi_enable_rx_statistics(true, true);
#else
    esp_wifi_enable_rx_statistics(true, false);
#endif
#endif
#if CONFIG_ESP_WIFI_ENABLE_WIFI_TX_STATS
    esp_wifi_enable_tx_statistics(ESP_WIFI_ACI_BE, true);
#endif

    wifi_scan_config_t scan_config = {0};
    esp_err_t err = esp_wifi_scan_start(&scan_config, false);

    xTaskCreate(wifi_task, "wifi_task", 4096, NULL, 5, NULL);
}
