idf_component_register(SRCS "App_Init.c" "Demo_esp_sr.c"
                    INCLUDE_DIRS "."
                    REQUIRES 
                        driver
                        wifi_sta
                        ap_wifi
                        button
                        LCD_1.69spi
                        INMP441
                        MAX98367A
                        spi-sdcard
                        sdio-sdcard
)

