
#include "stdint.h"
#include "stdio.h"
#include "stdbool.h"


typedef enum{
	JSON_VAL_TYPE_UNKNOWN = 0,
	JSON_VAL_TYPE_OBJECT,
	JSON_VAL_TYPE_BOOL,
	JSON_VAL_TYPE_LONG,
	JSON_VAL_TYPE_DOUBLE,
	JSON_VAL_TYPE_STRING,
} json_val_type_t;

typedef struct {
	char *string;
	uint16_t length;
	json_val_type_t value_type;

	char *value_string;
	long value_long;
	double value_double;
	bool value_bool;
} json_t;



json_t *json_parse(char *input_string);
bool json_release(json_t *json);


bool json_has_child(json_t *json, const char *key);
uint16_t json_has_item(json_t *json, const char *key);

bool json_item_is_bool(json_t *json, const char *key);
bool json_item_is_number(json_t *json, const char *key);
bool json_item_is_string(json_t *json, const char *key);


bool json_get_object(json_t *json, char *key);

bool json_get_item_bool(json_t *json, const char *key);
double json_get_item_number(json_t *json, const char *key);
char *json_get_item_string(json_t *json, const char *key);



