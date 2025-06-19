idf_component_register(
    SRCS "api_client.c"
    INCLUDE_DIRS "."
    REQUIRES 
        config 
        http_client
        json
        api_client
)
