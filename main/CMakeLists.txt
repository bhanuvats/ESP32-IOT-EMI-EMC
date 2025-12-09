# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.


set(COMPONENT_REQUIRES      "fatfs" "sd_card"  "esp_driver_uart" "esp_driver_gpio" "esp_http_client" "esp_netif" "efuse esp_system driver" "nvs_flash" "mqtt" "app_update" "esp_partition" "esp_system" "esp_https_ota" "spi_flash" "fatfs" "esp_netif" esp_eth esp_driver_i2c "console")

idf_component_register(
    SRCS "main.c" "sd_card_example_main.c" "modbus.c" "eth.c" "cmd_ethernet.c" "ring_buffer.c" "cmd_i2ctools.c" "i2c_tools.c"#"network_thread.cpp" "mqtt_thread.c" "esp_ota.c"    # list the source files of this component
    INCLUDE_DIRS "."     # optional, add here public include directories
    PRIV_INCLUDE_DIRS   # optional, add here private include directories
    REQUIRES        
    PRIV_REQUIRES "${COMPONENT_REQUIRES}" # optional, list the private requirements
)
