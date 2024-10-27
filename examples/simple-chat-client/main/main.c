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


static const char *TAG = "main";

/**
 * prefer to create this large object(~20KB) outside of the task running your app and
 *  on the global scope for allocation on the data segment at compile time
 * creating it inside of a task dramatically increases the stack size needed to run 
 * the task to prevent stackoverflow
 */

static PromptConf conf;


/*
returns null terminated string provided user enters < (size) characters
For safety, if the char array size is n(n > 1), pass size = n-1 and set arr[n-1] = '\0' 
in the calling function to guard against overrruns. 
*/
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


void chat_client_task(void *pvParameter) {
    conf = DEFAULT_PROMPTCONF_PARAMS;
    conf.prompt_mode = CHAT;
    char prompt_text[MAX_PROMPT_LENGHT];
    const char *api_key = CONFIG_GEMINI_API_KEY;
    strncpy(conf.api_key, api_key, MAX_API_KEY_LENGTH);

    esp_http_client_handle_t client = NULL;
    ESP_ERROR_CHECK(session_begin(&conf, &client));

    while(true){
      do{
          printf(TAG, "Prompt: ");
          get_string(prompt_text, MAX_PROMPT_LENGHT);
      }while(strlen(prompt_text)==0);


      ESP_ERROR_CHECK(prompt(prompt_text, &conf, client));
      //ESP_LOGI(TAG, "%s\n\n", conf.gen_text);
      printf("%s\n\n",conf.gen_text);
        ESP_LOGE(TAG, "Free heap: %d, Largest Free Block: %d", 
                heap_caps_get_free_size(MALLOC_CAP_8BIT),
                heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
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
    xTaskCreate(&chat_client_task, "chat_client", 8192, NULL, 5, NULL);

}