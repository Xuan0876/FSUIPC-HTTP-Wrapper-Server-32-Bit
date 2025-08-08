/*  UIPChello.c	Displays UIPC link data in a message box */

#include "mongoose.h"
#include <windows.h>
#include <stdio.h>
#include "FSUIPC_User.h"
#include "string.h"
//#include "FSUIPC_User64.h"
#include "cjson.h"
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include "geoHashInterface.c"
typedef struct uipc_para {
	DWORD *dwResult;  // Result of the FSUIPC operation
    DWORD *dwOffset;  // Offset in the FSUIPC memory
    DWORD *dwSize;    // Size of the data to read/write
} uipc_paras_t;
char* pszErrors[] = {
    "Okay",
    "Attempt to Open when already Open",
    "Cannot link to FSUIPC or WideClient",
    "Failed to Register common message with Windows",
    "Failed to create Atom for mapping filename",
    "Failed to create a file mapping object",
    "Failed to open a view to the file map",
    "Incorrect version of FSUIPC, or not FSUIPC",
    "Sim is not version requested",
    "Call cannot execute, link not Open",
    "Call cannot execute: no requests accumulated",
    "IPC timed out all retries",
    "IPC sendmessage failed all retries",
    "IPC request contains bad data",
    "Maybe running on WideClient, but FS not running on Server, or wrong FSUIPC",
    "Read or Write request cannot be added, memory for Process is full",
};

