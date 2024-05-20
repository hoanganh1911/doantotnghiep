#include <stdio.h>
#include <string.h>
#include "stdint.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/uart.h"

#include "esp_http_server.h"
#include "WiFiAP.h"
#include "WSServer.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "nvs_storage.h"
#include "cJSON.h"

#include "json.h"


#define WIFI_SSID      "Dev@"
#define WIFI_CHANNEL   1
#define WIFI_MAX_CONN  2
#define AP_LOCALIP     "192.168.1.1"
#define AP_NETMASK     "255.255.255.0"

#define UART1_BUF_SIZE (2048)


static void wifiap_eventhandler(wifi_event_t event, void *param);
static void ws_eventhandler(wsserver_event_t event, wsserver_data_t *pdata, void *param);

static void uart1_init(void);
static void uart_event_task(void *pvParameters);

static void uart_send(char *data);

static void ws_recv_task(void *pvParameters);

static void ws_new_cli(void *pvParameters);

EventGroupHandle_t event_group;

nvs_handle_t nvs;
nvs_data_t nvs_stored_data = {0};

static const char *TAG = "MAIN";
static QueueHandle_t uart1_queue;
static QueueHandle_t ws_recv_queue;
wifiap_t ap1 = {
	.ssid_prefix  = WIFI_SSID,
	.channel      = WIFI_CHANNEL,
	.maxconn      = WIFI_MAX_CONN,
	.eventhandler = wifiap_eventhandler,
	.eventparam   = NULL,
	.ip     = AP_LOCALIP,
	.netmsk = AP_NETMASK,
};
extern uint8_t index_html_start[] asm("_binary_index_html_start");
extern uint8_t index_html_end[] asm("_binary_index_html_end");
extern uint8_t style_css_start[] asm("_binary_style_css_start");
extern uint8_t style_css_end[] asm("_binary_style_css_end");
extern uint8_t script_js_start[] asm("_binary_script_js_start");
extern uint8_t script_js_end[] asm("_binary_script_js_end");
wsserver_t ws = {
	.ws_uri  = "/ws",
	.web_root_uri = "/",
	.web_style_uri = "/style.css",
	.web_script_uri = "/script.js",
	.web_html_start   = index_html_start,
	.web_html_end     = index_html_end,
	.web_style_start  = style_css_start,
	.web_style_end    = style_css_end,
	.web_script_start = script_js_start,
	.web_script_end   = script_js_end,
	.eventhandler = ws_eventhandler,
	.param = &ws,
	.max_clients = 10,
};

bool ws_start = false;
uint8_t ledc_state = 0;
extern bool _relwbnwcli;
typedef struct {
	char *data;
	uint8_t len;
}ws_recv_dt_t;


void app_main(void){
    nvs_init();
    nvs_read_stored_data();

    event_group = xEventGroupCreate();

    uart1_init();

    ws_recv_queue = xQueueCreate(10, sizeof(ws_recv_dt_t));

    wifi_init();
    wifiap_init(&ap1);
    wifiap_config_ip(&ap1);
    wifiap_start(&ap1);

    wsserver_init(&ws);

    xTaskCreate(ws_recv_task, "ws_receive_task", 9016, NULL, 11, NULL);
    xTaskCreate(ws_new_cli, "ws new client", 4096, NULL, 8, NULL);
}

static void ws_server_send_first_time()
{
	wsserver_data_t send_data;

	char *data_rak = nvs_create_lorawan_data();
	if(data_rak == NULL) return;
	if(ws_start == true){
		memset(send_data.data, 0, WS_MAX_DATA_LEN);
		memcpy(send_data.data, data_rak, strlen(data_rak));
		send_data.len = strlen(data_rak);
		wsserver_sendto_all(&ws, &send_data);
	}
	if(data_rak != NULL) free(data_rak);

}
static void ws_new_cli(void *pvParameters){
	EventBits_t bits;
	while(1)
	{
		bits = xEventGroupWaitBits(event_group, (1 << 0), pdTRUE, pdFALSE, portMAX_DELAY);
		if(bits & (1 << 0)){
			ws_server_send_first_time();
		}
	}
}


