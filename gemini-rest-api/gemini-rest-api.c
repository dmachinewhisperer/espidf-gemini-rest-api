#include <string.h>
#include <sys/param.h>
#include <stdlib.h>
#include <ctype.h>
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "protocol_examples_utils.h"
#include "esp_tls.h"
#include <sys/stat.h>

#include "esp_system.h"
#include "cJSON.h"
#include "esp_http_client.h"

#include "esp_crt_bundle.h"

#include "spiffs.h"
#include "gemini-rest-api.h"


static const char *TAG = "germini-rest-api";
//static const char *model_name = "gemini-1.5-flash";
static const char *model_name = CONFIG_GEMINI_MODEL_NAME;
static const char *base_url = "https://generativelanguage.googleapis.com";


/**
 * NOTES:
 * 1. esp_http_client_handle_t is a pointer: 
 *      definition: typedef struct esp_http_client *esp_http_client_handle_t
 * 2. 
 */


/**
 * Default PromptConf struct
 */
const PromptConf DEFAULT_PROMPTCONF_PARAMS = {
    .file_uri = NULL,
    .artifacts = NONE,
    .chat_history = NULL,
    .file_upload_url = NULL,
    .prompt_mode = ONESHOT,
    .mime_type = "",
    .file_path = "",
    .api_key = "",
    .gen_text = "",
};

/**
 * dump json
 */
static void dump_json_string(char* st){
    ESP_LOGI(TAG, "%s", st);
}

/*

*/
//static void dump_conf();
//static void show_json();
/**
 * @brief Extracts the file name and file type from a given path.
 *
 * This function takes a file path and extracts the base file name and
 * its extension. The file name is the last component of the path after
 * the last '/' character, while the file type is the substring after
 * the last '.' character in the file name.
 *
 * @param path      The file path from which to extract the file name and type.
 * @param file_name A pointer to a buffer where the extracted file name will be stored.
 * @param file_type A pointer to a buffer where the extracted file type will be stored.
 *
 * @note The function assumes that the provided buffers are sufficiently large enough.
 * 
 * @todo cap file_name and file_type at MAX_FILE_METADATA_LENGHT
 */
static void extract_file_name_and_type(const char *path, char *file_name, char *file_type) {
    const char *last_slash = strrchr(path, '/');
    if (last_slash) {
        strcpy(file_name, last_slash + 1);
    } else {
        strcpy(file_name, path);
    }

    const char *last_dot = strrchr(file_name, '.');
    if (last_dot) {
        size_t name_length = last_dot - file_name;
        strncpy(file_type, last_dot + 1, sizeof(file_type) - 1);
        file_type[sizeof(file_type) - 1] = '\0';
        file_name[name_length] = '\0';
    } else {
        file_type[0] = '\0';
    }
}

/**
 * @brief Extracts the "text" key field from a JSON response.
 *
 * This function parses a JSON object that contains a "candidates" array, 
 * extracts the "text" value from the first candidate's "content" object. 
 * The expected format of the JSON is:
 * 
 * {
 *     "candidates": [{
 *         "content": {
 *             "parts": [{
 *                 "text": ""
 *             }],
 *             "role": ""
 *         },
 *         "finishReason": "",
 *         "index": 0,
 *         "safetyRatings": [{
 *             "category": "",
 *             "probability": ""
 *         }]
 *     }],
 *     "usageMetadata": {
 *         "promptTokenCount": 0,
 *         "candidatesTokenCount": 0,
 *         "totalTokenCount": 0
 *     }
 * }
 *
 * @param root A pointer to the cJSON object representing the root of the JSON response.
 *
 * @return A pointer to a string containing the extracted "text" value. 
 *         The caller is responsible for freeing this string. 
 *         Returns NULL if parsing fails or if the "text" field is not found.
 *
 * @note It is the responsibility of the caller to free the root JSON object. 
 */
