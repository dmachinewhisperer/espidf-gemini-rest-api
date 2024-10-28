#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "gemini-rest-api.h"

#define MAX_PROMPT_LENGHT 1024
#define PART_SIZE 32

static const char *TAG = "prompt> ";

/**
 * prefer to create this large object(~20KB) outside of the task running your app and
 *  on the global scope for allocation on the data segment at compile time
 * creating it inside of a task dramatically increases the stack size needed to run 
 * the task to prevent stackoverflow
 */

static PromptConf conf;

#if 0
static void get_string(char *line, size_t size)
{
    int count = 0;
    while (count < size) {
        int c = fgetc(stdin);
        if (c == '\n') {
            line[count] = '\0';
            break;
        } else if (c > 0 && c < 127) {
            line[count] = c;
            ++count;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
#endif

void split_string(const char *input, char part1[PART_SIZE], char part2[PART_SIZE]) {
    while (isspace((unsigned char)*input)) {
        input++;
    }

    const char *space = strchr(input, ' ');
    if (!space) {
        strncpy(part1, input, PART_SIZE);
        part1[PART_SIZE - 1] = '\0';
        part2[0] = '\0';
        return;
    }

    size_t len1 = space - input;
    strncpy(part1, input, len1 < PART_SIZE ? len1 : PART_SIZE - 1);
    part1[len1 < PART_SIZE ? len1 : PART_SIZE - 1] = '\0';

    input = space + 1;

    while (isspace((unsigned char)*input)) {
        input++;
    }

    const char *end = input + strlen(input);
    while (end > input && isspace((unsigned char)*(end - 1))) {
        end--;
    }

    size_t len2 = end - input;
    strncpy(part2, input, len2 < PART_SIZE ? len2 : PART_SIZE - 1);
    part2[len2 < PART_SIZE ? len2 : PART_SIZE - 1] = '\0';
}

static void get_string(char *line, size_t size)
{
    int count = 0;
    int c;
    while (count < size) {
        c = fgetc(stdin);
        if (c == '\n' || c == '\r') {
            printf("\n");
            line[count] = '\0';
            break;
        } else if (c == 127 || c == '\b') { // Backspace
            if (count > 0) {
                printf("\b \b"); // Move cursor back and erase character
                count--;
            }
        } else if (c > 0 && c < 127) {
            printf("%c", c); // Echo the character
            line[count] = c;
            count++;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}


void file_processing_task(void *pvParameter) {
    esp_err_t ret;
    //setup the conf object for file upload
    conf = DEFAULT_PROMPTCONF_PARAMS;
    //strcpy(conf.file_path, "/spiffs/pets.jpg");
    //strcpy(conf.mime_type, "image/jpg");    /*check that your file mime is supported by gemini api*/
    const char *api_key = CONFIG_GEMINI_API_KEY;
    strncpy(conf.api_key, api_key, MAX_API_KEY_LENGTH);
    
    //setup http client
    esp_http_client_handle_t client = NULL;
    ESP_ERROR_CHECK(session_begin(&conf, &client));

    //upload file
    //after the upload_file(), the file_uri will be stored in conf.file_uri
    //ESP_ERROR_CHECK(upload_file(&conf, client));
    //ESP_LOGI(TAG, "file_upload_url: %s", conf.file_upload_url);
    //ESP_LOGI(TAG, "file_uri: %s", conf.file_uri);
    

    char prompt_text[MAX_PROMPT_LENGHT];
    conf.prompt_mode  = CHAT;   /*store chat history*/

    
    while(true){
      do{
          printf(TAG, "Prompt: ");
          get_string(prompt_text, MAX_PROMPT_LENGHT);
      }while(strlen(prompt_text)==0);

      //if user enters '/spiffs/filename mime/type', upload the file
      if (strncmp(prompt_text, "/spiffs", 5) == 0){
        
        char filename[PART_SIZE];
        char mimetype[PART_SIZE];
        split_string(prompt_text, filename, mimetype);
        printf("filename: %s, mimetype: %s", filename, mimetype);
        strcpy(conf.file_path, filename);
        strcpy(conf.mime_type, mimetype);
        //ESP_ERROR_CHECK(upload_file(&conf, client));
        ret = upload_file(&conf, client);

        if(ret ==ESP_OK){
        ESP_LOGI(TAG, "file_upload_url: %s", conf.file_upload_url);
        ESP_LOGI(TAG, "file_uri: %s", conf.file_uri);
        //indicate there is an outstanding uploaded file. 
        //This is how prompt()knows to include when forming prompts
        conf.artifacts = UPLOADED;
        }
      }
      else{
      //after prompt returns, file_uri is cleared. 
      //save in a var if you intend to reuse before the subsequent line
      //ESP_ERROR_CHECK(prompt(prompt_text, &conf, client));
      ret = prompt(prompt_text, &conf, client);
      if(ret == ESP_OK){
      //ESP_LOGI(TAG, "%s\n\n", conf.gen_text);
      printf("%s\n\n",conf.gen_text);
        ESP_LOGE(TAG, "Free heap: %d, Largest Free Block: %d", 
                heap_caps_get_free_size(MALLOC_CAP_8BIT),
                heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));   
      }     
      }
    }
     
    session_end(conf, client);
    ESP_LOGI(TAG, "Ended successfully");
    vTaskDelete(NULL);
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    //mount fs
    //ESP_ERROR_CHECK(mount_spiffs());

    //http_test_task(NULL);
    mount_spiffs();
    xTaskCreate(&file_processing_task, "img_p", 8192, NULL, 5, NULL);

}