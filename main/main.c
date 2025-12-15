/* UART asynchronous example, that uses separate RX and TX tasks

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/



#include "main.h"
#include "esp_timer.h"


//#include "esp_http_client.h"
//#include "cJSON.h"
device_details dev;

char g_rx_buffer[256];
char L_rx_buffer[256];
char ec200_rx_buffer[256];
char sd[20000];

char *file_SD = MOUNT_POINT"/SD.txt";

static const int GPS_RX_BUF_SIZE = 128;
static const int LORA_RX_BUF_SIZE = 128;
static const int EC200_RX_BUF_SIZE = 128;
//static const int RS485_RX_BUF_SIZE = 128;

SemaphoreHandle_t myMutex = NULL;

#define CONFIG_GPS_TASK_STACK_SIZE 3072
#define CONFIG_LORA_TASK_STACK_SIZE 3072
#define CONFIG_EC200_TASK_STACK_SIZE 3072
#define CONFIG_RS485_TASK_STACK_SIZE 3072
#define CONFIG_SD_TASK_STACK_SIZE 3072

#define UART_BUF_SIZE 2048
#define AT_RESP_SIZE 512  // Max response buffer

#define AT_RESPONSE_BUF    256
#define EC200_PWR (23)
//#define GPIO_OUTPUT_PIN_SEL  (1ULL<<EC200_PWR)

#define RS485_PIN (27)


#define GPIO_OUTPUT_PIN_SEL  (1ULL<<RS485_PIN) | (1ULL<<EC200_PWR) | (1ULL<<RXER_PIN)

#define UART1_TX 12 //gps_tx
#define UART1_RX 13	//gps_rx
#define UART2_TX 6	//lora_tx
#define UART2_RX 5	//lora_rx
#define UART3_TX 4	//ec200
#define UART3_RX 3	//ec200
#define UART4_TX 10	//rs485
#define UART4_RX 11	//rs485

//static const char *TAG = "HTTP_JSON";
							
uart_port_t uart_ports[] = {UART_NUM_0, UART_NUM_1, UART_NUM_2, UART_NUM_3 };
int uart_tx_pins[] = {UART1_TX, UART2_TX,  UART3_TX,  UART4_TX};
int uart_rx_pins[] = {UART1_RX, UART2_RX, UART3_RX,UART4_RX};

#define THREAD_COUNT 5

// ---------------------- PACKETS -------------------------- //

PartialPacket pkt;             // actual definition
//QueueHandle_t partialQueue;
//QueueHandle_t fullQueue;

ring_buffer_t g_modbus_tcp_ring;
ring_buffer_t g_tx_ring;


bool scan_start = false;

const char *TAG = "EMI-EMC";

static uint32_t app_millis(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

uart_parity_t get_uart_parity(const char *str)
{
    if (str == NULL) {
        return UART_PARITY_DISABLE;
    }

    if (strcasecmp(str, "none") == 0)
        return UART_PARITY_DISABLE;

    if (strcasecmp(str, "even") == 0)
        return UART_PARITY_EVEN;

    if (strcasecmp(str, "odd") == 0)
        return UART_PARITY_ODD;

    ESP_LOGW("UART", "Invalid parity string: %s", str);
    return UART_PARITY_DISABLE;
}

// ================= UART Initialization =================
void init_uart(uart_port_t uart_num, int tx_pin, int rx_pin, int baud_rate, char* parity_str) {

	uart_parity_t par = get_uart_parity(parity_str);
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity    = par,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    ESP_ERROR_CHECK(uart_driver_install(uart_num, UART_BUF_SIZE*2, UART_BUF_SIZE*2, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    printf(" UART%d initialized (TX=%d, RX=%d)\n", uart_num + 1, tx_pin, rx_pin);
}


static void gps_rx_task(void *arg)
{
    printf("GPS task started\n");
    static const char *RX_TASK_TAG = "gps_RX_TASK";
    //esp_log_level_set("gps_RX_TASK", ESP_LOG_NONE);
   // esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(GPS_RX_BUF_SIZE + 1);
    //PartialPacket pkt;
    
    while (1) {
        PartialPacket pkt;
        const int rxBytes = uart_read_bytes(uart_ports[0], data, GPS_RX_BUF_SIZE, 50 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            //ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            //ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);

			snprintf(pkt.data, sizeof(pkt.data) - 1, "%s", data);
			pkt.thread_id = 1;
			ring_buffer_push(&g_modbus_tcp_ring, &pkt);
        
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
		
    }
    free(data);
	vTaskDelete(NULL);
}


void IMEI(uart_port_t port,const char *cmd, int timeout_ms) {
    uint8_t data[AT_RESP_SIZE];
    int len = 0;
    //char imei[20]; // IMEI is max 15 digits + null terminator
    char response[AT_RESP_SIZE] = {0};
    const char *p = response;
    int i = 0;
    // Send command with CRLF
    uart_write_bytes(port, cmd, strlen(cmd));
    //uart_write_bytes(UART_NUM_0, "\r\n", 2);

    // Read response
    TickType_t start_tick = xTaskGetTickCount();
    //char response[AT_RESP_SIZE] = {0};

    while ((xTaskGetTickCount() - start_tick) * portTICK_PERIOD_MS < timeout_ms) {
        len = uart_read_bytes(port, data, sizeof(data) - 1, pdMS_TO_TICKS(2000));
        if (len > 0) {
            data[len] = '\0';
            strcat(response, (char *)data);
        }
    }

   // printf("CMD: %s\n", cmd);
   // printf("RSP: %s\n", response);
    // Skip characters until we find the first digit
        while (*p && !isdigit((unsigned char)*p)) {
            p++;
        }

        // Copy digits to imei buffer
        while (*p && isdigit((unsigned char)*p) && i < sizeof(dev.gatewayID) - 1) {
        	dev.gatewayID[i++] = *p++;
        }

    dev.gatewayID[i] = '\0'; // Null terminate

    //printf("Parsed IMEI: %s\n", dev.gatewayID);
}

void parse_cclk_to_buffer(const char *src, char *out, size_t out_len)
{
	const char *p = src;
	    size_t i = 0;

	    printf("RAW INPUT = %s\n", src);

	    // 1. Skip until first digit
	    while (*p && !isdigit((unsigned char)*p)) {
	        printf("Skipping: %c\n", *p);
	        p++;
	    }

	    printf("First digit found at: '%c'\n", *p);

	    // 2. Copy digits and allowed separators
	    while (*p && i < out_len - 1) {

	       // printf("Processing: %c\n", *p);

	        if (isdigit((unsigned char)*p) || *p == '/' || *p == ',' || *p == ':') {
	            out[i++] = *p;
	            printf(" -> Copied '%c'\n", *p);
	        }
	        else if (*p == '+') {
	          //  printf("Encountered '+', stopping.\n");
	            break;          // Stop before timezone
	        }
	        else if (*p == '"') {
	          //  printf("Skipping quote\n");
	            p++;            // skip "
	            continue;
	        }
	        else {
	         //   printf("Ignoring character: %c\n", *p);
	        }

	        p++;
	    }

	    out[i] = '\0';
	    printf("FINAL PARSED TIME = %s\n", out);
}


void timed_AT(uart_port_t port,const char *cmd, int timeout_ms) {
    uint8_t data[AT_RESP_SIZE];
    int len = 0;
    char time_str[32];
    // Send command with CRLF
    uart_write_bytes(port, cmd, strlen(cmd));
    //uart_write_bytes(UART_NUM_0, "\r\n", 2);

    // Read response
    TickType_t start_tick = xTaskGetTickCount();
    char response[AT_RESP_SIZE] = {0};

    while ((xTaskGetTickCount() - start_tick) * portTICK_PERIOD_MS < timeout_ms) {
        len = uart_read_bytes(port, data, sizeof(data) - 1, pdMS_TO_TICKS(2000));
        if (len > 0) {
            data[len] = '\0';
            strcat(response, (char *)data);
        }
    }

    // printf("CMD: %s\n", cmd);
    //printf("RSP: %s\n", response);

    //parse_cclk_to_buffer(response, time_str, sizeof(time_str));
}
// Send AT command and read response
void sendAT(uart_port_t port,const char *cmd, int timeout_ms) {
    uint8_t data[AT_RESP_SIZE];
    int len = 0;

    // Send command with CRLF
    uart_write_bytes(port, cmd, strlen(cmd));
    //uart_write_bytes(UART_NUM_0, "\r\n", 2);

    // Read response
    TickType_t start_tick = xTaskGetTickCount();
    char response[AT_RESP_SIZE] = {0};

    while ((xTaskGetTickCount() - start_tick) * portTICK_PERIOD_MS < timeout_ms) {
        len = uart_read_bytes(port, data, sizeof(data) - 1, pdMS_TO_TICKS(2000));
        if (len > 0) {
            data[len] = '\0';
            strcat(response, (char *)data);
        }
    }
}

int sendData(uart_port_t port,const char* logName, const char* data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(port, data, len);
    //ESP_LOGI(logName, "Wrote %d bytes", txBytes);
        
    return txBytes;
}

static void lora_rx_task(void *arg)
{
    printf("LORA task started\n");
    static const char *RX_TASK_TAG = "LORA_RX_TASK";
    esp_log_level_set("RX_TASK_TAG", ESP_LOG_NONE);
   // PartialPacket pkt;
    
   // esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
        PartialPacket pkt;
    	//memset(L_rx_buffer, 0, sizeof(g_rx_buffer));
		uint8_t* data = (uint8_t*) malloc(LORA_RX_BUF_SIZE + 1);
        sendData(uart_ports[1],RX_TASK_TAG, "AT+QP2P=1\r\n");
        vTaskDelay(500 / portTICK_PERIOD_MS);
        sendData(uart_ports[1],RX_TASK_TAG, "AT+QTCONF=868001000:14:4:12:4/5:0:0:1:16:25000:2:3\r\n");
        vTaskDelay(500 / portTICK_PERIOD_MS);
        sendData(uart_ports[1],RX_TASK_TAG, "AT+QTRX=1\r\n");
        vTaskDelay(500 / portTICK_PERIOD_MS);
        const int rxBytes = uart_read_bytes(uart_ports[1], data, LORA_RX_BUF_SIZE, 2000 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
            //ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            //printf("Read lora data : '%s'",L_rx_buffer);
            //ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
            pkt.thread_id = 2;
            snprintf(pkt.data, sizeof(pkt.data), "%s", data);

            ring_buffer_push(&g_modbus_tcp_ring, &pkt);

	    }
	    free(data);
	    vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    
}


static void lora_tx_task(void *arg)
{
    static const char *TX_TASK_TAG = "LORA_TX_TASK";
    esp_log_level_set("TX_TASK_TAG", ESP_LOG_NONE);
   // esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
		if (xSemaphoreTake(myMutex, portMAX_DELAY) == pdTRUE) {
	        
	    	sendAT(uart_ports[1],"AT+QP2P=1\r\n", 500);
	    	sendAT(uart_ports[1],"AT+QTCONF=868000000:14:4:12:4/5:0:0:1:16:25000:2:3\r\n", 500);
	    	sendAT(uart_ports[1],"AT+QTDA=Hello I am KG200Z\r\n", 500);
	    	sendAT(uart_ports[1],"AT+QTTX=1\r\n", 500);
	    	uart_flush(uart_ports[1]);
	        //vTaskDelay(2000 / portTICK_PERIOD_MS);
	        xSemaphoreGive(myMutex);
	   }
	   vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void gpio_init_u(void)
{
	vTaskDelay(2000 / portTICK_PERIOD_MS);
	//printf("GPIO Config init\n");
	gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    //interrupt of rising edge
    io_conf.mode = GPIO_MODE_OUTPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
    
    
    gpio_set_level(EC200_PWR, 1);
    printf("GPIO HIGH\n");
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    gpio_set_level(EC200_PWR, 0);
    printf("GPIO LOW\n");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
}

static void ec200_network()
{

  // === SIM & Network ===
    sendAT(uart_ports[2],CMD_ECHO_OFF, 1000);             
    sendAT(uart_ports[2],CMD_CHECK_SIM, 1000);       
    sendAT(uart_ports[2],CMD_NETWORK_REG, 1000);
    sendAT(uart_ports[2],CMD_NETWORK_PDP_CFG, 1000);
   // sendAT(UART_NUM_2,CMD_ACTIVATE_PDP, 2000);
    sendAT(uart_ports[2],"AT+CPIN?\r\n",1000);
    sendAT(uart_ports[2],"AT+CFUN=1\r\n", 1000);
    sendAT(uart_ports[2],"AT+COPS?\r\n", 1000);
    sendAT(uart_ports[2],"AT+CSQ\r\n", 1000);
    sendAT(uart_ports[2],"AT+CGATT=1\r\n", 1000);
    sendAT(uart_ports[2],"AT+QIACT?\r\n", 1000);
    sendAT(uart_ports[2], "AT+QHTTPCFG=\"contextid\",1\r\n", 1000);
    sendAT(uart_ports[2], "AT+QHTTPCFG=\"requestheader\",1\r\n", 1000);
    sendAT(uart_ports[2], "AT+QHTTPCFG=\"responseheader\",1\r\n", 1000);
    sendAT(uart_ports[2],"AT+CGDCONT=1,\"IP\",\"airtelgprs.com\"\r\n",1000);
    sendAT(uart_ports[2],CMD_ACTIVATE_PDP, 1000);
    sendAT(uart_ports[2],"AT+CGACT=1,1\r\n", 1000);
    sendAT(uart_ports[2],"AT+CGPADDR=1\r\n", 1000);
    IMEI(uart_ports[2],"AT+GSN\r\n", 1000);
    //timed_AT(UART_NUM_2,"AT+CCLK?\r\n",1000);
    printf("EC200 Done\n");

}

static void ec200_tx_task(void *arg)
{
    static const char *TX_TASK_TAG = "ec200_TX_TASK";
    esp_log_level_set("TX_TASK_TAG", ESP_LOG_NONE);
   // esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
        sendData(uart_ports[2],TX_TASK_TAG, CMD_PING);
        //sendAT(CMD_PING,100);
        ESP_LOGI(TX_TASK_TAG, "Sent Ping req");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

static void ec200_rx_task(void *arg)
{
    printf("EC200 task started\n");
    static const char *RX_TASK_TAG = "ec200_RX_TASK";
    esp_log_level_set("RX_TASK_TAG", ESP_LOG_NONE);
   // esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(EC200_RX_BUF_SIZE + 1);

    //PartialPacket pkt;
    

    while (1) {

        PartialPacket pkt;

        sendData(uart_ports[2],RX_TASK_TAG, CMD_PING);
        //sendAT(CMD_PING,100);
        //ESP_LOGI(RX_TASK_TAG, "Sent Ping req");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        const int rxBytes = uart_read_bytes(uart_ports[2], data, EC200_RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
            //ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            //memcpy(ec200_rx_buffer, data, rxBytes);
           //	printf("Read gps data : '%s'",ec200_rx_buffer);
            //ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
			snprintf(pkt.data, sizeof(pkt.data), "%s\n%s",CMD_PING, data);
            pkt.thread_id = 4;
            ring_buffer_push(&g_modbus_tcp_ring, &pkt);
        }    
		vTaskDelay(2000 / portTICK_PERIOD_MS);
		
    }
    free(data);
}

// void i2cThread(void *arg)
// {
//     while(1)
//     {
//         i2c_detect();
//     }
// }
// ---------------------- COMBINER THREAD -------------------- //

void CombinerThread(void *arg)
{
    printf("Combine task started\n");
    PartialPacket pkt;
    //char buffer[THREAD_COUNT][2056];
    char *buff_th1 = NULL;
	char *buff_th2 = NULL;
	char *buff_th3 = NULL;
	char *buff_th4 = NULL;
	char *buff_th5 = NULL;
	char *buff_th6 = NULL;
	char *buff_th7 = NULL;

    buff_th1 = (char *)malloc(600 * sizeof(char));
    buff_th2 = (char *)malloc(600 * sizeof(char));
    buff_th3 = (char *)malloc(600 * sizeof(char));
    buff_th4 = (char *)malloc(200 * sizeof(char));
    buff_th5 = (char *)malloc(200 * sizeof(char));
    buff_th6 = (char *)malloc(600 * sizeof(char));
    buff_th7 = (char *)malloc(200 * sizeof(char));

    snprintf(buff_th7,200, "i2c Address of TA101 detected at : \n0x17\n");
	
	
    int received_mask = 0;

    //memset(buffer, 0, sizeof(buffer));

    while (1)
    {
        
		if (!ring_buffer_pop(&g_modbus_tcp_ring, &pkt)) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        

    	// Optional: Check if allocation was successful
    	if (buff_th1 == NULL || buff_th2 == NULL || buff_th3 == NULL || buff_th4 == NULL || buff_th5 == NULL) {
        	// Handle error (e.g., print error message, exit program)
        	free(buff_th1); free(buff_th2); free(buff_th3); free(buff_th4); free(buff_th5);
        	
        	continue;
    	}
        if(pkt.thread_id == 3)
        {
            pkt.thread_id = 0;
            snprintf(buff_th7,200, "i2c Address of TA101 detected at : \n0x17\n");
            //ESP_LOGI("Partial", "%s",buff_th6);
        }
        else if(pkt.thread_id == 32)
        {
            snprintf(buff_th6,600, "WiFi Scan list :\n%s\n", pkt.data);
            //ESP_LOGI("Partial", "%s",buff_th6);
        }
        else if(pkt.thread_id == 16)
        {
            snprintf(buff_th5,200, "MODBUS RTU DATA:\n%f\n", pkt.rtu_data);
            //ESP_LOGI("Partial", "%s",buff_th5);
        }
        else if(pkt.thread_id == 8)
        {
            snprintf(buff_th4,200, "MODBUS TCP DATA:\n%f\n", pkt.tcp_data);
            //ESP_LOGI("Partial", "%s",buff_th4);
        }
        else if(pkt.thread_id == 4)
        {
            snprintf(buff_th3,600, "EC200U DATA:\n%s\n", pkt.data);
            //ESP_LOGI("Partial", "%s",buff_th3);
        }
        else if(pkt.thread_id == 2)
        {
            snprintf(buff_th2,600, "LORA DATA:\n%s\n", pkt.data);
            //ESP_LOGI("Partial", "%s",buff_th2);
        }
        else if(pkt.thread_id == 1)
        {
            snprintf(buff_th1,600, "GPS DATA:\n%s\n", pkt.data);
            //ESP_LOGI("Partial", "%s",buff_th1);
        }

        received_mask |= (pkt.thread_id);

        if (received_mask == 0x3f)    // 0b11111 = 5 threads 0x1f
        {
            CombinedPacket combined;

            snprintf(combined.full_line, sizeof(combined.full_line),
                        "\nTimestamp: %ldms\n%s%s%s%s%s%s%s\nReset Counter: %ld",
                        app_millis(),buff_th1, buff_th2, buff_th3, buff_th4, buff_th5, buff_th6 ,buff_th7,reset_ctr++);
            //snprintf(combined.full_line, sizeof(combined.full_line),"\nTimestamp: %ldms\n%s%s\n",app_millis(),buff_th4,buff_th5);
            //ESP_LOGI("Combining", "%s", combined.full_line);
            
            if (strlen(combined.full_line) >= sizeof(combined.full_line)) {
                ESP_LOGI("Combining", "BUFFER OVERFLOW DETECTED!");
            }

            // Reset mask
            received_mask = 0;

            // Send result to SD writer
            //xQueueSend(fullQueue, &combined, portMAX_DELAY);
            ring_buffer_push(&g_tx_ring, &combined);
            
            // free(buff_th1);
            // free(buff_th2);
            // free(buff_th3);
            // free(buff_th4);
            // free(buff_th5); 
        }
        //printf("End of Combiner\n");
        vTaskDelay(150 / portTICK_PERIOD_MS);
    }
}

// ---------------------- SD WRITER THREAD -------------------- //

void SDWriterThread(void *arg)
{
    //scan_start = true;
    printf("SD Card task started\n");
    CombinedPacket combined;

    while (1)
    {
        if (!ring_buffer_pop(&g_tx_ring, &combined)) {
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }
        combined.full_line[strlen(combined.full_line)-1] = '\0';
        ESP_LOGI("SDWriter", "%s", combined.full_line);
        int ret = s_example_write_file(file_SD, combined.full_line);
        if (ret != ESP_OK) {
            vTaskDelay(1000);
            continue;;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
// volatile unsigned tick_count;
// void vApplicationTickHook(void)
// {
//     tick_count++;
// }

void app_main(void)
{
    unsigned char voltage1[] = {0x01, 0x03, 0x0B, 0xD3, 0x00, 0x02, 0x37, 0xD6}; 
    //esp_log_level_set("*", ESP_LOG_WARN);

    
    i2c_main();

    init_uart(uart_ports[0], uart_tx_pins[0], uart_rx_pins[0],9600,"none");
	init_uart(uart_ports[1], uart_tx_pins[1], uart_rx_pins[1],115200,"none");
	init_uart(uart_ports[2], uart_tx_pins[2], uart_rx_pins[2],115200, "none");
	init_uart(uart_ports[3], uart_tx_pins[3], uart_rx_pins[3],19200,"even");

    

    ring_buffer_init(&g_modbus_tcp_ring, RING_BUFFER_SIZE, sizeof(PartialPacket));
    ring_buffer_init(&g_tx_ring, RING_BUFFER_SIZE, sizeof(CombinedPacket));

	// myMutex = xSemaphoreCreateMutex();
	// if (myMutex == NULL) {
	//     ESP_LOGE("MAIN", "Failed to create  mymutex!");
	//     while(1);
	// }
    

	xTaskCreate(gps_rx_task, "gps_rx_task", 3001, NULL, configMAX_PRIORITIES - 2, NULL);
	
	xTaskCreate(lora_rx_task, "lora_rx_task", 3001, NULL, configMAX_PRIORITIES - 1, NULL);
    //xTaskCreate(lora_tx_task, "lora_tx_task", CONFIG_LORA_TASK_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
				
    //xTaskCreate(ec200_tx_task, "ec200_tx_task", CONFIG_EC200_TASK_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
 	
	
	xTaskCreate(CombinerThread, "Combiner", 8001, NULL, 6, NULL);
	xTaskCreate(SDWriterThread, "SDWriter", 8001, NULL, 4, NULL);

    wifi_main();
    eth();
    sd_card();

    //TODO:add the EC200 reset gpio and toggle it in the begining
	gpio_init_u();
    ec200_network();
    xTaskCreate(ec200_rx_task, "ec200_rx_task", 3001, NULL, configMAX_PRIORITIES - 2, NULL);

    while(1)
	{
        gpio_set_level(RS485_PIN, 1);

        esp_rom_delay_us(500);
        
        int result = uart_write_bytes(uart_ports[3], voltage1, sizeof(voltage1));
        //if (result == sizeof(voltage1))
        //{
            //ESP_LOGI("Phase 1 ", "Wrote Voltage %d bytes", result);
        //}
        result = 0;
        
        esp_rom_delay_us(5000);
        
        gpio_set_level(RS485_PIN, 0);
        
        volt_value1 = read_response();
        
        if(volt_value1 > 0.0 && volt_value1 < 270.0){

            pkt.thread_id = 16;
            pkt.rtu_data  = volt_value1;
            volt_value1 = 0.0;

            //if (xSemaphoreTake(myMutex, portMAX_DELAY) == pdTRUE) {


            //xQueueSend(partialQueue, &pkt, portMAX_DELAY);
            ring_buffer_push(&g_modbus_tcp_ring, &pkt);

            //xSemaphoreGive(myMutex);
            //}
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
	
}
