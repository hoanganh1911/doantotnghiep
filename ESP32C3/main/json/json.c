
#include "json.h"
#include "string.h"
#include "esp_log.h"
#include <ctype.h>



static const char *TAG = "JSON";

static bool is_number(const char *string){
	while(*string){
		if((*string > '9' || *string < '0') && *string != '.')
			return false;
		string++;
	}

	return true;
}

json_t *json_parse(char *input_string){
	if(input_string == NULL) return NULL;

	json_t *newjson = NULL;
	uint16_t len = strlen(input_string);
	uint8_t a = 0, b = 0, c = 0;

	for(uint16_t i=0; i<len; i++){
		if(input_string[i] == '{') a++;
		if(input_string[i] == '}') b++;
		if(input_string[i] == '"') c++;
	}
	if((c >= 2) && (c%2 == 0) && (a == b)){
		newjson = (json_t *)malloc(sizeof(json_t));
		if(newjson == NULL) {
			ESP_LOGE(TAG, "Memory alocation fail at %s -> %d", __FILE__, __LINE__);
			return newjson;
		}
		newjson->value_string = NULL;

		newjson->string = (char *)malloc(len + 1);
		if(newjson->string == NULL) {
			ESP_LOGE(TAG, "Memory alocation fail at %s -> %d", __FILE__, __LINE__);
			return newjson;
		}

		memset(newjson->string, 0, len + 1);
		memcpy(newjson->string, input_string, len);
		newjson->length = len;
	}
	else{
		ESP_LOGE(TAG, "Parse fail at %s -> %d", __FILE__, __LINE__);
	}

	return newjson;
}

uint16_t json_has_item(json_t *json, const char *key){
	uint16_t key_len = strlen(key);
	char *value_start = 0;

	char *find_key_str = (char *)malloc(key_len + 4); // "key":
	sprintf(find_key_str, "\"%s\":", key);
	find_key_str[key_len + 3] = 0;

	value_start = strstr(json->string, find_key_str);
	
	if(value_start != NULL){
		value_start += strlen(find_key_str);
		if(value_start[0] == ' ') value_start++;
		free(find_key_str);

		return (uint16_t)(value_start - json->string);
	}
	free(find_key_str);
	
	return 0;
}

bool json_has_child(json_t *json, const char *key){
	uint16_t value_start_pos = json_has_item(json, key);

	if(json->string[value_start_pos] == '{') return true;

	return false;
}

/* {"data":{"temp":3205,"humi":5203}, "batt":423} */
bool json_get_object(json_t *json, char *key){
	uint16_t val_len = 0;
	uint16_t val_start_pos = 0;
	char *p_val_start;
	bool is_num_bool = false;

	/** check input */
	if(json == NULL || key == NULL){
		ESP_LOGE(TAG, "Invalid parameter");
		return false;
	}
	if(json->string[0] != '{' || json->string[json->length - 1] != '}'){
		ESP_LOGE(TAG, "Invalid json format");
		ESP_LOGI(TAG, "%s", json->string);
		return false;
	}
	
	json->value_type = JSON_VAL_TYPE_UNKNOWN;

	/* Get value position */
	val_start_pos = json_has_item(json, key);

	if(val_start_pos == 0)
		return false;

	p_val_start = json->string + val_start_pos;
	/* Get value */
	if(json_has_child(json, key)){
		/** Search right brace } */
		uint8_t l_brace = 0, r_brace = 0;

		for(val_len=val_start_pos; val_len<json->length; val_len++){
			if(json->string[val_len] == '{') l_brace++;
			if(json->string[val_len] == '}') r_brace++;
			if(l_brace == r_brace) break;
		}
		val_len = val_len - val_start_pos + 1;

		json->value_type = JSON_VAL_TYPE_OBJECT;
	}
	else{ // This object is leaf.
		if(json->string[val_start_pos] == '"'){ // is string.
			uint8_t a = 0;
			for(val_len=val_start_pos; val_len<json->length; val_len++){
				if(json->string[val_len] == '"') a++;
				if(a == 2) break;
			}
			val_len = val_len - val_start_pos - 1;
			p_val_start += 1;

			json->value_type = JSON_VAL_TYPE_STRING;
		}
		else{
			for(val_len=val_start_pos; val_len<json->length; val_len++)
				if(json->string[val_len] == ',') break;
			val_len = val_len - val_start_pos;

			is_num_bool = true;
		}
	}

	if(json->value_string != NULL) free(json->value_string);
	json->value_string = malloc(val_len + 1);
	if(json->value_string == NULL) return false;

	memset((char *)json->value_string, 0, val_len+1);
	memcpy((char *)json->value_string, p_val_start, val_len);

	if(is_num_bool == true){
		if(strcmp((char *)json->value_string, "true") == 0 || strcmp((char *)json->value_string, "false") == 0){
			json->value_type = JSON_VAL_TYPE_BOOL;

			bool b = false;
			int x = strcmp((char *)json->value_string, "true");
			if(json->value_string != NULL) free(json->value_string);

			if(x == 0) b = true;
			json->value_bool = b;
		}
		else{
			if(!is_number((char *)json->value_string)){
				ESP_LOGE(TAG, "NAN");
				if(json->value_string != NULL) free(json->value_string);
				return false;
			}
			if(strstr((char *)json->value_string, ".") != NULL){
				json->value_type = JSON_VAL_TYPE_DOUBLE;

				double d = strtod(json->value_string, NULL);
				if(json->value_string != NULL) free(json->value_string);

				json->value_double = d;
			}
			else{
				json->value_type = JSON_VAL_TYPE_LONG;

				long l = strtol(json->value_string, NULL, 10);
				if(json->value_string != NULL) free(json->value_string);

				json->value_long = l;
			}
		}
	}

	return true;
}

bool json_release(json_t *json){
	if(json->string != NULL) free(json->string);
	if(json->value_string != NULL) free(json->value_string);
	if(json != NULL) free(json);

	return true;
}







bool json_item_is_bool(json_t *json, const char *key);
bool json_item_is_number(json_t *json, const char *key);
bool json_item_is_string(json_t *json, const char *key);
