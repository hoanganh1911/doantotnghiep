/*
 * WSServer.c
 *
 *  Created on: May 8, 2024
 *      Author: anh
 */

#include "WSServer.h"

#include "string.h"

#include "esp_event.h"
#include "esp_system.h"
#include "esp_log.h"




struct async_send_arg {
	wsserver_t *pws;
	wsserver_data_t *pdata;
	bool all_client;
};

static const char *TAG = "WSServer";


static esp_err_t ws_handler(httpd_req_t *req);
static esp_err_t web_handler(httpd_req_t *req);
static void config_data_to_send(struct async_send_arg *parg, wsserver_t *pws, wsserver_data_t *pdata);
static void ws_async_send(void *arg);




void wsserver_init(wsserver_t *pws){
	pws->web_html.uri = pws->web_root_uri;
	pws->web_html.method = HTTP_GET;
	pws->web_html.user_ctx = (void *)pws;
	pws->web_html.handler = web_handler;

	pws->web_style.uri = pws->web_style_uri;
	pws->web_style.method = HTTP_GET;
	pws->web_style.user_ctx = (void *)pws;
	pws->web_style.handler = web_handler;

	pws->web_script.uri = pws->web_script_uri;
	pws->web_script.method = HTTP_GET;
	pws->web_script.user_ctx = (void *)pws;
	pws->web_script.handler = web_handler;


	pws->ws.uri = pws->ws_uri;
	pws->ws.method = HTTP_GET;
	pws->ws.user_ctx = (void *)pws;
	pws->ws.is_websocket = true;
	pws->ws.handler = ws_handler;


	pws->nb_client = 0;
	pws->httpd = NULL;


	if(pws->max_clients == 0){
		ESP_LOGE(TAG, "Invalid maximum number of clients.");
		return;
	}
	pws->queue_sockfd = xQueueCreate(pws->max_clients, sizeof(uint32_t));
}



void wsserver_start(wsserver_t *pws){
	if(pws->httpd == NULL){
		httpd_config_t config = HTTPD_DEFAULT_CONFIG();

		ESP_LOGI(TAG, "Starting server on port: %d", config.server_port);
		if (httpd_start(&pws->httpd, &config) == ESP_OK) {
			ESP_LOGI(TAG, "Registering URI handlers");
			httpd_register_uri_handler(pws->httpd, &pws->ws);

			httpd_register_uri_handler(pws->httpd, &pws->web_html);
			httpd_register_uri_handler(pws->httpd, &pws->web_style);
			httpd_register_uri_handler(pws->httpd, &pws->web_script);

			if(pws->eventhandler) pws->eventhandler(WSSERVER_EVENT_START, NULL, pws->param);

			return;
		}

		ESP_LOGE(TAG, "Starting server failed!");
	}
}

void wsserver_stop(wsserver_t *pws){
	if(pws->httpd != NULL){
		httpd_stop(pws->httpd);
		ESP_LOGI(TAG, "WebSocket server disconnected!");
		if(pws->eventhandler) pws->eventhandler(WSSERVER_EVENT_STOP, NULL, pws->param);
	}
}



static esp_err_t web_handler(httpd_req_t *req){

	wsserver_t *pws = NULL;

	if (req->user_ctx != NULL) {
		pws = (wsserver_t *)req->user_ctx;
	}

	esp_err_t ret = ESP_OK;

	if (req->method == HTTP_GET) {
		if(pws->web_html_start != NULL && pws->web_html_end != NULL && strcmp(req->uri, pws->web_root_uri) == 0){
			const uint32_t index_html_len = pws->web_html_end - pws->web_html_start;
			ret = httpd_resp_send(req, (char *)pws->web_html_start, index_html_len);
			if(ret != ESP_OK) ESP_LOGE(TAG, "Error send web html.");
		}

		if(pws->web_style_start != NULL && pws->web_style_end != NULL && strcmp(req->uri, pws->web_style_uri) == 0){
			const uint32_t style_len = pws->web_style_end - pws->web_style_start;
			ret = httpd_resp_send(req, (char *)pws->web_style_start, style_len);
			if(ret != ESP_OK) ESP_LOGE(TAG, "Error send web style.");
		}

		if(pws->web_script_start != NULL && pws->web_script_end != NULL && strcmp(req->uri, pws->web_script_uri) == 0){
			const uint32_t script_len = pws->web_script_end - pws->web_script_start;
			ret = httpd_resp_send(req, (char *)pws->web_script_start, script_len);
			if(ret != ESP_OK) ESP_LOGE(TAG, "Error send web script.");
		}
	}

	return ret;
}