char* printBits(const unsigned char* data, int size) {
    int lo = 0;
    int hi = size * 8;
	char* bits = (char*)malloc(hi + 1); // +1 for null terminator
    if (bits == NULL) exit(1);
    for (int i = lo; i <= hi; i++) {
        int byteIndex = i / 8;
        int bitIndex = i % 8;
        int bitValue = (data[byteIndex] >> bitIndex) & 1;
		bits[i] = (bitValue == 1) ? '1' : '0';
    }
    bits[hi] = '\0';
	return bits;
}
//Credit: https://nachtimwald.com/2017/09/24/hex-encode-and-decode-in-c/
char *bin2hex(const unsigned char *bin, size_t len)
{
	char   *out;
	size_t  i;

	if (bin == NULL || len == 0)
		return NULL;

	out = malloc(len*2+1);
	for (i=0; i<len; i++) {
		out[i*2]   = "0123456789ABCDEF"[bin[i] >> 4];
		out[i*2+1] = "0123456789ABCDEF"[bin[i] & 0x0F];
	}
	out[len*2] = '\0';

	return out;
}
char* get_iso8601_timestamp() {
    time_t now;
    time(&now);
    struct tm* utc = gmtime(&now);
    static char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", utc);
    return timestamp;
}
void convert_data(const unsigned char* raw_data, size_t size, const char* targetType, cJSON* result_obj) {
    if (strcmp(targetType, "int8") == 0) {
        int8_t val;
        memcpy(&val, raw_data, size);
        cJSON_AddNumberToObject(result_obj, "convertedValue", val);
    } 
    else if (strcmp(targetType, "int16") == 0) {
        int16_t val;
        memcpy(&val, raw_data, size);
        cJSON_AddNumberToObject(result_obj, "convertedValue", val);
    } else if (strcmp(targetType, "int32") == 0) {
        int32_t val;
        memcpy(&val, raw_data, size);
        cJSON_AddNumberToObject(result_obj, "convertedValue", val);
    } else if (strcmp(targetType, "float32") == 0) {
        float val;
        memcpy(&val, raw_data, size);
        cJSON_AddNumberToObject(result_obj, "convertedValue", val);
    } else if (strcmp(targetType, "float64") == 0) {
        double val;
        memcpy(&val, raw_data, size);
        cJSON_AddNumberToObject(result_obj, "convertedValue", val);
    } else if (strcmp(targetType, "bool") == 0) {
        bool val = (raw_data[0] != 0);
        cJSON_AddBoolToObject(result_obj, "convertedValue", val);
    } else if (strcmp(targetType, "string") == 0) {
        char* str = malloc(size + 1);
        memcpy(str, raw_data, size);
        str[size] = '\0';
        cJSON_AddStringToObject(result_obj, "convertedValue", str);
        free(str);
    } else if (strcmp(targetType, "binaryString") == 0) {
        char* str = malloc(size + 1);
        memcpy(str, raw_data, size);
		char* bitsStr = printBits(str, size);
        cJSON_AddStringToObject(result_obj, "convertedValue", bitsStr);
        free(str);
        free(bitsStr);
    } else if (strcmp(targetType, "raw") == 0) {
        char* hex_str = bin2hex(raw_data, size);
        cJSON_AddStringToObject(result_obj, "convertedValue", hex_str);
        free(hex_str);
    }
    else if (strcmp(targetType, "bcd") == 0) {
		unsigned short bcd = *(unsigned short*)raw_data;
		int d1 = (bcd >> 12) & 0xF;
		int d2 = (bcd >> 8) & 0xF;
		int d3 = (bcd >> 4) & 0xF;
		int d4 = bcd & 0xF;
        int res = d1 * 1000 + d2 * 100 + d3 * 10 + d4;
        cJSON_AddNumberToObject(result_obj, "rawBytesHex", res);
    }


}
bool validate_type_size(const char* targetType, int size, char** error_msg) {
     if (strcmp(targetType, "int8") == 0) {
        if (size != 1) { *error_msg = "Requires size=1"; return false; }
    } 
    else if (strcmp(targetType, "int16") == 0) {
        if (size != 2) { *error_msg = "Requires size=2"; return false; }
    } else if (strcmp(targetType, "int32") == 0|| strcmp(targetType, "float32") == 0) {
        if (size != 4) { *error_msg = "Requires size=4"; return false; }
    } 
    else if (strcmp(targetType, "bcd") == 0) {
        if (size != 2) { *error_msg = "Requires size=2"; return false; }
    }
    else if (strcmp(targetType, "float32") == 0) {
         if (size != 8) { *error_msg = "Requires size=4"; return false; }
     }
    else if (strcmp(targetType, "float64") == 0) {
        if (size != 8) { *error_msg = "Requires size=8"; return false; }
    } else if (strcmp(targetType, "string") == 0) {
        if (size < 1) { *error_msg = "Requires size≥1"; return false; }
    }
    else if (strcmp(targetType, "binaryString") == 0) {
        if (size < 1) { *error_msg = "Requires size≥1"; return false; }
    }
    else if (strcmp(targetType, "raw") == 0) {

    }
    else {
        *error_msg = "Invalid targetType"; return false;
    }
    return true;
}
char* process_flight_sim_request(const char* request_json, struct uipc_para* paras) {
    // 1. Parse Request 
    cJSON* root_req = cJSON_Parse(request_json);
    if (!root_req) {
 	    char* error_msg = malloc(1024);
        if (error_msg == NULL) exit(1);
        strncpy(error_msg, 
            "{\"status\":\"failed\",\"errorDetails\":{\"code\":\"INVALID_JSON\",\"message\":\"Failed to parse request\"}}", 
            1023);      
        error_msg[1023] = '\0';
        return error_msg;
    }

	// Extract Required Fields 
    const char* request_id = cJSON_GetStringValue(cJSON_GetObjectItem(root_req, "requestId"));
    const char* api_version = cJSON_GetStringValue(cJSON_GetObjectItem(root_req, "apiVersion"));
    cJSON* data_queries = cJSON_GetObjectItem(root_req, "dataQueries");
    if (!request_id || !api_version || !cJSON_IsArray(data_queries)) {
        cJSON_Delete(root_req);
		char* error_msg = malloc(1024);
        strncpy(error_msg, 
        "{\"status\":\"failed\",\"errorDetails\":{\"code\":\"MISSING_FIELDS\",\"message\":\"Missing requestId/apiVersion/dataQueries\"}}",
            1023);      
        error_msg[1023] = '\0';
        return error_msg;
    }

	// 2. Initialize Response Object
    cJSON* root_resp = cJSON_CreateObject();
    cJSON_AddStringToObject(root_resp, "requestId", request_id);
    cJSON_AddStringToObject(root_resp, "timestamp", get_iso8601_timestamp());
    cJSON_AddStringToObject(root_resp, "status", "success");

	// 3. Handle Each Query
    cJSON* data_results = cJSON_CreateArray();
    cJSON_AddItemToObject(root_resp, "dataResults", data_results);

    bool global_success = true;
    char* error_code = "UNKNOWN_ERROR";
    char* error_msg = "Unknown error occurred";

    cJSON* query;
    cJSON_ArrayForEach(query, data_queries) {
        // Obtain Query Parameters
        const char* name = cJSON_GetStringValue(cJSON_GetObjectItem(query, "name"));
        const char* offset = cJSON_GetStringValue(cJSON_GetObjectItem(query, "offset"));
        int size = cJSON_GetNumberValue(cJSON_GetObjectItem(query, "size"));
        const char* target_type = cJSON_GetStringValue(cJSON_GetObjectItem(query, "targetType"));
        sscanf_s(offset, "%lx", paras->dwOffset);
        *(paras->dwSize) = size;
        // Validate Offset, Size, and TargetType
        char* validation_msg = NULL;
        if (offset < 0 || size < 1 || !target_type ||
            !validate_type_size(target_type, size, &validation_msg)) {
            global_success = false;
            error_code = "INVALID_QUERY";
            error_msg = validation_msg ? validation_msg : "Invalid query parameters";
            break;
        }

        // Read Data from Simulator
        unsigned char raw_bytes[256];
        bool read_success = FSUIPC_Read(*(paras->dwOffset), *(paras->dwSize), (char*)raw_bytes, paras->dwSize) &&
            FSUIPC_Process(paras->dwSize);
        if (!read_success) {
            global_success = false;
            error_code = "READ_FAILED";
            error_msg = "Failed to read data from simulator";
            break;
        }

        // Construct Result Object (Singular)
        cJSON* result = cJSON_CreateObject();
        cJSON_AddStringToObject(result, "name", name);
        cJSON_AddStringToObject(result, "offset", offset);
        cJSON_AddNumberToObject(result, "size", size);
        cJSON_AddStringToObject(result, "targetType", target_type);

        // Convert Data Based on TargetType
        convert_data(raw_bytes, size, target_type, result);


        cJSON_AddItemToArray(data_results, result);
    }

	// 4. Handle Global Failure
    if (!global_success) {
        cJSON_DeleteItemFromObject(root_resp, "dataResults"); 
        cJSON_AddStringToObject(root_resp, "status", "failed");
        
        cJSON* error_details = cJSON_CreateObject();
        cJSON_AddStringToObject(error_details, "code", error_code);
        cJSON_AddStringToObject(error_details, "message", error_msg);
        cJSON_AddItemToObject(root_resp, "errorDetails", error_details);
    }

	// 5. Convert Response to JSON String
    char* response_json = cJSON_PrintUnformatted(root_resp);
    cJSON_Delete(root_req);
    cJSON_Delete(root_resp);

    return response_json;
}
char* process_arpt_request(const char* request_json) {
    // 1. Parse Request 
    cJSON* root_req = cJSON_Parse(request_json);
    if (!root_req) {
 	    char* error_msg = malloc(1024);
        if (error_msg == NULL) exit(1);
        strncpy(error_msg, 
            "{\"status\":\"failed\",\"errorDetails\":{\"code\":\"INVALID_JSON\",\"message\":\"Failed to parse request\"}}", 
            1023);      
        error_msg[1023] = '\0';
        return error_msg;
    }

	// Extract Required Fields 
    const char* request_id = cJSON_GetStringValue(cJSON_GetObjectItem(root_req, "requestId"));
    const char* api_version = cJSON_GetStringValue(cJSON_GetObjectItem(root_req, "apiVersion"));
    cJSON* data_queries = cJSON_GetObjectItem(root_req, "dataQueries");
    if (!request_id || !api_version || !cJSON_IsArray(data_queries)) {
        cJSON_Delete(root_req);
		char* error_msg = malloc(1024);
        strncpy(error_msg, 
        "{\"status\":\"failed\",\"errorDetails\":{\"code\":\"MISSING_FIELDS\",\"message\":\"Missing requestId/apiVersion/dataQueries\"}}",
            1023);      
        error_msg[1023] = '\0';
        return error_msg;
    }

	// 2. Initialize Response Object
    cJSON* root_resp = cJSON_CreateObject();
    cJSON_AddStringToObject(root_resp, "requestId", request_id);
    cJSON_AddStringToObject(root_resp, "timestamp", get_iso8601_timestamp());
    cJSON_AddStringToObject(root_resp, "status", "success");

	// 3. Handle Each Query
    cJSON* data_results = cJSON_CreateArray();
    cJSON_AddItemToObject(root_resp, "dataResults", data_results);

    bool global_success = true;
    char* error_code = "UNKNOWN_ERROR";
    char* error_msg = "Unknown error occurred";

    cJSON* query;
        cJSON_ArrayForEach(query, data_queries) {
		// Obtain Query Parameters
        const char* name = cJSON_GetStringValue(cJSON_GetObjectItem(query, "name"));
        const char* lat = cJSON_GetStringValue(cJSON_GetObjectItem(query, "lat"));
        const char* lon = cJSON_GetStringValue(cJSON_GetObjectItem(query, "lon"));
		// Validate lat and lon
        if (!lat || !lon) {
            global_success = false;
            error_code = "INVALID_QUERY";
            error_msg = "Missing lat/lon parameters";
            break;
		}
        // Validate lat and lon range
		double latitude, longitude;
		latitude = atof(lat);
        longitude = atof(lon);
        if (!(latitude <= 91 && latitude >= -91 && longitude >= -180 && longitude <= 180)) {
            global_success = false;
            error_code = "INVALID_QUERY";
            error_msg = "lat/lon out of range";
            break;
        }

		// Calculate nearby airport
		char* airport_ICAO = malloc(64);
		char* airport_name = malloc(256);
		memset(airport_ICAO, 0, 64);
		memset(airport_name, 0, 256);
        double dist = DBL_MAX;
		query_location(latitude, longitude, airport_ICAO, airport_name, &dist);
		assert(airport_ICAO != NULL && airport_name != NULL);
		assert(dist != DBL_MAX);
        char dist_str[10];
        sprintf(dist_str, "%lf", dist);
		// Construct Result Object (Singular)
        cJSON* result = cJSON_CreateObject();
        cJSON_AddStringToObject(result, "name", name);
        cJSON_AddStringToObject(result, "lat", lat);
        cJSON_AddStringToObject(result, "lon", lon);
        cJSON_AddStringToObject(result, "airportICAO", airport_ICAO);
        cJSON_AddStringToObject(result, "airportFullName", airport_name);
        cJSON_AddStringToObject(result, "airportDist", dist_str);

		free(airport_ICAO);
		free(airport_name);

        cJSON_AddItemToArray(data_results, result);
    }
    // 4. Handle Global Failure
    if (!global_success) {
        cJSON_DeleteItemFromObject(root_resp, "dataResults"); 
        cJSON_AddStringToObject(root_resp, "status", "failed");
        
        cJSON* error_details = cJSON_CreateObject();
        cJSON_AddStringToObject(error_details, "code", error_code);
        cJSON_AddStringToObject(error_details, "message", error_msg);
        cJSON_AddItemToObject(root_resp, "errorDetails", error_details);
    }

	// 5. Convert Response to JSON String
    char* response_json = cJSON_PrintUnformatted(root_resp);
    cJSON_Delete(root_req);
    cJSON_Delete(root_resp);

    return response_json;
}
static void fn(struct mg_connection* c, int ev, void* ev_data) {
	uipc_paras_t* uipc_paras = (uipc_paras_t*)c->fn_data;  //Required for FSUIPC operations

    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message* hm = (struct mg_http_message*)ev_data;

        if (mg_match(hm->uri, mg_str("/api/uipc"), NULL)) {
            if (hm->body.len == 0) {
                mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                    "{\"error\": \"Empty request body\"}");
                return;
            }

			// For debugging
            printf("Request Body: %.*s\n", (int)hm->body.len, hm->body.buf);

			// Handle the request and generate response JSON
            char* responseJSON = process_flight_sim_request(hm->body.buf, uipc_paras);

            if (responseJSON != NULL) {
				// Return 200 OK with the complete JSON response
                mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                    "%s\n", responseJSON);
                free(responseJSON); 
            }
            else {
				// Handle failure to generate response
                mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                    "{\"error\": \"Failed to process request\"}");
            }
        }
        else if (mg_match(hm->uri, mg_str("/api/arpt"),NULL)) {
             if (hm->body.len == 0) {
                mg_http_reply(c, 400, "Content-Type: application/json\r\n",
                    "{\"error\": \"Empty request body\"}");
                return;
            }

			// For debugging
            printf("Request Body: %.*s\n", (int)hm->body.len, hm->body.buf);

			// Handle the request and generate response JSON
            char* responseJSON = process_arpt_request(hm->body.buf);

            if (responseJSON != NULL) {
				// Return 200 OK with the complete JSON response
                mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                    "%s\n", responseJSON);
                free(responseJSON); 
            }
            else {
				// Handle failure to generate response
                mg_http_reply(c, 500, "Content-Type: application/json\r\n",
                    "{\"error\": \"Failed to process request\"}");
            }
        }
        else {
			// Return 404 Not Found for unmatched paths
            mg_http_reply(c, 404, "Content-Type: application/json\r\n",
                "{\"error\": \"Endpoint not found\"}");
        }
    }
}
int main(int argc, char* argv[])
{

    DWORD dwResult;

    if (FSUIPC_Open(SIM_ANY, &dwResult))
    {
        char chMsg[128], chTimeMsg[64];
        char chTime[3];
        char acft_icao[256];
        BOOL fTimeOk = TRUE;
        BOOL facftOk = TRUE;

        static char* pFS[] = { "FS98", "FS2000", "CFS2", "CFS1", "Fly!", "FS2002", "FS2004", "FSX 32-bit", "ESP", "P3D 32-bit", "FSX 64-bit", "P3D 64-bit", "MSFS2020", "MSFS2024"};
        printf("Sim is %s,   FSUIPC Version = %c.%c%c%c%c\n",
            (FSUIPC_FS_Version && (FSUIPC_FS_Version <= 15)) ? pFS[FSUIPC_FS_Version - 1] : "Post FS2024",
            '0' + (0x0f & (FSUIPC_Version >> 28)),
            '0' + (0x0f & (FSUIPC_Version >> 24)),
            '0' + (0x0f & (FSUIPC_Version >> 20)),
            '0' + (0x0f & (FSUIPC_Version >> 16)),
            (FSUIPC_Version & 0xffff) ? 'a' + (FSUIPC_Version & 0xff) - 1 : ' '
            );

		DWORD dwOffset;
		DWORD dwSize;
		uipc_paras_t uipc_paras;
		uipc_paras.dwResult = &dwResult;
		uipc_paras.dwOffset= &dwOffset;
		uipc_paras.dwSize = &dwSize;
        printf("UIPC: Link established to FSUIPC\n\n");
        initialise_hash();
		struct mg_mgr mgr;  // Declare event manager
		mg_mgr_init(&mgr);  // Initialise event manager
		mg_http_listen(&mgr, "http://0.0.0.0:8000", fn, &uipc_paras);  // Setup listener
		for (;;) {          // Run an infinite event loop
			mg_mgr_poll(&mgr, 1000);
		}
    }
    else
    {
        printf("UIPC: Failed to open link to FSUIPC\n%s\n", pszErrors[dwResult]);
    }

    finalise_hash();
    FSUIPC_Close();
    return 0;
}
