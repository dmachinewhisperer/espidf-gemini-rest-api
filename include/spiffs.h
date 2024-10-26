#ifndef SPIFFS_H_
#define SPIFFS_H_
#include "esp_spiffs.h"

/**
 * @brief Mounts the SPIFFS filesystem.
 *
 * This function initializes and mounts the SPIFFS filesystem at the 
 * specified partition. It configures the filesystem settings, checks 
 * for existing files, and formats the filesystem if necessary.
 */
esp_err_t mount_spiffs(void);

/**
 * @brief Unmounts the SPIFFS filesystem.
 *
 *
 * @param conf Pointer to the esp_vfs_spiffs_conf_t configuration 
 *             structure used during mounting.
 */
void unmount_spiffs(esp_vfs_spiffs_conf_t *conf);


/**
 * @brief Reads a file from SPIFFS into a buffer.
 *
 * This function opens a specified file, reads its contents into a 
 * dynamically allocated buffer, and returns the size of the file. 
 * If the file size exceeds the specified maximum size, it returns 
 * an error. The caller is responsible for freeing the allocated 
 * buffer after use.
 *
 * @param file_path The path to the file in the SPIFFS.
 * @param buffer Pointer to a pointer where the allocated buffer 
 *               will be stored.
 * @param file_size Pointer to a size_t where the size of the 
 *                  read file will be stored.
 * @param max_size The maximum allowable size for the file.
 *
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t read_file_to_buffer(const char *file_path, uint8_t **buffer, size_t *file_size, size_t max_size);

#endif