Pracuję na kontrolerze ESP WROOM 32D i wyświetlaczu LCD ST7735S

Konwert pliku na rozmiar:
https://www.img2go.com/result#j=3a07dca5-01fe-4800-a342-de0bd0325f22

Zmiana formatu koloru:
https://lvgl.io/tools/imageconverter

Fragemnt kodu jaki trzeba podmienić w obrazku.h:
#ifndef OBRAZ_H
#define OBRAZ_H

#include <Arduino.h>  // Dla ESP32


const uint16_t obraz[] PROGMEM = {

0x95, 0xdd, 0x99, 0xfe, 0xfb, 0xfe, 0xda, 0xfe, 0xda, 0xfe, 0xf9, 0xfe, 0x39, 0xff, 0x5a, 0xff, 0x3b, 0xff, 0xfa, 0xfe, 0x1a, 0xff, 0x18, 0xff, 0xd6, 0xfe, 

};
#endif

Bibloteki:
Adafruit ST7735 and ST7789 Library
Adafruit GFX Library
https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
ESP32 by Espressif Systems
