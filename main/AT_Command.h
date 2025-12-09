/*
 * AT_Command.h
 *
 *  Created on: 05-Nov-2025
 *      Author: lenovo
 */

#ifndef MAIN_AT_COMMAND_H_
#define MAIN_AT_COMMAND_H_


#define NULL_					'\0'
#define SERVER_ACK				">"
#define GPS_ACK					"+QGPSLOC:"

#define CMD_ECHO_OFF            "ATI\r\n"
#define CMD_CHECK_SIM           "AT+CPIN?\r\n"
#define CMD_NETWORK_REG         "AT+CREG=2\r\n"
#define CMD_NETWORK_PDP_CFG		"AT+CGREG=0\r\n"
#define CMD_ACTIVATE_PDP        "AT+QIACT=1\r\n"
#define CMD_CHECK_PDP			"AT+QIACT?"
#define CMD_SET_APN             "AT+QICSGP=1,1,\"airtelgprs.com\","","",1\r\n"
#define CMD_NETWORK_ACTIVATE	"ATV=1\r\n"
#define CMD_GPS_CFG_PORT		"AT+QGPSCFG=\"outport\",\"uart1\"\r\n"
#define CMD_GPS_CFG_GNSS_CFG	"AT+QGPSCFG=\"gnssconfig\",0\r\n"
#define CMD_GPS_CFG_GPS			"AT+QGPSCFG=\"gpsnmeatype\",0\r\n"
#define CMD_GPS_CFG_GLONASS		"AT+QGPSCFG=\"glonassnmeatype\",0\r\n"
#define CMD_GPS_CFG_GALILEO		"AT+QGPSCFG=\"galileonmeatype\",0\r\n"
#define CMD_GPS_CFG_BID_T		"AT+QGPSCFG=\"beidounmeatype\",0\r\n"
#define CMD_GPS_CFG_BID_F		"AT+QGPSCFG=\"beidounmeaformat\",0\r\n"
#define CMD_GPS_CFG_GNSS		"AT+QGPSCFG=\"gnssnmeatype\",0\r\n"
#define CMD_GPS_CFG_GPS_A		"AT+QGPSCFG=\"autogps\",0\r\n"
#define CMD_GPS_MODE_ON         "AT+QGPS=1\r\n"
#define CMD_GPS_MODE_OFF		"AT+QGPSEND\r\n"
#define CMD_NETOPEN             "AT+NETOPEN\r\n"
#define RESPONSE_NETOPEN_OK     "+NETOPEN: 0"

#define CMD_OPEN_SOCKET_FORMAT  "AT+QIOPEN=1,%d,\"TCP\",\"%s\",%d,0,0\r\n"
#define RESPONSE_SOCKET_OPEN_OK "+QIPOPEN: 0,0"
#define CMD_PING				"AT+QPING=1,\"www.google.com\"\r\n"

#define CMD_SEND_DATA_FORMAT			"AT+QISEND=%d,%d\r\n"
#define CMD_RECEIVE_DATA_FORMAT		"AT+QIRD=%d,%d\r\n"

#define CMD_GPS_DATA			"AT+QGPSLOC=2\r\n"

#define CMD_GPS_CHECK			"AT+QGPS?\r\n"
#define CMD_GPS_CHECK_R_T		"+QGPS: 1"
#define CMD_GPS_CHECK_R_F		"+QGPS: 0"
#define ERROR_GPS_FIX			"+CME ERROR: 516"
#define ERROR_GPS_ACTIVE		"+CME ERROR: 504"
#define CHECK_DOT			    '.'
#define CHECK_COMMA				','


#endif /* MAIN_AT_COMMAND_H_ */