static void ws_recv_task(void *pvParameters){
	ws_recv_dt_t *rcv_data;
	while(1)
	{
		if (xQueueReceive(ws_recv_queue, (void *)&rcv_data, (TickType_t)portMAX_DELAY)) {
			if(rcv_data == NULL) continue;
			if(rcv_data->data == NULL) continue;

			rcv_data->data[rcv_data->len] = '\0';
			ESP_LOGI("Data","Recv data from queue: %s with len %d",rcv_data->data, strlen(rcv_data->data));

			ESP_LOGI("LOG HEAP","free size %ld",esp_get_free_heap_size());
			if(rcv_data->data == NULL) continue;

			json_t *dt_json = json_parse(rcv_data->data);

			if(json_get_object(dt_json, (char *)("shw_dev_rs485")))
			{

				ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs));

				size_t len;

				nvs_get_str(nvs, "mbdesc", NULL, &len);
				nvs_get_str(nvs, "mbdesc", nvs_stored_data.mb_desc, &len);
				//ESP_LOGI("DESC", "%s", nvs_stored_data.mb_desc);

				nvs_close(nvs);

				char *s_data;
				asprintf(&s_data, "{\"mbdesc\":%s}", nvs_stored_data.mb_desc);

				wsserver_data_t ws_s_data;

				memset(ws_s_data.data, 0, WS_MAX_DATA_LEN);
				memcpy(ws_s_data.data, s_data, strlen(s_data));
				ws_s_data.len = strlen(s_data);
				wsserver_sendto_all(&ws, &ws_s_data);

				free(s_data);
				goto EndnFree;
			}


			if(json_get_object(dt_json,(char *)"mbdesc"))
			{
				char *s_data;
				asprintf(&s_data,"%s",dt_json->value_string);
				ESP_LOGI("DESC","%s",s_data);

				ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs));
				ESP_ERROR_CHECK(nvs_set_str(nvs, "mbdesc", s_data));
				ESP_ERROR_CHECK(nvs_commit(nvs));

				free(s_data);
				goto EndnFree;
			}

			if(json_get_object(dt_json,(char *)"wan"))
			{

				char *appeui, *appkey;
				uint32_t period;
				json_t *dt_child = json_parse(dt_json->value_string);

				json_get_object(dt_child, (char*)"appeui");
				ESP_LOGI("DATA","appEUI: %s",dt_child->value_string);
				asprintf(&appeui,"%s",dt_child->value_string);
				json_get_object(dt_child, (char*)"appkey");
				ESP_LOGI("DATA","appKey: %s",dt_child->value_string);
				asprintf(&appkey,"%s",dt_child->value_string);
				json_get_object(dt_child, (char*)"period");
				ESP_LOGI("DATA","period: %ld",(uint32_t)dt_child->value_long);
				period = (uint32_t)dt_child->value_long;

				ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &nvs));

				ESP_ERROR_CHECK(nvs_set_str(nvs, "appeui", appeui));
				ESP_ERROR_CHECK(nvs_set_str(nvs, "appkey", appkey));
				ESP_ERROR_CHECK(nvs_set_u32(nvs, "period", period));

				ESP_ERROR_CHECK(nvs_commit(nvs));

				free(appeui);
				free(appkey);

//				if(dt_json) json_release(dt_json);
//
//				if(dt_child) json_release(dt_child);
//
//				if(rcv_data) free(rcv_data);

				while(1){
					//ESP_LOGI("AT","Try to reset !!");
					vTaskDelay(500/portTICK_PERIOD_MS);
					uart_tx_chars(UART_NUM_1, (const char *)"ATZ\r\n", 5);
					uart_wait_tx_done(UART_NUM_1,portMAX_DELAY);
				}
			}

			EndnFree:
				if(dt_json) json_release(dt_json);
				//if(rcv_data->data) free(rcv_data->data);
				if(rcv_data) free(rcv_data);

		}
	}
}

