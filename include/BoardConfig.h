#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#if defined(BOARD_ESP32_S3)

    #define RESET_BUTTON_PIN 0

    #define BOARD_HAS_RGB_LED
    #define RGB_LED_PIN 48

#elif defined(BOARD_ESP32)

    #define RESET_BUTTON_PIN 0

#else

    #error "Unsupported board configuration"

#endif

#endif