
#include "ModbusMaster.h"
#include "Arduino_JSON.h"
#include "def.h"

#define OTAA_PERIOD 1000


typedef struct{
    char desc[50]   = {0};  // Slave descriptor string.
    uint8_t addr    = 0x00; // Slave address.
    uint8_t fcode   = 0x00; // Function code.
    uint16_t reg    = 0x00; // Read register.
    uint8_t rqty    = 1;    // Read quanty.
    uint16_t *rdata = NULL; // Session read data.
    uint16_t wdata  = 0x00; // Session write data.
    uint8_t res     = 0;    // Session result.
} mb_desc_t; // Modbus descriptor.


ModbusMaster mb;
uint8_t mb_desc_set_size = 0;
mb_desc_t mb_desc_set[MB_DESC_SET_MAX_SIZE];
uint8_t deveui[8], appeui[8], appkey[16];
bool view_config_mode = false;                            


/**
* RS485 ModBus.
*/
void mb_sensor_pwron(void);
void mb_sensor_pwroff(void);
void mb_config_desc(const char *json_desc);
void mb_reqdata(void);
void mb_release_data(void);
/**
* WiFi module.
*/
void wf_pwron(void);
void wf_pwroff(void);
/**
* LED indicator.
*/
void led_on(void);
void led_off(void);


void hw_init(void);
void WakeUp(void);
void Serial_readbuffer(void);

void send_eui_to_wf(void);
void config_node(void);


/*
* Setup function.
*/

void setup(void) {

    hw_init();

    wf_pwron();
    
    led_on();
    config_node();
    delay(10000);
    led_off();
    // mb_config_desc(json_mb_desc);
}
/*
* Loop program.
*/
void loop(void) {
    if(view_config_mode == true){
        led_on();
        Serial.printf("Battery voltage = %.fV.\r\n", 0.00544 * (float)analogRead(BAT_SENSE_PIN));
        delay(1000);
    }
    else{  
        // wf_pwron();
        led_on();
        delay(OTAA_PERIOD);
        // mb_reqdata();
        // if(mb_desc_set[0].res == 1) Serial.printf("%s=%d, ", mb_desc_set[0].desc, mb_desc_set[0].rdata[0]);
        // if(mb_desc_set[1].res == 1) Serial.printf("%s=%d\r\n", mb_desc_set[1].desc, mb_desc_set[1].rdata[0]);
        // mb_release_data();
        led_off();
        delay(OTAA_PERIOD);
        // Serial.printf("Try sleep %ums..", OTAA_PERIOD);
        // api.system.sleep.all(OTAA_PERIOD);
        // Serial.println("Wakeup..");

    }


}

void WakeUp(void){
    delay(50);
    if(digitalRead(USR_BTN_PIN) == LOW){
      delay(100);
      view_config_mode = !view_config_mode;
      Serial.println("Switch mode");
      (view_config_mode)? wf_pwron() : wf_pwroff();
    }
}























