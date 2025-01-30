#include <U8G2OledMenu.h>

// Create an instance of the U8G2 display
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

// Create an instance of the OledMenu
OledMenu menu(u8g2, 1024, 500);

void setup() {
    // Initialize the display
    menu.initDisplay();
    menu.setText("Hello, World!");
}

void loop() {
    // Display a page
    menu.update();
}