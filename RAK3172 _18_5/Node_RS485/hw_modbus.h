
#include "stdint.h"
#include "stdio.h"
#include "Arduino.h"






typedef struct{
    char     desc[50] = {0};  // Slave descriptor string.
    int      baud     = 9600; // Slave baudrate.
    uint8_t  addr     = 0x00; // Slave address.
    uint8_t  fcode    = 0x00; // Function code.
    uint16_t reg      = 0x00; // Read register.
    uint8_t  rqty     = 1;    // Read quanty.
    uint16_t *rdata   = NULL; // Session read data.
    uint16_t wdata    = 0x00; // Session write data.
    uint8_t  res      = 0;    // Session result.
} mb_desc_t; // Modbus descriptor.



void wf_pwron(void);
void wf_pwroff(void);

void led_on(void);
void led_off(void);
void led_toggle(void);

float batt_voltage(void);

void brd_hw_init(void (*btn_wakeup_handler)(void));

void sensor_pwron(void);
void sensor_pwroff(void);

void mb_hw_init(void);
void mb_config_desc(String json_desc);
void mb_reqdata(void);
void mb_release_data(void);
uint8_t mb_set_size(void);
mb_desc_t *mb_select_slave(uint8_t idx);

