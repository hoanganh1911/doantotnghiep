
#include "hw_modbus.h"
#include "config.h"
#include "ModbusMaster.h"
#include "Arduino_JSON.h"





uint8_t mb_desc_set_size = 0;
mb_desc_t mb_desc_set[MB_DESC_SET_MAX_SIZE];
ModbusMaster mb;

void mb_txmode(void);
void mb_rxmode(void);



/**
* RS485 ModBus.
*/
void mb_txmode(void){
    pinMode(MB_DIR_PIN, OUTPUT);
    digitalWrite(MB_DIR_PIN, HIGH);
}
void mb_rxmode(void){
    pinMode(MB_DIR_PIN, OUTPUT);
    digitalWrite(MB_DIR_PIN, LOW);
}
void sensor_pwron(void){
    pinMode(MB_SENSOR_PWR_PIN, OUTPUT);
    digitalWrite(MB_SENSOR_PWR_PIN, HIGH);
    delay(1000);
}
void sensor_pwroff(void){
    digitalWrite(MB_SENSOR_PWR_PIN, LOW);
    pinMode(MB_SENSOR_PWR_PIN, INPUT);
}
/**
* WiFi module.
*/
void wf_pwron(void){
    delay(500);
    pinMode(WF_PWR_PIN, OUTPUT);
    digitalWrite(WF_PWR_PIN, LOW);
    delay(2000);
}
void wf_pwroff(void){
    digitalWrite(WF_PWR_PIN, HIGH);
    pinMode(WF_PWR_PIN, INPUT);
}
/**
* LED indicator.
*/
void led_on(void){
    digitalWrite(LED_ACT_PIN, LOW);
}
void led_off(void){
    digitalWrite(LED_ACT_PIN, HIGH);
}
void led_toggle(void){
    digitalWrite(LED_ACT_PIN, !digitalRead(LED_ACT_PIN));
}


float batt_voltage(void){
    uint16_t x = 0;
    
    for(int i=0; i<10; i++)
        x += analogRead(BAT_SENSE_PIN);

    return 0.00544 * (float)(x/10.0);
}


void brd_hw_init(void (*btn_wakeup_handler)(void)){   
    Serial.begin(115200, RAK_AT_MODE);
    Serial.println("Startup");

    pinMode(LED_ACT_PIN, OUTPUT);
    pinMode(USR_BTN_PIN, INPUT);
    attachInterrupt(USR_BTN_PIN, btn_wakeup_handler, FALLING);
    analogReadResolution(12);


}

void mb_hw_init(void){
    pinMode(MB_DIR_PIN, INPUT);
    sensor_pwroff();

    mb.preTransmission(mb_txmode);
    mb.postTransmission(mb_rxmode);
}

void mb_reqdata(void){
    pinMode(MB_DIR_PIN, INPUT);
    sensor_pwron();
    
    for(uint8_t id=0; id<mb_desc_set_size; id++){
        mb_desc_t *desc = &mb_desc_set[id];

        MB_UART.end();
        MB_UART.begin(desc->baud, RAK_CUSTOM_MODE);

        mb.begin(desc->addr, MB_UART);

        switch(desc->fcode){
            case MB_FCODE_READ_COILS:  
                desc->res = mb.readCoils(desc->reg, desc->rqty);
            break;
            case MB_FCODE_READ_DISCRTE_INPUTS:  
                desc->res = mb.readDiscreteInputs(desc->reg, desc->rqty);
            break;
            case MB_FCODE_READ_HOLDING_REGISTER:  
                desc->res = mb.readHoldingRegisters(desc->reg, desc->rqty);
            break;
            case MB_FCODE_READ_INPUT_REGISTER:  
                desc->res = mb.readInputRegisters(desc->reg, desc->rqty);
            break;
            case MB_FCODE_WRITE_SINGLE_COIL:  
                desc->res = mb.writeSingleCoil(desc->reg, (uint8_t)desc->wdata);
            break;
            case MB_FCODE_WRITE_SINGLE_REGISTER:  
                desc->res = mb.writeSingleRegister(desc->reg, desc->wdata);
            break;
        };

        if (desc->res == mb.ku8MBSuccess 
            && desc->fcode != MB_FCODE_WRITE_SINGLE_COIL 
            && desc->fcode != MB_FCODE_WRITE_SINGLE_REGISTER) {
            desc->rdata = (uint16_t *)calloc(desc->rqty, sizeof(uint16_t));
            for(uint8_t i=0; i<desc->rqty; i++)
                desc->rdata[i] = mb.getResponseBuffer(i); 

            desc->res = 1;  
        }
        else{
            Serial.printf("Modbus slave 0x%02x error.\r\n", desc->addr);
        }
    }

    sensor_pwroff();
    pinMode(MB_DIR_PIN, INPUT);
    MB_UART.end();
}

void mb_release_data(void){
    for(uint8_t id=0; id<mb_desc_set_size; id++){
        mb_desc_t *desc = &mb_desc_set[id];
        if(desc->rdata != NULL) free(desc->rdata);
    }
}

uint8_t mb_set_size(void){
    return mb_desc_set_size;
}

mb_desc_t *mb_select_slave(uint8_t idx){
    if(idx > MB_DESC_SET_MAX_SIZE) idx = MB_DESC_SET_MAX_SIZE;

    return &mb_desc_set[idx];
}

void mb_config_desc(String json_desc){
    JSONVar root = JSON.parse(json_desc);

    if (JSON.typeof(root) == "undefined") {
        Serial.println("Failed to parse JSON");
        return;
    }

    mb_desc_set_size = root.keys().length();
    for(uint8_t i=0; i<mb_desc_set_size; i++){
        String key = root.keys()[i];
        JSONVar child = root[key];
        // Serial.printf("%s:%s\r\n", key.c_str(), JSON.stringify(child).c_str());

        if (child.hasOwnProperty("desc"))  memcpy(mb_desc_set[i].desc, (const char*)child["desc"], strlen((const char*)child["desc"]));
        if (child.hasOwnProperty("baud"))  mb_desc_set[i].baud  = (int)child["baud"];
        if (child.hasOwnProperty("addr"))  mb_desc_set[i].addr  = strtol((const char *)child["addr"],  NULL, 16);
        if (child.hasOwnProperty("fcode")) mb_desc_set[i].fcode = strtol((const char *)child["fcode"], NULL, 16);
        if (child.hasOwnProperty("reg"))   mb_desc_set[i].reg   = strtol((const char *)child["reg"],   NULL, 16);
        if (child.hasOwnProperty("rqty"))  mb_desc_set[i].rqty  = (int)child["rqty"];
    }
}






















