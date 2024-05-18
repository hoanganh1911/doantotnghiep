
#include "lorawan.h"
#include "config.h"
#include "Arduino_JSON.h"



void joinCallback(int32_t status){
    Serial.printf("Join status: %d\r\n", status);
}

void sendCallback(int32_t status){
    if (status == RAK_LORAMAC_STATUS_OK) {
        Serial.println("Successfully sent");
    }
     else {
        Serial.printf("Sending failed, %d\r\n", status);
    }
}

void recvCallback(SERVICE_LORA_RECEIVE_T *data){
    if (data->BufferSize > 0) {
        Serial.println("Something received!");
        for (int i = 0; i < data->BufferSize; i++) {
            Serial.printf("%x", data->Buffer[i]);
        }
        Serial.print("\r\n");
    }
}



void lorawan_init(lorawan_config_t *pconfig){
    if(api.lorawan.nwm.get() != 1){
        api.lorawan.nwm.set(1);
        api.system.reboot();
    }

    if (!api.lorawan.appeui.set(pconfig->appeui, 8)) {
        Serial.printf("LoRaWan OTAA - set application EUI is incorrect! \r\n");
        return;
    }
    if (!api.lorawan.appkey.set(pconfig->appkey, 16)) {
        Serial.printf("LoRaWan OTAA - set application key is incorrect! \r\n");
        return;
    }
    if (!api.lorawan.deui.set(pconfig->deveui, 8)) {
        Serial.printf("LoRaWan OTAA - set device EUI is incorrect! \r\n");
        return;
    }
    if (!api.lorawan.band.set(OTAA_BAND)) {
        Serial.printf("LoRaWan OTAA - set band is incorrect! \r\n");
        return;
    }
    if (!api.lorawan.deviceClass.set(pconfig->dev_class)) {
        Serial.printf("LoRaWan OTAA - set device class is incorrect! \r\n");
        return;
    }
    if (!api.lorawan.njm.set(RAK_LORA_OTAA)){
        Serial.printf("LoRaWan OTAA - set network join mode is incorrect! \r\n");
        return;
    }
}

void lorawan_join(void){
    if (!api.lorawan.join()){
        Serial.printf("LoRaWan OTAA - join fail! \r\n");
        return;
    }
  
    while (api.lorawan.njs.get() == 0) {
        Serial.print("Wait for LoRaWAN join...");
        api.lorawan.join();
        delay(10000);
    }
}

void lorawan_start(void){
    if (!api.lorawan.adr.set(true)) {
        Serial.printf("LoRaWan OTAA - set adaptive data rate is incorrect! \r\n");
        return;
    }
    if (!api.lorawan.rety.set(1)) {
        Serial.printf("LoRaWan OTAA - set retry times is incorrect! \r\n");
        return;
    }
    if (!api.lorawan.cfm.set(1)) {
        Serial.printf("LoRaWan OTAA - set confirm mode is incorrect! \r\n");
        return;
    }
  
    api.lorawan.registerRecvCallback(recvCallback);
    api.lorawan.registerJoinCallback(joinCallback);
    api.lorawan.registerSendCallback(sendCallback);
}


void lorawan_send(uint8_t *buffer, uint8_t length){
    if(!api.lorawan.send(length, (uint8_t *)buffer, 2, true, 5)) {
        Serial.println("Sending failed");
    }
}














