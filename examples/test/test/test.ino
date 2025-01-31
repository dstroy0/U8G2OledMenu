#include <U8G2OledMenu.h>

// Create an instance of the U8G2 display
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE);

// Create an instance of the OledMenu
OledMenu menu(u8g2, 1024, 500);

// Define font sizes
const uint8_t *fontSizes[] = {
    u8g2_font_3x3basic_tr, u8g2_font_u8glib_4_tr, u8g2_font_tiny5_tr, u8g2_font_5x7_tr,
    u8g2_font_6x10_tr, u8g2_font_t0_11_tr, u8g2_font_6x13_tr, u8g2_font_7x14_tr,
    u8g2_font_t0_17_tr, u8g2_font_helvR12_tr, u8g2_font_10x20_tf, u8g2_font_profont22_tr,
    u8g2_font_courB18_tr, u8g2_font_crox5t_tr, u8g2_font_crox5h_tr, u8g2_font_ncenR18_tr,
    u8g2_font_courR24_tr, u8g2_font_fur20_tr, u8g2_font_osr21_tr, u8g2_font_logisoso22_tr,
    u8g2_font_timR24_tr
};
const int numFontSizes = sizeof(fontSizes) / sizeof(fontSizes[0]);

void setup() {
    // Initialize the display
    menu.init();
}

void loop() {
    static int fontIndex = 0;
    static bool growing = true;
    static unsigned long lastUpdateTime = 0;

    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime >= 1000 / numFontSizes) {
        lastUpdateTime = currentTime;

        // Set the font size
        u8g2.setFont(fontSizes[fontIndex]);

        // Clear the display buffer
        menu.clearDisplayBuffer();

        // Set the text to be displayed
        menu.setText("Hello World!");

        // Display the text
        menu.displayText(false);

        // Update the font index
        if (growing) {
            fontIndex++;
            if (fontIndex >= numFontSizes) {
                fontIndex = numFontSizes - 1;
                growing = false;
            }
        } else {
            fontIndex--;
            if (fontIndex < 0) {
                fontIndex = 0;
                growing = true;
            }
        }
    }
}