#ifndef GERMINI_REST_API_
#define GERMINI_REST_API_

#include "spiffs.h"
#include "cJSON.h"
#include "esp_system.h"
#include "esp_http_client.h"


#define MAX_URL_LENGTH 256        /**< Maximum length for URLs. */
#define MAX_FILE_PATH_LENGTH 64       /**< Maximum length for file paths. */
#define MAX_API_KEY_LENGTH 64    /**< Maximum length for API keys. */
#define MAX_MIME_TYPE_LENGTH 20    /**< Maximum length for MIME types. */
//#define MAX_FILE_SIZE (1024 * 50)  /**< Maximum file size attachable (50 KB). */
#define MAX_FILE_METADATA_LENGTH 10 /**< Maximum length for file name and file extension. */
//#define MAX_GENERATED_TEXT_LENGTH (1024 * 10) /**< Maximum length for generated text (10 KB) */

#define MAX_FILE_SIZE (1 * 1)  /**< Maximum file size attachable (1 KB). */
//#define MAX_FILE_SIZE MAX_ATTACHABLE_FILE_SIZE
//#define MAX_GENERATED_TEXT_LENGTH (1024 * 1 ) /**< Maximum length for generated text (1 KB) */
#define MAX_CA_CERT_LENGHT  2048
#define MAX_GENERATED_TEXT_LENGTH (1024 * 3)
#define MAX_HTTP_RESPONSE_META 1024
#define MAX_HTTP_RESPONSE_LENGTH (MAX_GENERATED_TEXT_LENGTH + MAX_HTTP_RESPONSE_META)
typedef enum {
    ONESHOT,
    CHAT
} PromptMode;

typedef enum {
    NONE,
    ATTACHED,
    UPLOADED
} Artifacts;

typedef struct {
    char *file_uri;
    Artifacts artifacts;
    cJSON *chat_history;
    char *file_upload_url; 
    PromptMode prompt_mode;
    char api_key[MAX_API_KEY_LENGTH + 1]; 
    char mime_type[MAX_MIME_TYPE_LENGTH + 1];
    char file_path[MAX_FILE_PATH_LENGTH + 1];
    //char gen_text[MAX_GENERATED_TEXT_LENGTH + 1];
    char gen_text[MAX_HTTP_RESPONSE_LENGTH + 1];
} PromptConf;


extern const PromptConf DEFAULT_PROMPTCONF_PARAMS;


/**
 * @brief Ends an HTTP client session.
 *
 * This function cleans up the resources associated with an HTTP client session 
 * and releases any memory allocated for the client handle.
 *
 * @param client The handle of the HTTP client session to be cleaned up.
 */
void session_end(PromptConf conf, esp_http_client_handle_t client);



/**
 * @brief Uploads a file to the server.
 *
 * This function checks if the specified file exists and uploads it to 
 * the Google Generative Language API using a resumable upload process. 
 * It sets the necessary headers and prepares the HTTP client for the upload.
 *

 * @param client An esp_http_client_handle_t initialized with a session.
 *
 * @return esp_err_t ESP_OK on success, or ESP_FAIL if an error occurs during 
 *         the upload process or if the file does not exist.
 */
esp_err_t upload_file(PromptConf conf, esp_http_client_handle_t client);



/**
 * @brief Begins an HTTP client session.
 *
 * This function initializes an HTTP client with the specified configuration, 
 * allowing interaction with the Google Generative Language API. It reads a 
 * CA certificate from a specified path (if enabled) for secure communication 
 * and sets up a client handle that can be used for further HTTP requests.
 *
 * @param conf Pointer to a PromptConf structure that contains session information.
 * @param client Pointer to an esp_http_client_handle_t where the initialized client 
 *               handle will be stored.
 *
 * @return esp_err_t ESP_OK on success, or ESP_FAIL if the initialization fails.
 */
esp_err_t session_begin(PromptConf *conf, esp_http_client_handle_t *client);


/**
 * @brief Sends a prompt to the server.
 *
 * This function constructs a JSON payload containing the prompt text and 
 * sends it to the Google Generative Language API. It handles both one-shot 
 * prompts and chat mode prompts, maintaining a history of user interactions.
 *
 * @param text The prompt text to be sent to the API.

 * @param client An esp_http_client_handle_t initialized with a session.
 *
 * @return esp_err_t ESP_OK on success, or ESP_FAIL if an error occurs 
 *         during the prompt process or if the client is not initialized.
 */
esp_err_t prompt(char *text, PromptConf *conf, esp_http_client_handle_t client);



#endif