static char* extract_text_from_json(cJSON *root) {
    char* result = NULL;
    
    
    if (!root) {
        ESP_LOGE(TAG, "JSON parsing failed: %s", cJSON_GetErrorPtr());
        return NULL;
    }

    cJSON* candidates = cJSON_GetObjectItem(root, "candidates");
    if (!candidates || !cJSON_IsArray(candidates) || cJSON_GetArraySize(candidates) == 0) {
        ESP_LOGE(TAG, "Invalid candidates array");
        cJSON_Delete(root);
        return NULL;
    }

    cJSON* first_candidate = cJSON_GetArrayItem(candidates, 0);
    cJSON* content = cJSON_GetObjectItem(first_candidate, "content");
    cJSON* parts = cJSON_GetObjectItem(content, "parts");
    
    if (!parts || !cJSON_IsArray(parts) || cJSON_GetArraySize(parts) == 0) {
        ESP_LOGE(TAG, "Invalid parts array");
        cJSON_Delete(root);
        return NULL;
    }

    cJSON* first_part = cJSON_GetArrayItem(parts, 0);
    cJSON* text = cJSON_GetObjectItem(first_part, "text");
    
    if (!text || !cJSON_IsString(text)) {
        ESP_LOGE(TAG, "Text not found or invalid");
        cJSON_Delete(root);
        return NULL;
    }

    result = strdup(text->valuestring);
    //cJSON_Delete(root);
    
    return result;
}

/**
 * @brief Handles HTTP eventst.
 *
 * @param evt A pointer to the HTTP client event structure containing event data.
 * 
 * @return An esp_err_t indicating the success or failure of the operation.
 */
static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    static int output_len = 0;

    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;     
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_ON_FINISH:
          ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");  
          (((PromptConf *)evt->user_data)->gen_text)[output_len] = '\0';
          output_len = 0;
          break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);

            //this event occures for each header recieved from the server
            //for uploads, the server includes "x-goog-upload-url" in the header.
            //retrive it whenever present and store it in a the client->user_data->file_uri
            //recall evt->user_data is a pointer to client->user_data
            if (strcmp(evt->header_key, "x-goog-upload-url") == 0) {
                if(((PromptConf *)evt->user_data)->file_upload_url){
                    free(((PromptConf *)evt->user_data)->file_upload_url);
                }
                ((PromptConf *)evt->user_data)->file_upload_url = strdup(evt->header_value);
                }

            break;               
        case HTTP_EVENT_ON_DATA:
            //ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, data = %s, len=%d", (char*)evt->data, evt->data_len);
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA");

            // If user_data buffer is configured, copy the response into the buffer
            int copy_len = 0;
            if (evt->user_data) {
                copy_len = MIN(evt->data_len, (MAX_HTTP_RESPONSE_LENGTH - output_len));
                if (copy_len) {
                    memcpy((((PromptConf *)evt->user_data)->gen_text) + output_len, evt->data, copy_len);
                    
                }
            } 
            output_len += copy_len;
          
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            //ESP_LOGI(TAG, "%s", (char *)evt->data);
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            break;            
        default:
            break;
    }
    return ESP_OK;
}


/**
 * @todo: use defines  to select when certs are used or not
 */
esp_err_t session_begin(PromptConf *conf, esp_http_client_handle_t *client) {

    //check if required fields set
    if (strcmp(conf->api_key, "") == 0) {
        ESP_LOGE(TAG, "api_key not set");
        return ESP_FAIL;
    }

    //load google ca file
#if 0
    size_t ca_size;    
    uint8_t *google_ca;
    const char *ca_path = "/spiffs/google_root_ca.pem";
    esp_err_t ret = read_file_to_buffer(ca_path, &google_ca, &ca_size, MAX_CA_CERT_LENGHT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read ca file");
        return ESP_FAIL;
    }
#endif
    //user_data is a void ptr.
    //point it at the conf struct to track session info 
    esp_http_client_config_t config = {
        .url = "http://generativelanguage.googleapis.com",
        .event_handler = http_event_handler,
        .buffer_size = 2048,
        .timeout_ms = 10000,
        //.auth_type = HTTP_AUTH_TYPE_NONE,
        .user_data = (void*)conf,

        .use_global_ca_store = true,
        .crt_bundle_attach = esp_crt_bundle_attach,
        #if 0
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .cert_pem = (char *)google_ca,
        #endif
    };
    
    *client = esp_http_client_init(&config);
    if (*client == NULL) {
        return ESP_FAIL;
    }
    return ESP_OK;    
}


