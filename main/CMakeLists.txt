set(SRCS
    app_webserver/app_webserver.cpp
    app_ethernet/app_ethernet.cpp
    app_gpio/user_gpio.cpp
    app_gpio/user_key.cpp
    app_led/led_ctrl.cpp
    main.cpp
)

set(INCLUDES
    app_ethernet
    app_gpio
    app_led
    app_webserver
)

set(PRIVATEREQUES
    esp_netif 
    esp_eth 
    ethernet_init
)
set(REQUIRES
    json
    driver
    nvs_flash
    esp_http_server
    esp_timer
    spiffs
    mdns
)

idf_component_register(
    SRCS ${SRCS}
    REQUIRES ${REQUIRES}
    PRIV_REQUIRES ${PRIVATEREQUES}
    INCLUDE_DIRS ${INCLUDES}
)

spiffs_create_partition_image(spiffs ${CMAKE_CURRENT_SOURCE_DIR}/spiffs_image FLASH_IN_PROJECT)