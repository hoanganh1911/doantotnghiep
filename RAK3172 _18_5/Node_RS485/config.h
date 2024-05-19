

#include "stdint.h"
#include "Arduino.h"


#define MB_UART           Serial1
#define MB_BAUDRATE       9600
#define MB_DIR_PIN        PA1
#define MB_SENSOR_PWR_PIN PA11
#define WF_PWR_PIN        PA15
#define LED_ACT_PIN       PA8
#define USR_BTN_PIN       PA9
#define BAT_SENSE_PIN     PB2

#define ANALOG_SAMPLE_NUMBER 100

#define MB_DESC_SET_MAX_SIZE           10
#define MB_FCODE_READ_COILS            0x01
#define MB_FCODE_READ_DISCRTE_INPUTS   0x02
#define MB_FCODE_READ_HOLDING_REGISTER 0x03
#define MB_FCODE_READ_INPUT_REGISTER   0x04
#define MB_FCODE_WRITE_SINGLE_COIL     0x05
#define MB_FCODE_WRITE_SINGLE_REGISTER 0x06

#define OTAA_BAND         (RAK_REGION_AS923_2)
#define OTAA_PERIOD       10000
#define DATAVIEW_PERIOD   2000

#define VIEWMODE_TIMER     RAK_TIMER_1
#define VIEWMODE_TIMER_OVR 200





