/**
 * @todo:
 */
void session_end(PromptConf conf, esp_http_client_handle_t client) {
    if (client) {
        
        // Free allocated resources 
        if (conf.file_uri) free(conf.file_uri);
        if (conf.file_upload_url) free(conf.file_upload_url);
        if (conf.chat_history) cJSON_Delete(conf.chat_history);
        esp_http_client_cleanup(client);
        conf.file_uri = NULL; 
        conf.file_upload_url = NULL;
        conf.chat_history = NULL; 
    }
}


/**
 * @todo:
 */
esp_err_t upload_file(PromptConf *conf, esp_http_client_handle_t client){
    //check if file_path is set and file exists

    //check if client is initialized
    if(!client){
        ESP_LOGE(TAG, "http_client is not initialized");
        return ESP_FAIL;
    }    
    //PromptConf conf = *((PromptConf *)client->user_data);
    //char *ptr = (char *)client->user_data;

    struct stat st;
    if (stat(conf->file_path, &st) != 0) {
        ESP_LOGE(TAG, "file_path not set in conf or file does not exist");
        return ESP_FAIL;
    }

    //TODO: check if mime type and file extension match
    ///...

    size_t file_size; 
    uint8_t *file = NULL;
    esp_err_t ret = ESP_FAIL;
    

    //NOTE: file is dynamically allocated it is your responsibility to free it
    ret = read_file_to_buffer(conf->file_path, &file, &file_size, MAX_FILE_SIZE);
    if (ret !=ESP_OK) {
        ESP_LOGE(TAG, "Failed to read file");
        return ret;
    }

    //convert file_size to string, get filename and extension
    char file_name[MAX_FILE_METADATA_LENGTH];
    char file_type[MAX_FILE_METADATA_LENGTH];
    char file_size_str[MAX_FILE_METADATA_LENGTH];
    snprintf(file_size_str, sizeof(file_size_str), "%zu", file_size);
    extract_file_name_and_type(conf->file_path, file_name, file_type);


    //initial resumable request
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "X-Goog-Upload-Protocol", "resumable");
    esp_http_client_set_header(client, "X-Goog-Upload-Command", "start");
    esp_http_client_set_header(client, "X-Goog-Upload-Header-Content-Length", file_size_str);
    esp_http_client_set_header(client, "X-Goog-Upload-Header-Content-Type", conf->mime_type);
    esp_http_client_set_header(client, "Content-Type", "application/json");
 
    cJSON *root = cJSON_CreateObject();
    cJSON *__file = cJSON_CreateObject();
    //TODO
    if(!root || !__file){
        ESP_LOGE(TAG, "Failed to allocated root || __file json structure");
        ret = ESP_FAIL;
        goto cleanup;
    }
    cJSON_AddItemToObject(root, "file", __file);
    cJSON_AddStringToObject(__file, "display_name", file_name);
    
    //NOTE: post_data is dynamically allocated it is your responsibility to free it
    char *post_data = NULL;
    post_data = cJSON_Print(root);
    if(!post_data){
        ESP_LOGE(TAG, "Could not allocate space for post_data");
        ret = ESP_FAIL;
        goto cleanup;
    }
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    //modify url of client to point to upload server
    char url[MAX_URL_LENGTH];
    snprintf(url, sizeof(url), "%s/upload/v1beta/files?key=%s", base_url, conf->api_key);
    esp_http_client_set_url(client, url);
    ret = esp_http_client_perform(client);

    //"esp_http_client_perform" blocks until response is recieved
    //the upload server sends back the upload url in the header of response
    //in the handling of HTTP_EVENT_ON_HEADER, whenever the "x-goog-upload-url"
    //header occurs, it is stored in the conf->user_data.file_upload_url
    if (ret !=ESP_OK) {
        ESP_LOGI(TAG, "Initial resumable failed");
        goto cleanup;
    }
    
    //file upload
    if (!conf->file_upload_url) {
        ESP_LOGE(TAG, "File upload url not set in conf->user_data.file_upload_url");
        ret = ESP_FAIL;
        goto cleanup;
    }

        //unset irrelevant headers
        esp_http_client_delete_header(client, "X-Goog-Upload-Protocol");
        esp_http_client_delete_header(client, "X-Goog-Upload-Command");
        esp_http_client_delete_header(client, "X-Goog-Upload-Header-Content-Length");
        esp_http_client_delete_header(client, "X-Goog-Upload-Header-Content-Type");
        esp_http_client_delete_header(client, "Content-Type");

        //set relevant ones
        esp_http_client_set_header(client, "Content-Length", file_size_str);
        esp_http_client_set_header(client, "X-Goog-Upload-Offset", "0");
        esp_http_client_set_header(client, "X-Goog-Upload-Command", "upload, finalize");
        
        //TODO: Review file datatype
        esp_http_client_set_post_field(client, (char*)file, file_size);

        //point the client to the upload url
        esp_http_client_set_url(client, conf->file_upload_url);
        
        ret = esp_http_client_perform(client);
        if (ret!=ESP_OK) {
            ESP_LOGE(TAG, "File upload failed");
            goto cleanup;
        }

        //process response in conf->text
        cJSON *json = cJSON_Parse(conf->gen_text);
        cJSON *file_obj = cJSON_GetObjectItem(json, "file");
        if(file_obj){
            //this response type is sent after a file upload. it contains the file uri
            //in the format
            /**
                         * {
                            "file": {
                                "uri": ""
                            }
                        }
                */
            char *file_uri = cJSON_GetObjectItem(file_obj, "uri")->valuestring;
            if(file_uri){
                if(conf->file_uri){
                    free(conf->file_uri);
                }
                conf->file_uri = strdup(file_uri);
            }
            
    }        

        esp_http_client_delete_header(client, "Content-Length");
        esp_http_client_delete_header(client, "X-Goog-Upload-Offset");
        esp_http_client_delete_header(client, "X-Goog-Upload-Command");

        ret = ESP_OK;
