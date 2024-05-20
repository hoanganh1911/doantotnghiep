/*
 * nvs_storage.h
 *
 *  Created on: May 14, 2024
 *      Author: anh
 */

#ifndef MAIN_NVS_STORAGE_NVS_STORAGE_H_
#define MAIN_NVS_STORAGE_NVS_STORAGE_H_

#ifdef __cplusplus
extern "C"{
#endif

#include "stdint.h"
#include "configure.h"

typedef struct {
	char name[64];
	char class[2];
	char deveui[17];
	char appeui[17];
	char appkey[33];
	uint32_t period;
#ifdef RS485
	char mb_desc[512];
#elif defined(ANALOG)
	char calib[100];
#elif defined(RS232)
	char mpn[100];
#endif
} nvs_data_t;



void nvs_init(void);
void nvs_init_data(nvs_data_t *pdata);

void nvs_storing_eui(char *deveui);
void nvs_read_stored_data(void);

char *nvs_create_rak_data(void);

char *nvs_create_lorawan_data(void);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_NVS_STORAGE_NVS_STORAGE_H_ */
