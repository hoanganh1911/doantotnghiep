/*
 * WiFiAP.h
 *
 *  Created on: May 8, 2024
 *      Author: anh
 */

#ifndef MAIN_WIFIAP_WIFIAP_H_
#define MAIN_WIFIAP_WIFIAP_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "esp_wifi.h"




typedef struct{
	const char *ssid_prefix;
	uint8_t channel;
	uint8_t maxconn;
	void (*eventhandler)(wifi_event_t event, void *param);
	void *eventparam;

	const char *ip;
	const char *netmsk;

	/**
	 * Private members.
	 */
	esp_netif_t *netif;
	wifi_config_t wifi_config;
} wifiap_t;


void wifi_init(void);

void wifiap_init(wifiap_t *pap);

void wifiap_config_ip(wifiap_t *pap);

void wifiap_start(wifiap_t *pap);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_WIFIAP_WIFIAP_H_ */