cleanup:
    if (post_data) free(post_data);
    if(json) cJSON_Delete(json);
    if (root) cJSON_Delete(root);
    if (file) free(file);
    
    
    return ret;    

}


/**
 * @todo: remove conf as it is accessible via client.user_data
 */
esp_err_t prompt(char *text, PromptConf *conf, esp_http_client_handle_t client){
    //check if client is initialized
    if(!client){
        ESP_LOGE(TAG, "http_client is not initialized");
        return ESP_FAIL;
    }   

    char *post_data = NULL;
    esp_err_t ret = ESP_FAIL; 

    cJSON *root = cJSON_CreateObject();
    cJSON *contents = cJSON_CreateArray();
    cJSON *user_content = cJSON_CreateObject();
    cJSON *user_parts = cJSON_CreateArray();
    cJSON *user_text = cJSON_CreateObject();

    //TODO: there is a potential of  memory leakage here. 
    if(!root || !contents || !user_content || !user_parts || !user_text){
        ESP_LOGE(TAG, "Cannot allocate root || contents || user_content || user_parts || user_text ");
        goto cleanup;
    }

    cJSON_AddItemToObject(root, "contents", contents);
    cJSON_AddItemToArray(contents, user_content);
    if(conf->prompt_mode == CHAT){
        cJSON_AddStringToObject(user_content, "role", "user");
    }
    cJSON_AddItemToObject(user_content, "parts", user_parts);
    cJSON_AddStringToObject(user_text, "text", text);
    //cJSON_AddStringToObject(text_part, "text", "Write a poem about Burna Boy.");
    //cJSON_AddStringToObject(text_part, "text", "How does Trump and Harris compare?.");
    cJSON_AddItemToArray(user_parts, user_text);

#if 0
    if(conf->artifacts == ATTACHED){
        //in this prompting style, user wants to attach a file to the prompt

        //check if file_path is set and file exists
        struct stat st;
        if (!conf->file_path || stat(conf->file_path, &st) != 0) {
            ESP_LOGE(TAG, "file_path not set in conf or file does not exist");
            ret = ESP_FAIL;
            goto cleanup;
        }
        
        //attach file    
        cJSON *img_part = cJSON_CreateObject();
        cJSON *inline_data = cJSON_CreateObject();
        cJSON_AddStringToObject(inline_data, "mime_type", "image/jpeg");
        cJSON_AddStringToObject(inline_data, "data", b64_data);
        cJSON_AddItemToObject(img_part, "inline_data", inline_data);
        cJSON_AddItemToArray(parts, img_part);    

    }
#endif


    //TODO: validate mime_type with file extension

    if(conf->artifacts == UPLOADED){
        //in this prompting style, the user have already uploaded 
        //a file using the upload_file() function and the conf->file_uri is set

        if(!conf->file_uri){
            ESP_LOGE(TAG, "failed. conf->artifacts set while file is not uploaded.");
            ret =  ESP_FAIL;
            goto cleanup;
        }
    cJSON *file_part = cJSON_CreateObject();
    cJSON *file_meta = cJSON_CreateObject();

    //TODO: there is a potential of  memory leakage here. 
    if(!file_part || !file_meta){
        ESP_LOGE(TAG, "Cannot allocate file_part || file_meta");
        ret = ESP_FAIL;
        goto cleanup;
    }
    cJSON_AddStringToObject(file_meta, "mime_type", conf->mime_type);
    cJSON_AddStringToObject(file_meta, "file_uri", conf->file_uri);
    cJSON_AddItemToObject(file_part, "file_data", file_meta);
    cJSON_AddItemToArray(user_parts, file_part);    
  
    }

    char generate_url[256];
    int len = snprintf(generate_url, sizeof(generate_url), 
        "%s/v1beta/models/%s:generateContent?key=%s", base_url, model_name, conf->api_key);
    if (len < 0 || len >= sizeof(generate_url)) {
        ESP_LOGE(TAG, "len = %d: Failed to create URL, buffer overflow or error", len);
        ret = ESP_FAIL;
        goto cleanup;
    }
    else{
        ESP_LOGD(TAG, "%s", generate_url);
    }
    esp_http_client_set_url(client, generate_url);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
   
    //if chatting, save the user request history in conf->chat_history
    if(conf->prompt_mode == CHAT){
        if(!conf->chat_history){
            conf->chat_history = cJSON_CreateObject();
            cJSON *chat_contents = cJSON_CreateArray();
            //TODO: 
            if(!chat_contents || !conf->chat_history){
                ESP_LOGE(TAG, "failed to allocate chat_history");
                ret = ESP_FAIL;
                goto cleanup;
            }
            cJSON_AddItemToObject(conf->chat_history, "contents", chat_contents);
            #if 0
            if (cJSON_AddItemToObject(conf->chat_history, "contents", chat_contents) == NULL) {
                ESP_LOGE(TAG, "Failed to add item to chat_history");
                ret = ESP_FAIL;
                goto cleanup;
            }
            #endif
        }
        //save user prompt context
        cJSON *contents = cJSON_GetObjectItem(conf->chat_history, "contents");
        if(!contents){
            ESP_LOGE(TAG, "contents field not found");
            ret = ESP_FAIL;
            goto cleanup;
        }
        //create duplicate instead because root owns user_content so when it is freed, 
        //contents is corrupted
        cJSON *user_content_dup = cJSON_Duplicate(user_content, true);
        cJSON_AddItemToArray(contents, user_content_dup);

    }


    
    //post_data = cJSON_Print(root);

    if(conf->prompt_mode == ONESHOT){
        post_data = cJSON_Print(root);
    }
    else{
        post_data = cJSON_Print(conf->chat_history);
    }

    //dump string
    if(!post_data) {
        ESP_LOGE(TAG, "post_data allocation failed");
        ret = ESP_FAIL;
        goto cleanup;
    } 
    else{
        ESP_LOGI(TAG, "post_data allocated");
        dump_json_string(post_data);
    }
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    ret = esp_http_client_perform(client);
    if(ret != ESP_OK){
        ESP_LOGE(TAG, "prompting failed");
        goto cleanup;
    }
    //int res_len = esp_http_client_read_response(client, conf->gen_text, MAX_GENERATED_TEXT_LENGTH);
    //ESP_LOGI(TAG, "res = %s\nread %d bytes response", conf->gen_text, res_len);
    conf->gen_text[MAX_HTTP_RESPONSE_LENGTH] = '\0';

    //process the response to recover the text part.
    cJSON *json = cJSON_Parse(conf->gen_text);
    if (!json) {
        printf("Failed to parse JSON\n");
        ret = ESP_FAIL;
        goto cleanup;
    }

    if(cJSON_GetObjectItem(json, "candidates")){
        // this response type is due to a prior text prompt
        //see the api response format at: https://ai.google.dev/gemini-api/docs
        //NOTE: gen_text is dynamically allocated it is your responsibility to free it

            char *gen_text = extract_text_from_json(json);
            if(!gen_text){
                ret = ESP_FAIL;
                goto cleanup;
            }
            strncpy(conf->gen_text, gen_text, MAX_HTTP_RESPONSE_LENGTH);
            free(gen_text);
    }
#if 0
    cJSON *file_obj = cJSON_GetObjectItem(json, "file");
    if(file_obj){
        //this response type is sent after a file upload. it contains the file uri
        //in the format
        /**
                     * {
                        "file": {
                            "uri": ""
                        }
                    }
            */
        char *file_uri = cJSON_GetObjectItem(file_obj, "uri")->valuestring;
        if(file_uri){
            if(conf->file_uri){
                free(conf->file_uri);
            }
            conf->file_uri = strdup(file_uri);
        }
        
    }
#endif

    //if chatting, save model response history in conf->chat_history
    if(conf->prompt_mode == CHAT){

        cJSON *contents = cJSON_GetObjectItem(conf->chat_history, "contents");
        cJSON *model_content = cJSON_CreateObject();
        cJSON *model_parts = cJSON_CreateArray();
        cJSON *model_text = cJSON_CreateObject();

        //TODO: there is a potential of  memory leakage here. 
        if(!model_content || !model_parts || !model_text){
            ESP_LOGE(TAG, "Cannot allocate model_content || model_parts || model_text ");
            ret = ESP_FAIL;
            goto cleanup;
        }  

        cJSON_AddItemToArray(contents, model_content);
        cJSON_AddStringToObject(model_content, "role", "model");
        cJSON_AddItemToObject(model_content, "parts", model_parts);
        cJSON_AddStringToObject(model_text, "text", conf->gen_text);
        cJSON_AddItemToArray(model_parts, model_text);       

    }


    //after each prompt, all file artifacts are destroyed to prevent memory bugs
    //if you want to prompt using an uploaded file again, save the file_uri 
    //in a variable after calling upload_file() and before passing and set it to prompt()
    //and supply it to conf whenever doing a file prompt
    ret = ESP_OK;
cleanup:
    if (post_data) free(post_data);
    //if (post_data) cJSON_free(post_data);
    if (json) cJSON_Delete(json);
    if (root) cJSON_Delete(root); 
    if(conf->file_uri) free(conf->file_uri);
    if(conf->file_upload_url) free(conf->file_upload_url);

    conf->file_uri = NULL;
    conf->file_upload_url = NULL;
    return ret;
}

