/*
 * meter.h
 *
 *  Created on: 06-Nov-2025
 *      Author: lenovo
 */

#ifndef MAIN_METER_H_
#define MAIN_METER_H_

#include <stdint.h>
#include "sd_card.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"


     // modified for new meter (PM1000-H) //3000

extern float volt_value1, curr_value, powf_value, act_value, app_value;


#define ONE 1
#define TWO 2
#define NEG_ONE -1
#define NEG_TWO -2

#define RS485_PIN (27)
#define GPIO_OUTPUT_PIN_SEL  (1ULL<<RS485_PIN) | (1ULL<<EC200_PWR)
extern const char *filePath;


static const char *TAG = "MODBUS_READER"; // static is okay here
#define MAX_JSON_SIZE 3072
extern char rs485_rx_buffer[MAX_JSON_SIZE];

extern char serial_info_block[512]; // extern means declaration, not definition

typedef struct {
    float pb;
    float Tb;
    float VmT;
    float VbT;
    float Qb;
    float Batt_R;
} flow_data;

typedef struct
{
    char gatewayID[32];
    char f_version[32];
    char timestamp[32];
} device_details;

extern device_details dev;
extern flow_data flow; // extern declaration



//bool get_json_value(const char *json, const char *key, char *value, size_t max_length);
char *extract_str_value(const char *json, const char *key);
unsigned short modbus_crc16(const unsigned char *buf, int len);
uint16_t bytes_to_uint16(uint8_t high, uint8_t low);
unsigned int bytes_to_uint32(unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3);
int setup_communication_from_json(char *serial_info_block);
int write_and_receive_response(int serial_port, unsigned char *frame, size_t frame_size, char *datatype, char *label);
int extract_serial_info_block(const char *json, char *out_buf, size_t buf_size);
int extract_int_value(const char *json, const char *key);
int read_file(const char *filename, char *buffer);
float getvalue(char *string);
float read_response();
float combine_to_float_via_string(int real_value, int dec_value);
void create_modbus_frames(const char *json);
void assign_float_field(flow_data *flow, char *label, float value);
void parse_and_create_frame();
void send_data_to_server();
void IMEI(uart_port_t port,const char *cmd, int timeout_ms);
void sendAT(uart_port_t port,const char *cmd, int timeout_ms);

#endif /* MAIN_METER_H_ */

