Steps to build in idf environment;

--> Set target as esp32p4

--> Replace wifi_hanler.c in managed_components/esp-qa_wifi-cmd/src/

--> Build and flash


menuconfig changes;

-->Serial Flaser Config

    -->Flash Size(32 MB)
  
    -->detect flash size when flashing bootloader (.)

  
-->Example Ethernet Configuration

    -->Ethernet PHY Decive (LAN87xx)
    
    -->PHY Address (0)


-->Component Config

    -->I2C Configurations
    
      -->I2C ISR IRAM-Safe (.)
  
    -->ESP PSRAM
  
      -->Support for external PSRAM (.)
      
        -->PSRAM Config
        
            -->Ignore PSRAM when not found (.)
            
            -->Try to allocate WiFi and LWIP in SPIRAM (.)
            
            -->Reserve amount of data for DMA and internal memory (16384)
            
  
    -->ESP System Settings
    
      -->Panic reboot delay (1)
      
      -->Main task stack size (4096)
      
  
    -->FreeRTOS
    
      -->Kernel
      
        -->configMinimal_Stack_Size (16384)
        
        -->configQUEUE_Registry_Size (1)
