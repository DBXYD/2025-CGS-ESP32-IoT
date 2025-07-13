idf_component_register(
    SRCS "rack_ctrl.c"
    INCLUDE_DIRS "."
    REQUIRES 
        driver 
        led_strip
        api_client
)
