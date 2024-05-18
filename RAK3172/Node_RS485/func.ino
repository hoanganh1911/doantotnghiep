
#include "def.h"
#include "ModbusMaster.h"
#include "Arduino_JSON.h"

#define FIRM_VER "v1.7.15.5"

void mb_txmode(void);
void mb_rxmode(void);



/**
* RS485 ModBus.
*/
void mb_txmode(void){
    digitalWrite(MB_DIR_PIN, HIGH);
}
void mb_rxmode(void){
    digitalWrite(MB_DIR_PIN, LOW);
}
void mb_sensor_pwron(void){
    digitalWrite(MB_SENSOR_PWR_PIN, HIGH);
}
void mb_sensor_pwroff(void){
    digitalWrite(MB_SENSOR_PWR_PIN, LOW);
}
/**
* WiFi module.
*/
void wf_pwron(void){
    // Serial.end();
    delay(500);
    pinMode(WF_PWR_PIN, OUTPUT);
    digitalWrite(WF_PWR_PIN, LOW);
    delay(2000);
    // Serial.begin(115200, RAK_AT_MODE);
    // delay(100);
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


void hw_init(void){   
    Serial.end();
    pinMode(MB_DIR_PIN,        OUTPUT);
    pinMode(MB_SENSOR_PWR_PIN, OUTPUT);
    pinMode(LED_ACT_PIN,       OUTPUT);
    pinMode(USR_BTN_PIN,       INPUT_PULLUP);
    attachInterrupt(USR_BTN_PIN, WakeUp, FALLING);
    analogReadResolution(12);

    digitalWrite(LED_ACT_PIN, HIGH);
    digitalWrite(MB_DIR_PIN, LOW);
    led_on();
    mb_sensor_pwroff();

    MB_UART.begin(MB_BAUDRATE, RAK_CUSTOM_MODE);
    mb.preTransmission(mb_txmode);
    mb.postTransmission(mb_rxmode);

    delay(1000);
    Serial.begin(115200, RAK_AT_MODE);
    Serial.setTimeout(5000);
    Serial.print("Time out = ");
    Serial.println(Serial.getTimeout());
    Serial.printf("Startup with verison %s\r\n",FIRM_VER);

}

void mb_reqdata(void){
    mb_sensor_pwron();
    delay(1000);
    
    for(uint8_t id=0; id<mb_desc_set_size; id++){
        mb_desc_t *desc = &mb_desc_set[id];

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
            Serial.printf("Modbus slave 0x%02x have problem.\r\n", desc->addr);
        }
    }
    mb_sensor_pwroff();
}

void mb_release_data(void){
    for(uint8_t id=0; id<mb_desc_set_size; id++){
        mb_desc_t *desc = &mb_desc_set[id];
        if(desc->rdata != NULL) free(desc->rdata);
    }
}

void mb_config_desc(const char *json_desc){
    JSONVar root = JSON.parse(json_desc);
    Serial.println(json_desc);

    if (JSON.typeof(root) == "undefined") {
        Serial.println("Failed to parse JSON");
        return;
    }

    mb_desc_set_size = root.keys().length();
    for(uint8_t i=0; i<mb_desc_set_size; i++){
        String key = root.keys()[i];
        JSONVar child = root[key];
        Serial.printf("%s:%s\r\n", key.c_str(), JSON.stringify(child).c_str());

        if (child.hasOwnProperty("desc"))  memcpy(mb_desc_set[i].desc, (const char*)child["desc"], strlen((const char*)child["desc"]));
        if (child.hasOwnProperty("addr"))  mb_desc_set[i].addr  = strtol((const char *)child["addr"],  NULL, 16);
        if (child.hasOwnProperty("fcode")) mb_desc_set[i].fcode = strtol((const char *)child["fcode"], NULL, 16);
        if (child.hasOwnProperty("reg"))   mb_desc_set[i].reg   = strtol((const char *)child["reg"],   NULL, 16);
        if (child.hasOwnProperty("rqty"))  mb_desc_set[i].rqty  = (int)child["rqty"];
    }
}







void send_eui_to_wf(void){
    JSONVar root;

    uint32_t UID_L = *(__IO uint32_t *)UID64_BASE;
    uint32_t UID_H = *(__IO uint32_t *)(UID64_BASE + 4UL);

    deveui[0] = (UID_L >> 0) & 0xFF;
    deveui[1] = (UID_L >> 8) & 0xFF;
    deveui[2] = (UID_L >> 16) & 0xFF;
    deveui[3] = (UID_L >> 24) & 0xFF;
    deveui[4] = (UID_H >> 0) & 0xFF;
    deveui[5] = (UID_H >> 8) & 0xFF;
    deveui[6] = (UID_H >> 16) & 0xFF;
    deveui[7] = (UID_H >> 24) & 0xFF;

    char buf[17] = {0};
    sprintf(buf, "%08x%08x", UID_H, UID_L);
    root["deveui"] = buf;

    String jsonString = JSON.stringify(root);
    Serial.write(jsonString.c_str());
}


void config_node(void){

    uint32_t timeout = millis();

    send_eui_to_wf();
    while(!Serial.available()){
        if(millis() - timeout > 5000){
            send_eui_to_wf();
            timeout = millis();
        }
    }
    
    String returnString = "";

    while (Serial.available()) {
        returnString+=Serial.readStringUntil('$');
    }

    Serial.println();
    Serial.println(returnString);    
    JSONVar root = JSON.parse(returnString);
    if (JSON.typeof(root) == "undefined") {
        Serial.println("Failed to parse JSON");
        return;
    }
    String jsonString = JSON.stringify(root);

    Serial.print("JSON.stringify(root) = ");
    Serial.println(jsonString);
    Serial.print("JSON length = ");
    Serial.println(jsonString.length());
}


void Serial_readbuffer(void){
    String cfg_str = "", len_str = "";
    uint16_t total_len = 0;



    
}



















