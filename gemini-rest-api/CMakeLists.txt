idf_component_register(SRCS "gemini-rest-api.c" "spiffs.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp-tls esp_http_client esp_netif protocol_examples_common nvs_flash  esp_event json heap spiffs
                    )

set_source_files_properties(gemini-rest-api.c
                            PROPERTIES COMPILE_FLAGS
                            -Wno-maybe-uninitialized
                            )
                
spiffs_create_partition_image(storage fsimage FLASH_IN_PROJECT)           