/*
 * WSServer.h
 *
 *  Created on: May 8, 2024
 *      Author: anh
 */

#ifndef MAIN_WSSERVER_WSSERVER_H_
#define MAIN_WSSERVER_WSSERVER_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "stdint.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"



#define WS_MAX_DATA_LEN 512

typedef enum{
	WSSERVER_EVENT_START,
	WSSERVER_EVENT_STOP,
	WSSERVER_EVENT_INIT_UI,
	WSSERVER_EVENT_HANDSHAKE,
	WSSERVER_EVENT_SENT,
	WSSERVER_EVENT_RECV,
} wsserver_event_t;

typedef struct{
	uint8_t data[WS_MAX_DATA_LEN];
	uint32_t len;
	int clientid;
} wsserver_data_t;

typedef struct {
	const char *ws_uri;
	const char *web_root_uri;
	const char *web_style_uri;
	const char *web_script_uri;
	uint8_t *web_html_start;
	uint8_t *web_html_end;
	uint8_t *web_style_start;
	uint8_t *web_style_end;
	uint8_t *web_script_start;
	uint8_t *web_script_end;
	void (*eventhandler)(wsserver_event_t event, wsserver_data_t *pdata, void *param);
	void *param;
	uint8_t max_clients;
	/**
	 * Private members.
	 */
	httpd_handle_t httpd;
	uint8_t nb_client;
	QueueHandle_t queue_sockfd;

	httpd_uri_t ws;
	httpd_uri_t web_html;
	httpd_uri_t web_style;
	httpd_uri_t web_script;
} wsserver_t;



void wsserver_init(wsserver_t *pws);

void wsserver_start(wsserver_t *pws);
void wsserver_stop(wsserver_t *pws);


void wsserver_sendto_sock(wsserver_t *pws, wsserver_data_t *pdata);
void wsserver_sendto_all(wsserver_t *pws, wsserver_data_t *pdata);


#ifdef __cplusplus
}
#endif

#endif /* MAIN_WSSERVER_WSSERVER_H_ */
