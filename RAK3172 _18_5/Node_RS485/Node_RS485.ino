
#include "ModbusMaster.h"
#include "Arduino_JSON.h"
#include "config.h"
#include "config_node.h"
#include "hw_modbus.h"





bool viewmode = false;    
bool viewmode_timer_flag = 0;
uint8_t viewmode_view_period = 0;
uint8_t viewmode_otaa_period = 0;
config_node_t cfg;

void usr_btn_handler(void);
void viewmode_timer_handler(void *data);

void startup_config(void);
void lorawan_begin(void);

char *create_view_data(void);
void get_sensor_data_and_send(bool lorawan_is_true);



/*
* Setup function.
*/
void setup(void) {
    brd_hw_init(usr_btn_handler);

    startup_config();

    lorawan_begin();

    mb_hw_init();
    mb_config_desc(cfg.mbdesc);
}

/*
* Loop program.
*/
void loop(void) {
    if(viewmode == true){
        if(viewmode_view_period * VIEWMODE_TIMER_OVR >= DATAVIEW_PERIOD){
            get_sensor_data_and_send(false);

            viewmode_view_period = 0;
        }

        if(viewmode_otaa_period * VIEWMODE_TIMER_OVR >=  cfg.lorawan.period){
            get_sensor_data_and_send(true);

            viewmode_otaa_period = 0;
        }
    }
    else{  
        led_on();
        get_sensor_data_and_send(true);
        led_off();
        
        api.system.sleep.all(cfg.lorawan.period);
        api.system.sleep.all(cfg.lorawan.period);
    }
}








void usr_btn_handler(void){
    delay(50);

    if(digitalRead(USR_BTN_PIN) == LOW){
        delay(10);
        viewmode = !viewmode;
        
        if(viewmode == true){
            Serial.println("Enter view mode.");
            wf_pwron(); 

            if (api.system.timer.create(VIEWMODE_TIMER, viewmode_timer_handler, RAK_TIMER_PERIODIC) != true)
                Serial.println("Creating timer failed.");
            if (api.system.timer.start(VIEWMODE_TIMER, VIEWMODE_TIMER_OVR, (void *)VIEWMODE_TIMER) != true)
                Serial.println("Starting timer failed.");
        }
        else{
            Serial.println("Enter normal mode.");
            wf_pwroff();

            if (api.system.timer.stop(VIEWMODE_TIMER) != true)
                Serial.println("Stopping timer failed.");
        }
    }
}

void viewmode_timer_handler(void *data){
    if((int)data == VIEWMODE_TIMER) {
        viewmode_view_period++;
        viewmode_otaa_period++;
        led_toggle();
    }
}










void startup_config(void){
    led_on();

    wf_pwron();
    bool config_success = config_node(&cfg);
    delay(1000);
    wf_pwroff();
    if(config_success == false) api.system.reboot();

    led_off();

    delay(100);
}

void lorawan_begin(void){
    led_on();

    lorawan_init(&cfg.lorawan);
    lorawan_join();
    lorawan_start();

    led_off();
}

char *create_view_data(void){
    char *s;
    JSONVar root;
    JSONVar data;

    for(uint8_t i=0; i<mb_set_size(); i++){
        if(mb_select_slave(i)->res == 1)
            data[(const char *)mb_select_slave(i)->desc] = mb_select_slave(i)->rdata[0];
    }

    data["batt"] = (int)(batt_voltage() * 100);
    root["data"] = data;

    String json_str = JSON.stringify(root);
    size_t len = json_str.length();
    s = (char *)malloc(len * sizeof(char) + 1);
    memset(s, 0, len+1);
    json_str.toCharArray(s, len+1);

    return s;
}

void get_sensor_data_and_send(bool lorawan_is_true){
    char *viewdata;

    mb_reqdata();
    viewdata = create_view_data();
    mb_release_data();

    if(lorawan_is_true){
        lorawan_send((uint8_t *)viewdata, strlen(viewdata));
    }
    else{
        Serial.write(viewdata);
    }

    if(viewdata) free(viewdata);
}