static esp_err_t ws_handler(httpd_req_t *req){
	wsserver_t *pws = (wsserver_t *)req->user_ctx;
	wsserver_data_t evdata;;

    if (req->method == HTTP_GET) {
        int sockfd = httpd_req_to_sockfd(req);
        ESP_LOGI(TAG, "New client sockfd[%d]", sockfd);
        //ws_server_send_first_time();

        UBaseType_t queue_nb_item = uxQueueMessagesWaiting(pws->queue_sockfd);
        while(queue_nb_item--){
        	int sock_in_queue = 0;
        	if(xQueueReceive(pws->queue_sockfd, &sock_in_queue, 10) == pdTRUE){
				if(sockfd == sock_in_queue) {
					if(xQueueSend(pws->queue_sockfd, (void *)&sock_in_queue, 10) != pdTRUE)
						ESP_LOGE(TAG, "Queue socket send failed.");
					ESP_LOGE(TAG, "Duplicated socket, number clients connected: %d", pws->nb_client);

					return ESP_OK;
				}
        	}
        }

		if(xQueueSend(pws->queue_sockfd, (void *)&sockfd, 10) != pdTRUE)
			ESP_LOGE(TAG, "Queue socket send failed.");

		evdata.clientid = sockfd;
		pws->nb_client++;
		ESP_LOGI(TAG, "Number Client connected: %d", pws->nb_client);

		if(pws->eventhandler) pws->eventhandler(WSSERVER_EVENT_HANDSHAKE, &evdata, pws->param);

        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }

    if (ws_pkt.len > 0) {
    	evdata.len = ws_pkt.len;
        ws_pkt.payload = evdata.data;
        evdata.data[evdata.len] = 0;

        esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            return ret;
        }
        evdata.clientid = httpd_req_to_sockfd(req);
        if(pws->eventhandler) pws->eventhandler(WSSERVER_EVENT_RECV, &evdata, pws->param);
    }

    return ret;
}

void wsserver_sendto_sock(wsserver_t *pws, wsserver_data_t *pdata){
	struct async_send_arg *arg = (struct async_send_arg *)malloc(sizeof(struct async_send_arg));

	config_data_to_send(arg, pws, pdata);
	arg->all_client = false;

	ESP_ERROR_CHECK(httpd_queue_work(pws->httpd, ws_async_send, (void *)arg));
}

void wsserver_sendto_all(wsserver_t *pws, wsserver_data_t *pdata){
	struct async_send_arg *arg = (struct async_send_arg *)malloc(sizeof(struct async_send_arg));

	config_data_to_send(arg, pws, pdata);
	arg->all_client = true;

	ESP_ERROR_CHECK(httpd_queue_work(pws->httpd, ws_async_send, (void *)arg));
}



static void config_data_to_send(struct async_send_arg *parg, wsserver_t *pws, wsserver_data_t *pdata){
	parg->pws = pws;
	parg->pdata = (wsserver_data_t *)malloc(sizeof(wsserver_data_t));
	parg->pdata->clientid = pdata->clientid;
	parg->pdata->len = pdata->len;
	memcpy(parg->pdata->data, pdata->data, parg->pdata->len);
	parg->pdata->data[parg->pdata->len] = '\0';
}

static void ws_async_send(void *arg){
	struct async_send_arg *send_arg = (struct async_send_arg *)arg;

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
	ws_pkt.payload = (uint8_t*)send_arg->pdata->data;
	ws_pkt.len = send_arg->pdata->len;
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    if(send_arg->all_client == false)
		httpd_ws_send_frame_async(send_arg->pws->httpd, send_arg->pdata->clientid, &ws_pkt);
    else{
        UBaseType_t queue_nb_item = uxQueueMessagesWaiting(send_arg->pws->queue_sockfd);
        while(queue_nb_item--){
        	int sock_in_queue = 0;
        	if(xQueueReceive(send_arg->pws->queue_sockfd, &sock_in_queue, 10) == pdTRUE){
				if(xQueueSend(send_arg->pws->queue_sockfd, (void *)&sock_in_queue, 10) != pdTRUE)
					ESP_LOGE(TAG, "Queue socket send failed.");

				httpd_ws_send_frame_async(send_arg->pws->httpd, sock_in_queue, &ws_pkt);
        	}
        }
    }

	if(send_arg->pdata != NULL) free(send_arg->pdata);
	if(send_arg != NULL) free(send_arg);
}

