/*
 * modbus.c
 *
 *  Created on: Nov 18, 2025
 *      Author: amit
 */
 #include<string.h>
 #include<stdio.h>
 #include "hal/gpio_types.h"
 #include "driver/gpio.h"
 #include "esp_rom_sys.h"
 #include "hal/uart_types.h"
 #include "driver/uart.h"

 #include"portmacro.h"
 #include "esp_log.h"
 #include <errno.h>
 #include <stdlib.h>
 #include <unistd.h>
 #include <stdint.h>
 #include "sd_card.h"
 //#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_netif.h"

#include "main.h"
#include <math.h>
 #include "meter_emi_emc.h"

char serial_info_block[512];
char rs485_rx_buffer[MAX_JSON_SIZE];

flow_data flow;

float volt_value1, curr_value, powf_value, act_value, app_value = 0;

 
 /**
 * @brief Converts a hexadecimal string to a float value.
 *
 * This function parses a hexadecimal string, extracts the four characters,
 * and converts them into a float value.
 *
 * @param string The input string in hexadecimal format.
 * @return float The converted float value.
 */
float getvalue(char *string)
{
    unsigned int num;
    float f;
    sscanf(string, "%x", &num);
    f = *((float *)&num); // Convert the integer representation to float
    return f;
}
 /**
 * @brief Reads a response from the serial port.
 *
 * This function reads data from the serial port, parses it, and converts it
 * into a float value using the `getvalue()` function.
 *
 * @return float The value read from the serial port.
 */
float regs_to_float(uint16_t reg_hi, uint16_t reg_lo)
{
    uint32_t raw;
    float value;

    // Word-swapped order (most common in Modbus)
    raw = ((uint32_t)reg_hi << 16) | (uint32_t)reg_lo;

    printf("\nRaw 32-bit: 0x%08lX\n", raw);

    memcpy(&value, &raw, sizeof(value));
    printf("Decoded float value: %.3f\n", value);

    return value;
}
void reverse(char* str, int len)
{
    int i = 0, j = len - 1, temp;
    while (i < j) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++;
        j--;
    }
}

// Converts a given integer x to string str[].
// d is the number of digits required in the output.
// If d is more than the number of digits in x,
// then 0s are added at the beginning.
int intToStr(int x, char str[], int d)
{
    int i = 0;
    while (x) {
        str[i++] = (x % 10) + '0';
        x = x / 10;
    }

    // If number of digits required is more, then
    // add 0s at the beginning
    while (i < d)
        str[i++] = '0';

    reverse(str, i);
    str[i] = '\0';
    return i;
}

// Converts a floating-point/double number to a string.
void ftoa(float n, char* res, int afterpoint)
{
    // Extract integer part
    int ipart = (int)n;

    // Extract floating part
    float fpart = n - (float)ipart;

    // convert integer part to string
    int i = intToStr(ipart, res, 0);

    // check for display option after point
    if (afterpoint != 0) {
        res[i] = '.'; // add dot

        // Get the value of fraction part upto given no.
        // of points after dot. The third parameter
        // is needed to handle cases like 233.007
        fpart = fpart * pow(10, afterpoint);

        intToStr((int)fpart, res + i + 1, afterpoint);
    }
}


float read_response()
{
    char buf[32];
    int point = 0;
    unsigned char string[10];
    float value;
    unsigned char read_buf[1024];
    uint32_t raw = 0;

    memset(read_buf, 0, sizeof(read_buf));

    int num_bytes = uart_read_bytes(uart_ports[3], &read_buf, sizeof(read_buf),1000 / portTICK_PERIOD_MS); // reading the hexadecimal values from the registers.
   // printf(" reading: %s\n",read_buf);
    if (num_bytes < 0)
    {
        printf("Error reading: %s\n","something error at reading responce");
    }
    for(int i=0;i<num_bytes;i++){

        //printf(" raw_data :");
        //printf(" %02X  ",read_buf[i]);
        //printf("\n");
        if(read_buf[i] == 4)
            point = i;
    }
    //printf("\n");

    //uint16_t reg_hi = (read_buf[5] << 8) | read_buf[6];
    //uint16_t reg_lo = (read_buf[3] << 8) | read_buf[4];
    //value = regs_to_float(reg_lo, reg_hi);

    //ftoa(value,buf,6);
    //printf("value before scaling : %s\n", buf);
    //memset(&buf,0,sizeof(buf));
    raw = ( (uint32_t)read_buf[point+1] << 24 )
                 | ( (uint32_t)read_buf[point+2] << 16 )
                 | ( (uint32_t)read_buf[point+3] << 8  )
                 | ( (uint32_t)read_buf[point+4] );

    memcpy(&value, &raw, sizeof(value));

    //ESP_LOGI("MODBUS RTU", "Extracted float value = %f", value);

    return value;
}

