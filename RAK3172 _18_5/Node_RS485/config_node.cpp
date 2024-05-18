
#include "config_node.h"
#include "config.h"
#include "ModbusMaster.h"
#include "Arduino_JSON.h"





void send_eui_to_wf(void);

void send_eui_to_wf(void){
    JSONVar root;
    uint8_t deveui[8] = {0};

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

bool config_node(config_node_t *conf_data){
    String cfg_str = "";
    uint32_t timeout = 0;

    /**
     * Send request config data to ESP32.
    */
    send_eui_to_wf();
    timeout = millis();
    while(!Serial.available()){
        if(millis() - timeout > 5000){
            send_eui_to_wf();
            timeout = millis();
        }
    }
    /**
     * Read config data from ESP32.
    */
    while (Serial.available()) {
        cfg_str += Serial.readStringUntil((char)0xFE);
    }
    Serial.println();

    /**
     * Parse config data.
    */
    JSONVar root = JSON.parse(cfg_str);

    if (JSON.typeof(root) == "undefined") {
        Serial.println("Failed to parse JSON");
        return false;
    }

    JSONVar wan = root["wan"];
    JSONVar mbdesc = root["mbdesc"];
    Serial.println(JSON.stringify(wan));
    Serial.println(JSON.stringify(mbdesc));

    if (wan.hasOwnProperty("name")) conf_data->lorawan.name = String((const char *)wan["name"]);
    if (wan.hasOwnProperty("deveui")) {
        uint64_t deveui = (uint64_t)strtoull((const char *)wan["deveui"], NULL, 16);
        for (uint8_t i=0; i<8; i++){
            conf_data->lorawan.deveui[i] = (uint8_t)(deveui >> (56-8*i));
        }
    }
    if (wan.hasOwnProperty("appeui")) {
        uint64_t appeui = (uint64_t)strtoull((const char *)wan["appeui"], NULL, 16);
        for (uint8_t i=0; i<8; i++){
            conf_data->lorawan.appeui[i] = (uint8_t)(appeui >> (56-8*i));
        }
    }
    if (wan.hasOwnProperty("appkey")) {
        const char *appkey = (const char *)wan["appkey"];
        char appkey_l_buf[16];
        memcpy(appkey_l_buf, appkey, 16);

        uint64_t appkey_l = strtoull((const char *)appkey_l_buf, NULL, 16);
        uint64_t appkey_h = strtoull((const char *)(appkey + 16), NULL, 16);

        for (uint8_t i=0; i<8; i++){
            conf_data->lorawan.appkey[i] = (uint8_t)(appkey_l >> (56-8*i));
            conf_data->lorawan.appkey[i+8] = (uint8_t)(appkey_h >> (56-8*i));
        }
    }
    if (wan.hasOwnProperty("class")) {
        String dev_class = (String((const char *)wan["class"]));
        if(dev_class == "B") conf_data->lorawan.dev_class = RAK_LORA_CLASS_B;
        if(dev_class == "C") conf_data->lorawan.dev_class = RAK_LORA_CLASS_C;
        else                 conf_data->lorawan.dev_class = RAK_LORA_CLASS_A;
    }
    if (wan.hasOwnProperty("period")) conf_data->lorawan.period = (int)wan["period"];

    conf_data->mbdesc = JSON.stringify(mbdesc);

    return true;
}

