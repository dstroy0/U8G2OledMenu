#ifndef SSD1306_OLED_MENU
#define SSD1306_OLED_MENU

#include <Arduino.h>
#include <IPAddress.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <U8g2lib.h>
#include <MemoryManagerLite.h>
#include <TemplatedLinkedList.h>

// Macro to calculate the number of elements in an array
#ifndef NELEMS
#define NELEMS(x) (sizeof(x) / sizeof((x)[0]))
#endif

namespace MENU
{
    namespace structs
    {
        /// @brief Enumeration for page types
        enum PAGE_TYPE
        {
            USER = 0,    ///< User-defined page
            ERROR = 1,   ///< Error page
            _DEFAULT = 2 ///< Default page
        };

        /// @brief Forward declaration of menuPageInfo struct
        struct menuPageInfo;

        /// @brief Typedef for menu callback function
        typedef void (*menu_callback)(MENU::structs::menuPageInfo *page_info);

        /// @brief Struct for menu page information
        struct menuPageInfo
        {
            PAGE_TYPE type;            ///< Type of the page
            const bool interactive;    ///< Whether the page is interactive
            menu_callback callback;    ///< Callback function for the page
            bool select_item;          ///< Whether an item is selected
            uint8_t page_line = 0;     ///< Current line on the page
            uint8_t page_col = 0;      ///< Current column on the page
            uint8_t num_lines = 0;     ///< Number of lines on the page
            uint8_t chars_on_line = 0; ///< Number of characters on the current line
            char *buffer;              ///< Buffer for the page content
            uint16_t buffer_size;      ///< Size of the buffer
            void *parameters;          ///< Additional parameters for the page

            /// @brief Constructor for menuPageInfo
            /// @param page_type Type of the page
            /// @param interactive Whether the page is interactive
            /// @param callback Callback function for the page
            /// @param buffer Buffer for the page content
            /// @param buffer_size Size of the buffer
            menuPageInfo(PAGE_TYPE page_type, bool interactive, menu_callback callback, char *buffer, uint16_t buffer_size)
                : type(page_type), interactive(interactive), callback(callback), buffer(buffer), buffer_size(buffer_size)
            {
            }
        };

        /// @brief Struct for error page information
        struct errorPageInfo
        {
            PAGE_TYPE type;            ///< Type of the page
            const bool interactive;    ///< Whether the page is interactive
            menu_callback callback;    ///< Callback function for the page
            bool select_item;          ///< Whether an item is selected
            uint8_t page_line = 0;     ///< Current line on the page
            uint8_t page_col = 0;      ///< Current column on the page
            uint8_t num_lines = 0;     ///< Number of lines on the page
            uint8_t chars_on_line = 0; ///< Number of characters on the current line
            char *buffer;              ///< Buffer for the page content
            uint16_t buffer_size;      ///< Size of the buffer
            void *parameters;          ///< Additional parameters for the page

            /// @brief Constructor for errorPageInfo
            /// @param page_type Type of the page
            /// @param interactive Whether the page is interactive
            /// @param callback Callback function for the page
            /// @param buffer Buffer for the page content
            /// @param buffer_size Size of the buffer
            errorPageInfo(PAGE_TYPE page_type, bool interactive, menu_callback callback, char *buffer, uint16_t buffer_size)
                : type(page_type), interactive(interactive), callback(callback), buffer(buffer), buffer_size(buffer_size)
            {
            }
        };

    }; // namespace structs

    namespace builtin_pages
    {
        /// @brief Function to display connection information on the OLED menu
        /// @param page_info Pointer to the menuPageInfo struct
        void connectionInfo(MENU::structs::menuPageInfo *page_info);

        /// @brief Function to display OTA update information on the OLED menu
        /// @param page Pointer to the menuPageInfo struct
        void OTAInfo(MENU::structs::menuPageInfo *page);
    };
};

/// @brief Class representing the OLED menu with text scrolling functionality
class OledMenu
{
public:
    MemoryManager *mmptr; ///< Pointer to the memory manager
    U8G2 &display_hal;    ///< Reference to the U8G2 display object

    singlylist<MENU::structs::menuPageInfo, MENU::structs::PAGE_TYPE, bool, MENU::structs::menu_callback, char *, uint16_t> pages;        ///< List of menu pages
    singlylist<MENU::structs::errorPageInfo, MENU::structs::PAGE_TYPE, bool, MENU::structs::menu_callback, char *, uint16_t> error_pages; ///< List of error pages

    // display buffer
    MemoryManager &display_buffer; ///< Reference to the display buffer
    uint16_t display_buffer_size;  ///< Size of the display buffer

    char *error_buffer_;        ///< Buffer for error messages
    uint16_t error_buffer_size; ///< Size of the error buffer

    char *page_buffer_;        ///< Buffer for page content
    uint16_t page_buffer_size; ///< Size of the page buffer

    byte num_error;                              ///< Number of errors
    bool error_message_display_override = false; ///< Flag to override error message display
    uint8_t num_pages;                           ///< Number of pages
    byte current_page_displayed;                 ///< Index of the currently displayed page
    bool page_entered;                           ///< Flag indicating if a page is entered
    bool line_blinking;                          ///< Flag indicating if a line is blinking
    bool display_connected;                      ///< Flag indicating if the display is connected

    MENU::structs::menuPageInfo *page_info; ///< Pointer to the current page info

