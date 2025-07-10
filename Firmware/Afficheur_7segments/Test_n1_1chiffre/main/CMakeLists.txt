idf_component_register(
    SRCS "Test_n1_1chiffre.c"
    INCLUDE_DIRS "."
    REQUIRES 
        driver
        wifi 
        config 
        http_client
        time_utils
        led_strip   ## pour controler les neopixels
                    
)