static void uart_event_task(void *pvParameters){
	uart_event_t event;
	while(1){
		if (xQueueReceive(uart1_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
			switch (event.type) {
				case UART_DATA:{
					char *rxdata = (char *)malloc(event.size*sizeof(char) + 1);
					uart_read_bytes(UART_NUM_1, rxdata, event.size, portMAX_DELAY);
					rxdata[event.size] = '\0';
					//ESP_LOGW(TAG, "%s", rxdata);

					if(rxdata[0] == '{' && rxdata[event.size - 1] == '}'){ // Is JSON

						cJSON *root = cJSON_Parse(rxdata);

						cJSON* deveui_json = cJSON_GetObjectItem(root, "deveui");
						if(deveui_json){

							char *deveui = cJSON_GetObjectItem(root, "deveui")->valuestring;
							nvs_storing_eui(deveui);

							char *json = nvs_create_rak_data();
							ESP_LOGI(TAG, "\r\n%s", json);
							uart_send(json);
							uint8_t endbyte = 0xFE;
							uart_write_bytes(UART_NUM_1, &endbyte, 1);
							free(json);
						}

						if(ws_start == true)
						{
							wsserver_data_t send_data;

							memset(send_data.data, 0, WS_MAX_DATA_LEN);
							memcpy(send_data.data, rxdata, strlen(rxdata));
							send_data.len = strlen(rxdata);

							wsserver_sendto_all(&ws, &send_data);
						}
						cJSON_Delete(root);
					}
					free(rxdata);
				}
				break;
				default:
					ESP_LOGI(TAG, "uart event type: %d", event.type);
				break;
			}
		}
	}
}



static void wifiap_eventhandler(wifi_event_t event, void *param){
    if (event == WIFI_EVENT_AP_STACONNECTED)
    	wsserver_start(&ws);
    else if (event == WIFI_EVENT_AP_STADISCONNECTED){
//    	wsserver_stop(&ws);
    }
}


static void ws_eventhandler(wsserver_event_t event, wsserver_data_t *pdata, void *param){
//	wsserver_t *pws = (wsserver_t *)param;

	switch(event){
		case WSSERVER_EVENT_START:
			ESP_LOGI(TAG, "Start");
		break;
		case WSSERVER_EVENT_STOP:
			ESP_LOGI(TAG, "Stop");
		break;
		case WSSERVER_EVENT_HANDSHAKE:
			ESP_LOGI(TAG, "Handshake, sockfd[%d]", pdata->clientid);
			ws_start = true;
			ws_server_send_first_time();
		break;
		case WSSERVER_EVENT_RECV:
			ESP_LOGI(TAG, "Receive \"%s\" with len %ld from sockfd[%d]", (char *)pdata->data,pdata->len, pdata->clientid);

			pdata->data[pdata->len] = '\0';

			ws_recv_dt_t *ws_recv_data = (ws_recv_dt_t *)malloc(sizeof(ws_recv_dt_t));
			if(ws_recv_data == NULL) break;

			ws_recv_data->data = (char *)(malloc(pdata->len * sizeof(char)));
			if(ws_recv_data->data == NULL) break;

			memcpy(ws_recv_data->data, pdata->data, pdata->len);
			ws_recv_data->len = pdata->len;
			xQueueSend(ws_recv_queue, (void *)&ws_recv_data, portMAX_DELAY);
		break;
		default:

		break;
	};
}




static void uart1_init(void){
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    uart_driver_install(UART_NUM_1, UART1_BUF_SIZE*2, UART1_BUF_SIZE*2, 20, &uart1_queue, 0);
    uart_param_config(UART_NUM_1, &uart_config);

    uart_set_pin(UART_NUM_1, GPIO_NUM_7, GPIO_NUM_6, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    xTaskCreate(uart_event_task, "uart_event_task", 8192, NULL, 3, NULL);
}


static void uart_send(char *data){
	char *x = data;
	uint16_t data_len = strlen(data);

	ESP_LOGI(TAG,"data len %d",data_len);

	uint8_t a = data_len/100;
	uint8_t b = data_len%100;

	for(uint8_t i=0; i<a; i++)
		uart_write_bytes(UART_NUM_1, (char *)(x + i*100), 100);
	if(b)
		uart_write_bytes(UART_NUM_1, (char *)(x + a*100), b);
}

//{"mbdata":{"SHT31 temperature": "3207","SHT31 humidity": "5200"}}






















