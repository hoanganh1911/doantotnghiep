
#include "stdint.h"
#include "RAKLorawan.h"



typedef struct {
    String name = "";
    uint8_t deveui[8] = {0};
    uint8_t appeui[8] = {0};
    uint8_t appkey[16] = {0};
    RAK_LORA_CLASS dev_class = RAK_LORA_CLASS_A;
    uint32_t period = 30000;
} lorawan_config_t;


void lorawan_init(lorawan_config_t *pconfig);
void lorawan_join(void);
void lorawan_start(void);
void lorawan_send(uint8_t *buffer, uint8_t length);
