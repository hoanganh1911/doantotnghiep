

#include "stdint.h"
#include "Arduino.h"
#include "lorawan.h"



typedef struct {
    lorawan_config_t lorawan;
    String mbdesc = "";
} config_node_t;

bool config_node(config_node_t *pconfig);