    // Text scroller variables
    const char *text;                          ///< Text to be displayed
    char *buffer;                              ///< Buffer to hold the text
    size_t bufferSize;                         ///< Size of the buffer
    int cursorX;                               ///< X position of the cursor
    int cursorY;                               ///< Y position of the cursor
    bool blinkState;                           ///< State of the blink (on/off)
    bool blinkEnabled;                         ///< Whether blinking is enabled
    bool highlightEnabled;                     ///< Whether highlighting is enabled
    unsigned long lastBlinkTime;               ///< Last time the blink state was updated
    const int blinkInterval = 500;             ///< Blink interval in milliseconds
    int minLines;                              ///< Minimum number of lines to display
    int maxLines;                              ///< Maximum number of lines to display
    int dispLines;                             ///< Number of lines to display
    const uint8_t fontMinPixelHeight = 3;      ///< Minimum font pixel height
    const uint8_t fontMaxPixelHeight = 23;     ///< Maximum font pixel height
    const uint8_t *u8g2_font_lookup_table[21]; ///< Lookup table for fonts

    /// @brief Constructor for OledMenu
    /// @param display Reference to the U8G2 display object
    /// @param buffer_size Size of the display buffer
    /// @param text_blink_delay Delay for text blinking
    OledMenu(U8G2 &display, uint16_t buffer_size, uint32_t text_blink_delay);

    /// @brief Destructor for OledMenu
    ~OledMenu();

    /// @brief Initialize connected display
    void initDisplay();

    /// @brief Display the current page
    void displayCurrentPage();

    /// @brief Display text on the screen
    void displayTextOnScreen();

    /// @brief Add a page to the menu
    /// @param type Type of the page
    /// @param interactive Whether the page is interactive
    /// @param callback Callback function for the page
    /// @param page_buffer Buffer for the page content
    /// @param page_buffer_size Size of the page buffer
    /// @return True if the page was added successfully, false otherwise
    bool addMenuPage(MENU::structs::PAGE_TYPE type, bool interactive, MENU::structs::menu_callback callback, char *page_buffer = nullptr, uint16_t page_buffer_size = 0);

    /// @brief Add an error page to the menu
    /// @param callback Callback function for the error page
    /// @return True if the error page was added successfully, false otherwise
    bool addErrorPage(MENU::structs::menu_callback callback);

    /// @brief Refresh the display
    void refreshDisplay();

    /// @brief Move to the next page
    void moveToNextPage();

    /// @brief Move to the previous page
    void moveToPreviousPage();

    /// @brief Move up an item in the menu
    void moveUpMenuItem();

    /// @brief Move down an item in the menu
    void moveDownMenuItem();

    /// @brief Clear the display buffer
    void clearDisplayBuffer();

    /// @brief Clear the page buffer
    void clearPageBuffer();

    /// @brief Acknowledge an error
    void acknowledgeError();

    /// @brief Check if there is an active error
    /// @return True if there is an active error, false otherwise
    bool hasActiveError();

    /// @brief Show an error message
    /// @param fmt Format string for the error message
    /// @param ... Additional arguments for the format string
    /// @return Length of the error message
    int showErrorMessage(const char *fmt, ...);

    /// @brief Check if a page is entered
    /// @return True if a page is entered, false otherwise
    bool isPageEntered();

    /// @brief Exit the current page
    void exitCurrentPage();

    /// @brief Check if the current page is interactive
    /// @return True if the current page is interactive, false otherwise
    bool isCurrentPageInteractive();

    /// @brief Enter the current page
    /// @return True if the page was entered successfully, false otherwise
    bool enterCurrentPage();

    /// @brief Set the text to be displayed and scrolled.
    /// @param txt The text to be displayed.
    void setScrollText(const char *txt);

    /// @brief Set the text to be displayed and scrolled using an external buffer.
    /// @param buf The buffer to hold the text.
    /// @param bufSize The size of the buffer.
    void setScrollText(char *buf, size_t bufSize);

    /// @brief Display the text on the screen.
    /// @param showCursor Whether to show the cursor.
    void displayScrollText(bool showCursor);

    /// @brief Manage the blinking state of the cursor.
    void manageCursorBlink();

    /// @brief Blink the text at the cursor position.
    void blinkTextAtCursorPosition();

    /// @brief Set the number of display lines.
    /// @param num_lines Number of lines to display.
    void setNumberOfDisplayLines(int num_lines);

    /// @brief Get the number of display lines.
    /// @return Number of lines to display.
    int getNumberOfDisplayLines();

    /// @brief Set the font size based on the number of lines to be displayed.
    void setFontSizeForLineLimits();

    /// @brief Get the current X position of the cursor.
    /// @return The X position of the cursor.
    int getCursorXPosition();

    /// @brief Get the current Y position of the cursor.
    /// @return The Y position of the cursor.
    int getCursorYPosition();

    /// @brief Get the width of a character in the current font.
    /// @return The width of a character.
    int getFontCharacterWidth();

    /// @brief Update the display, managing blinking and cursor state.
    void updateDisplay();

private:
    /// @brief Get the menu page info for a given page
    /// @param page Index of the page
    /// @return Pointer to the menu page info
    MENU::structs::menuPageInfo *getMenuPageInfo(uint8_t page);

    /// @brief Get the error page info for a given page
    /// @param page Index of the page
    /// @return Pointer to the error page info
    MENU::structs::errorPageInfo *getErrorPageInfo(uint8_t page);
};

#endif // SSD1306_OLED_MENU