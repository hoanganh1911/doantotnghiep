/*
 * WiFiAP.c
 *
 *  Created on: May 8, 2024
 *      Author: anh
 */


#include "WiFiAP.h"

#include "string.h"

#include "esp_mac.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"


static const char *TAG = "WiFiAP";



static void wifiap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);



void wifi_init(void){
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}


void wifiap_init(wifiap_t *pap){
    ESP_LOGW(TAG, "WIFI MODE AP");

    pap->netif = esp_netif_create_default_wifi_ap();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiap_event_handler, (void *)pap, NULL));

	uint8_t mac_addr[6];
    ESP_ERROR_CHECK(esp_read_mac(mac_addr, ESP_MAC_EFUSE_FACTORY));

    uint8_t ssid_len = 12 + strlen(pap->ssid_prefix);
    char *pass = (char *)malloc(12 + 1);
    char *ssid = (char *)malloc(ssid_len + 1);

    sprintf(pass, "%02x%02x%02x%02x%02x%02x"
		, mac_addr[0]
		, mac_addr[1]
		, mac_addr[2]
		, mac_addr[3]
		, mac_addr[4]
		, mac_addr[5]);
    pass[12] = '\0';
    strupr(pass);
    sprintf(ssid, "%s%s", pap->ssid_prefix, pass);
    ssid[ssid_len] = '\0';


    memcpy(pap->wifi_config.ap.ssid, ssid, ssid_len);
    memcpy(pap->wifi_config.ap.password, "12345678", 8);
    pap->wifi_config.ap.ssid_len = ssid_len;
    pap->wifi_config.ap.channel  = pap->channel;
    pap->wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    pap->wifi_config.ap.max_connection = pap->maxconn;
    pap->wifi_config.ap.pmf_cfg.required = true;

    ESP_LOGW(TAG, "SSID          : %s", ssid);
    ESP_LOGW(TAG, "Password      : %s", pass);
    ESP_LOGW(TAG, "Channel       : %d", pap->channel);
    ESP_LOGW(TAG, "Max connection: %d", pap->maxconn);

}

void wifiap_config_ip(wifiap_t *pap){
	if(pap->ip == NULL || pap->netmsk == NULL){
		ESP_LOGE(TAG, "Invalid IP address, configuration failed!");
		return;
	}

    esp_netif_ip_info_t ip_info;
    memset(&ip_info, 0, sizeof(esp_netif_ip_info_t));

    if (pap->netif){
        esp_netif_dhcps_stop(pap->netif);

        ip_info.ip.addr = esp_ip4addr_aton((const char *)pap->ip);
        ip_info.gw.addr = esp_ip4addr_aton((const char *)pap->ip);
        ip_info.netmask.addr = esp_ip4addr_aton((const char *)pap->netmsk);

        esp_netif_set_ip_info(pap->netif, &ip_info);
        esp_netif_dhcps_start(pap->netif);
    }
}


void wifiap_start(wifiap_t *pap){
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &pap->wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}



static void wifiap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
	wifiap_t *pap = (wifiap_t *)arg;

	if (event_id == WIFI_EVENT_AP_START){
		ESP_LOGW(TAG, "WiFi Access Point Start");
	}
	else if (event_id == WIFI_EVENT_AP_STOP){
		ESP_LOGW(TAG, "WiFi Access Point Stop");
	}
    else if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGW(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGW(TAG, "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
    }

    if(pap->eventhandler != NULL) pap->eventhandler((wifi_event_t)event_id, pap->eventparam);
}

