#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#include "esp_heap_caps.h" /*for debugging heap */

//#include "esp_vfs.h"
//#include "esp_vfs_fat.h"
//#include <sys/unistd.h> //unlink()
//#include <sys/stat.h> //stat()
static const char *TAG = "spiffs";

//mounts the fs at the spiff partition specified in partitions.csv at the project root
esp_err_t mount_spiffs(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }
#if 0
#ifdef CONFIG_EXAMPLE_SPIFFS_CHECK_ON_START
    ESP_LOGI(TAG, "Performing SPIFFS_check().");
    ret = esp_spiffs_check(conf.partition_label);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
        return ret;
    } else {
        ESP_LOGI(TAG, "SPIFFS_check() successful");
    }
#endif
#endif
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);
        return ret; 
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }


    // Check consistency of reported partiton size info.
    if (used > total) {
        ESP_LOGW(TAG, "Number of used bytes cannot be larger than total. Performing SPIFFS_check().");
        ret = esp_spiffs_check(conf.partition_label);
        // Could be also used to mend broken files, to clean unreferenced pages, etc.
        // More info at https://github.com/pellepl/spiffs/wiki/FAQ#powerlosses-contd-when-should-i-run-spiffs_check
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
            return ret;
        } else {
            ESP_LOGI(TAG, "SPIFFS_check() successful");
        }
    }

    return ESP_OK;
}

//reads file from flash into ram. 
//space is allocated on the dram heap on sram which is about (320 - space 
//used by static data allocated at compile time)
//find out the runtime heap using idf.py size and specify max_size according
//if file is more than max_size, program returns.
//returns file contents in buffer and size in file_size

esp_err_t read_file_to_buffer(const char *file_path, uint8_t **buffer, size_t *file_size, size_t max_size) {
    FILE *f = fopen(file_path, "rb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file: %s", file_path);
        return ESP_FAIL;
    }
    
    fseek(f, 0, SEEK_END);
    *file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if(*file_size > max_size){
        return ESP_ERR_NO_MEM;
    }    
    //NOTE on using allocators: use this allocator when spiram is available
    //*buffer = heap_caps_malloc(file_size, MALLOC_CAP_SPIRAM);

    *buffer = malloc(*file_size);
    if (!*buffer) {
        ESP_LOGE(TAG, "Failed to allocate %d bytes", *file_size);
        ESP_LOGE(TAG, "heap_caps_get_free_size: %d", heap_caps_get_free_size(MALLOC_CAP_8BIT));
        ESP_LOGE(TAG, "heap_caps_get_largest_free_block: %d", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
        
        fclose(f);
        return ESP_FAIL;
    }
    
    if (fread(*buffer, 1, *file_size, f) != *file_size) {
        ESP_LOGE(TAG, "Failed to read file");
        free(*buffer);
        fclose(f);
        return ESP_FAIL;
    }
    
    fclose(f);
    return ESP_OK;
}

void unmount_spiffs(esp_vfs_spiffs_conf_t *conf){
    esp_vfs_spiffs_unregister(conf->partition_label);
    ESP_LOGI(TAG, "SPIFFS unmounted");
}