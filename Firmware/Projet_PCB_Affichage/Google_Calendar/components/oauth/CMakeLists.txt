idf_component_register(
        SRCS "oauth.c"
        INCLUDE_DIRS "."
        REQUIRES 
                config
                nvs_flash
                esp_http_client
                http_client     # pour http_fetch()
                json           # pour cJSON_Parse(), etc.
)
