# ESPIDF Google Gemini LLM Prompting Component

This project is a REST API component for prompting the Google Gemini language model from an ESP32 microcontroller.

## Features

| Feature                          | Description                                         | Status  |
|----------------------------------|-----------------------------------------------------|---------|
| **Text-Only Prompting**          | Text only prompting.   | ✅      |
| **Text with Attached File Prompting** |                                                   |         |
| - Image                          | Support for prompting with image files.            | ⚠️      |
| - Document                       | Support for prompting with document files.         | ⚠️      |
| - Media                          | Support for prompting with audio/video.            | ⚠️      |
| **Text with Uploaded File Prompting** | Prompts on files uploaded to gemini's storage server. | ⚠️      |


- ✅ **Stable**: Feature is stable and fully functional.
- 🔄 **Working**: Feature is actively being developed and tested.
- ⚠️ **WIP**: Work in Progress, features are still being worked on.


## Using
1. Include as you would an ESPIDF component in your project. [See how.](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/build-system.html)

2. See bundled [examples]()

3. Blog [post]()

## Documentation (WIP)

I tried to cover all the key features Gemini currently supports as in the [documentation](https://ai.google.dev/gemini-api/docs)

### Header File
```
#include "gemini-rest-api.h"
```
### Exposed  fuctions
```
/**
 * @brief Mounts the SPIFFS filesystem.
 *
 * This function initializes and mounts the SPIFFS filesystem at the 
 * specified partition. It configures the filesystem settings, checks 
 * for existing files, and formats the filesystem if necessary.
 */
esp_err_t mount_spiffs(void);
```
Only call this function if you intend to do text + file prompting. Before building, put your files in ```/fsimage``` and supply the file name to  PROMPCONF object you pass to ```prompt()```.
***

```
/**
 * @brief Unmounts the SPIFFS filesystem.
 *
 *
 * @param conf Pointer to the esp_vfs_spiffs_conf_t configuration 
 *             structure used during mounting.
 */
void unmount_spiffs(esp_vfs_spiffs_conf_t *conf);
```
Call this function to free resources when file I/O is no longer required in your project. 
***
```
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
```
Start a prompting session. Must be called before ```prompt()```. Only need to be called once in a prompting session.  
***
```
/**
 * @brief Ends an HTTP client session.
 *
 * This function cleans up the resources associated with an HTTP client session 
 * and releases any memory allocated for the client handle.
 *
 * @param client The handle of the HTTP client session to be cleaned up.
 */
void session_end(PromptConf conf, esp_http_client_handle_t client);
```
Call this function when prompting functionality is no longer required to free all resources.
***
```
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
```
This function uploads a file to google upload server and stores the file uri in  ```conf.file_uri``` of the conf object passed to it. 



```
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
```
There are two ways to include a file in a prompt. Embed it directly into the prompt or upload it to Geminis upload server and embed the uri instead in the prompt. The first technique will work only if the file is ASCII encoded(it is base64 encoded into the prompt) while the second can work for any type of MIME file supported by the Gemini API. [See the docs](https://ai.google.dev/gemini-api/docs)

When prompting text-only, call  ```prompt()``` directly. When using a file, pass the file name to ```PROMPTCONF```'s ```conf.file_name``` member and call ```upload_file()``` first before ```prompt()```. If all goes well with ```upload_file()```, it should store the file uri in the ```.file_uri``` field of the ```PROMPTCONF``` object. 

When ```prompt()``` returns, ```conf.file_uri``` is cleared. If you intend to reuse the file again in a prompt, save the file uri in a variable before passing ```PROMPTCONF conf``` to ```prompt()```

### Exposed Objects
```
typedef struct {
    char *file_uri;
    Artifacts artifacts;
    cJSON *chat_history;
    char *file_upload_url; 
    PromptMode prompt_mode;
    char api_key[MAX_API_KEY_LENGTH + 1]; 
    char mime_type[MAX_MIME_TYPE_LENGTH + 1];
    char file_path[MAX_FILE_PATH_LENGTH + 1];
    char gen_text[MAX_HTTP_RESPONSE_LENGTH + 1];
} PromptConf;
```
```
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
```

## Issues and Contributing
Open issues/send a PR.