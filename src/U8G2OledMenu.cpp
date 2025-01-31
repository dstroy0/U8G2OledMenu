#include "U8G2OledMenu.h"

/// @brief Constructor for OledMenu
/// @param display Reference to the U8G2 display object
/// @param buffer_size Size of the display buffer
/// @param text_blink_delay Delay for text blinking
OledMenu::OledMenu(U8G2 &display, uint16_t buffer_size, uint32_t text_blink_delay)
    : display_hal(display), display_buffer_size(buffer_size), mmptr(new MemoryManager(buffer_size)),
      display_buffer(*mmptr), error_buffer_(reinterpret_cast<char *>(display_buffer.allocate(display_buffer.size() / 2))),
      error_buffer_size(display_buffer.size() / 2), num_pages(0), page_buffer_(nullptr),
      page_buffer_size(0), num_error(0), error_message_display_override(false), current_page_displayed(0),
      page_entered(false), line_blinking(false), display_connected(false), page_info(nullptr),
      text(nullptr), buffer(nullptr), bufferSize(0), blinkState(false), blinkEnabled(false),
      highlightEnabled(false), lastBlinkTime(0), minLines(1), maxLines(10), dispLines(4), maxWidth(display_hal.getDisplayWidth()), maxHeight(display_hal.getDisplayHeight()),
      u8g2_font_lookup_table{
          u8g2_font_3x3basic_tr, u8g2_font_u8glib_4_tr, u8g2_font_tiny5_tr, u8g2_font_5x7_tr,
          u8g2_font_6x10_tr, u8g2_font_t0_11_tr, u8g2_font_6x13_tr, u8g2_font_7x14_tr,
          u8g2_font_t0_17_tr, u8g2_font_helvR12_tr, u8g2_font_10x20_tf, u8g2_font_profont22_tr,
          u8g2_font_courB18_tr, u8g2_font_crox5t_tr, u8g2_font_crox5h_tr, u8g2_font_ncenR18_tr,
          u8g2_font_courR24_tr, u8g2_font_fur20_tr, u8g2_font_osr21_tr, u8g2_font_logisoso22_tr,
          u8g2_font_timR24_tr}
{
}

/// @brief Destructor for OledMenu
OledMenu::~OledMenu()
{
    delete mmptr;
}

/// @brief Initialize connected display
void OledMenu::init()
{
    if (display_hal.begin())
    {
        display_connected = true;
        display_hal.setFont(u8g2_font_helvB08_tf);
        display_hal.setFontRefHeightExtendedText();
        display_hal.enableUTF8Print();
    }
    else
    {
        display_connected = false;
    }
}

/// @brief Display text on the screen
/// @param showCursor Whether to show the cursor.
void OledMenu::displayText(bool showCursor)
{
    if (buffer == nullptr)
    {
        display_hal.clearBuffer();
        display_hal.sendBuffer();
        return;
    }

    display_hal.clearBuffer();
    setFontSizeForLineLimits();
    display_hal.setFontMode(1); // Enable transparent mode for highlighting

    int lineSpacing = display_hal.getMaxCharHeight();
    int maxLines = display_hal.getDisplayHeight() / lineSpacing;
    char *line;
    char *savePtr;

    line = strtok_r(buffer, "\n", &savePtr);

    int currentY = page_info->anchorY;
    int lineCount = 0;
    while (line != NULL && lineCount < maxLines)
    {
        if (highlightEnabled)
        {
            display_hal.drawBox(page_info->anchorX, currentY - lineSpacing, maxWidth, lineSpacing);
        }
        display_hal.drawStr(page_info->anchorX, currentY, line);
        if (showCursor)
        {
            display_hal.drawVLine(page_info->cursorX, currentY - lineSpacing, lineSpacing);
        }
        currentY += lineSpacing;
        line = strtok_r(NULL, "\n", &savePtr);
        lineCount++;
    }

    // Fill remaining lines with empty space if less than maxLines
    while (lineCount < maxLines)
    {
        currentY += lineSpacing;
        lineCount++;
    }

    display_hal.sendBuffer();
}

/// @brief Add a page to the menu
/// @param type Type of the page
/// @param interactive Whether the page is interactive
/// @param callback Callback function for the page
/// @param page_buffer Buffer for the page content
/// @param target_buffer_size Size of the page buffer
/// @return True if the page was added successfully, false otherwise
bool OledMenu::addMenuPage(MENU::structs::PAGE_TYPE type, bool interactive, MENU::structs::menu_callback callback, char *page_buffer, uint16_t target_buffer_size)
{
    if (page_buffer == nullptr || target_buffer_size == 0)
    {
        page_buffer = page_buffer_;
        target_buffer_size = page_buffer_size;
    }

    MENU::structs::menuPageInfo page_info(type, interactive, callback, page_buffer, target_buffer_size);
    page_info.callback(&page_info); // Call the callback function to populate the page buffer

    // Check if the buffer size is sufficient
    if (page_info.needs_buffer_size > target_buffer_size)
    {
        clearDisplayBuffer();
        snprintf(error_buffer_, error_buffer_size, "Insufficient display_buffer size");
        displayText(false);
        return false;
    }

    pages.insertAtEnd(type, interactive, callback, page_buffer, target_buffer_size);
    MENU::structs::menuPageInfo *page = pages.getLastAccessedNodeStoragePtr();
    if (page)
    {
        page->max_chars_on_line = calculateMaxCharsOnLine(page_buffer, target_buffer_size); // Set max_chars_on_line
        num_pages++;
        return true;
    }
    return false;
}

/// @brief Add an error page to the menu
/// @param callback Callback function for the error page
/// @return True if the error page was added successfully, false otherwise
bool OledMenu::addErrorPage(MENU::structs::menu_callback callback)
{
    MENU::structs::errorPageInfo page_info(MENU::structs::PAGE_TYPE::ERROR, false, callback, error_buffer_, error_buffer_size);
    page_info.callback(&page_info); // Call the callback function to populate the page buffer

    // Check if the buffer size is sufficient
    if (page_info.needs_buffer_size > error_buffer_size)
    {
        clearDisplayBuffer();
        snprintf(error_buffer_, error_buffer_size, "Insufficient display_buffer size");
        displayText(false);
        return false;
    }

    pages.insertAtEnd(MENU::structs::PAGE_TYPE::ERROR, false, callback, error_buffer_, error_buffer_size);
    MENU::structs::errorPageInfo *page = error_pages.getLastAccessedNodeStoragePtr();
    if (page)
    {
        page->max_chars_on_line = calculateMaxCharsOnLine(error_buffer_, error_buffer_size); // Set max_chars_on_line
        num_error++;
        return true;
    }
    return false;
}

uint8_t OledMenu::calculateMaxCharsOnLine(char *buffer, uint16_t buffer_size)
{
    uint8_t max_chars = 0;
    uint8_t current_chars = 0;
    for (uint16_t i = 0; i < buffer_size; i++)
    {
        if (buffer[i] == '\n' || buffer[i] == '\0')
        {
            if (current_chars > max_chars)
            {
                max_chars = current_chars;
            }
            current_chars = 0;
            if (buffer[i] == '\0')
            {
                break;
            }
        }
        else
        {
            current_chars++;
        }
    }
    return max_chars;
}

/// @brief Refresh the display
void OledMenu::refreshDisplay()
{
    if (display_connected)
    {
        if (error_message_display_override)
        {
            renderErrorPageText();
        }
        else
        {
            renderMenuPageText();
        }
        manageCursorBlink();
    }
}

/// @brief Move to the next page
void OledMenu::moveToNextPage()
{
    if (current_page_displayed < num_pages)
    {
        current_page_displayed++;
    }
    if (current_page_displayed == num_pages)
    {
        current_page_displayed = 0;
    }
}

/// @brief Move to the previous page
void OledMenu::moveToPreviousPage()
{
    if (current_page_displayed > 0)
    {
        current_page_displayed--;
        return;
    }
    if (current_page_displayed == 0)
    {
        current_page_displayed = num_pages - 1;
    }
}

/// @brief Move up an item in the menu
void OledMenu::moveUpMenuItem()
{
    page_info = getMenuPageInfo(current_page_displayed);

    if (page_info->page_line > 0)
    {
        page_info->page_line--;
        return;
    }
    if (page_info->page_line == 0)
    {
        page_info->page_line = page_info->num_lines - 1;
    }
}

/// @brief Move down an item in the menu
void OledMenu::moveDownMenuItem()
{
    page_info = getMenuPageInfo(current_page_displayed);
    if (page_info->page_line < page_info->num_lines)
    {
        page_info->page_line++;
    }
    if (page_info->page_line == page_info->num_lines)
    {
        page_info->page_line = 0;
    }
}

/// @brief Clear the display buffer
void OledMenu::clearDisplayBuffer()
{
    memset(&display_buffer, '\0', display_buffer_size);
}

/// @brief Clear the page buffer
void OledMenu::clearPageBuffer()
{
    memset(&page_buffer_, '\0', page_buffer_size);
}

/// @brief Acknowledge an error
void OledMenu::acknowledgeError()
{
    if (num_error > 0)
    {
        num_error--;
    }
    if (num_error == 0)
    {
        OledMenu::error_message_display_override = false;
    }
}

/// @brief Check if there is an active error
/// @return True if there is an active error, false otherwise
bool OledMenu::hasActiveError()
{
    return OledMenu::error_message_display_override;
}

/// @brief Show an error message
/// @param fmt Format string for the error message
/// @param ... Additional arguments for the format string
/// @return Length of the error message
int OledMenu::showErrorMessage(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(NULL, 0, fmt, args);

    if (len >= error_buffer_size)
    {
        clearDisplayBuffer();
        snprintf(error_buffer_, error_buffer_size, "Insufficient display_buffer size");
        displayText(false);
        va_end(args);
        return -1;
    }

    va_start(args, fmt);
    vsnprintf(error_buffer_, len + 1, fmt, args);
    va_end(args);

    return len;
}

/// @brief Check if a page is entered
/// @return True if a page is entered, false otherwise
bool OledMenu::isPageEntered()
{
    return page_entered;
}

/// @brief Exit the current page
void OledMenu::exitCurrentPage()
{
    page_entered = false;
}

/// @brief Check if the current page is interactive
/// @return True if the current page is interactive, false otherwise
bool OledMenu::isCurrentPageInteractive()
{
    return pages.getStoragePtr(current_page_displayed)->interactive;
}

/// @brief Enter the current page
/// @return True if the page was entered successfully, false otherwise
bool OledMenu::enterCurrentPage()
{
    if (isCurrentPageInteractive())
    {
        page_entered = true;
        return true;
    }
    return false;
}

/// @brief Get the menu page info for a given page
/// @param page Index of the page
/// @return Pointer to the menu page info
MENU::structs::menuPageInfo *OledMenu::getMenuPageInfo(uint8_t page)
{
    return pages.getStoragePtr(page);
}

/// @brief Get the error page info for a given page
/// @param page Index of the page
/// @return Pointer to the error page info
MENU::structs::errorPageInfo *OledMenu::getErrorPageInfo(uint8_t page)
{
    return error_pages.getStoragePtr(page);
}

/// @brief Set the text to be displayed and scrolled.
/// @param txt The text to be displayed.
void OledMenu::setText(const char *txt)
{
    size_t len = strlen(txt) + 1;
    if (len > display_buffer_size)
    {
        clearDisplayBuffer();
        snprintf(error_buffer_, error_buffer_size, "Insufficient display_buffer size");
        displayText(false);
        return;
    }
    strncpy(reinterpret_cast<char *>(display_buffer.allocate(len)), txt, len);
    buffer = reinterpret_cast<char *>(display_buffer.allocate(len));
    bufferSize = len;
}

/// @brief Set the text to be displayed and scrolled using an external buffer.
/// @param buf The buffer to hold the text.
/// @param bufSize The size of the buffer.
void OledMenu::setText(char *buf, size_t bufSize)
{
    if (bufSize > display_buffer_size)
    {
        clearDisplayBuffer();
        snprintf(error_buffer_, error_buffer_size, "Insufficient display_buffer size");
        displayText(false);
        return;
    }
    buffer = buf;
    bufferSize = bufSize;
}

/// @brief Manage the blinking state of the cursor.
void OledMenu::manageCursorBlink()
{
    unsigned long currentTime = millis();
    if (blinkEnabled && (currentTime - lastBlinkTime >= blinkInterval))
    {
        blinkState = !blinkState;
        lastBlinkTime = currentTime;
    }
}

/// @brief Blink the text at the cursor position.
void OledMenu::blinkTextAtCursorPosition()
{
    displayText(blinkState);
}

/// @brief Set the number of display lines.
/// @param num_lines Number of lines to display.
void OledMenu::setNumberOfDisplayLines(int num_lines)
{
    if (num_lines >= minLines && num_lines <= maxLines)
    {
        dispLines = num_lines;
    }
    else if (num_lines < minLines)
    {
        dispLines = minLines;
    }
    else if (num_lines > maxLines)
    {
        dispLines = maxLines;
    }
}

/// @brief Get the number of display lines.
/// @return Number of lines to display.
int OledMenu::getNumberOfDisplayLines()
{
    return dispLines;
}

/// @brief Set the font size based on the number of lines to be displayed.
void OledMenu::setFontSizeForLineLimits()
{
    uint8_t fontPixelHeight = maxHeight / dispLines;
    if (fontPixelHeight >= fontMinPixelHeight && fontPixelHeight <= fontMaxPixelHeight)
    {
        // Ensure the lookup table is defined and within bounds
        uint8_t index = fontPixelHeight - fontMinPixelHeight;
        if (index < sizeof(u8g2_font_lookup_table) / sizeof(u8g2_font_lookup_table[0]))
        {
            display_hal.setFont(u8g2_font_lookup_table[index]);
        }
        else
        {
            // Handle out-of-bounds index
            display_hal.setFont(u8g2_font_helvB08_tf); // Default font as fallback
        }
    }
    else
    {
        // Handle fontPixelHeight out of range
        display_hal.setFont(u8g2_font_helvB08_tf); // Default font as fallback
    }
}

/// @brief Get the current X position of the cursor.
/// @return The X position of the cursor.
int OledMenu::getCursorXPosition()
{
    return page_info->cursorX;
}

/// @brief Get the current Y position of the cursor.
/// @return The Y position of the cursor.
int OledMenu::getCursorYPosition()
{
    return page_info->cursorY;
}

/// @brief Get the width of a character in the current font.
/// @return The width of a character.
int OledMenu::getFontCharacterWidth()
{
    return display_hal.getMaxCharWidth();
}

/// @brief Set the display anchor position.
/// @param x X position of the anchor.
/// @param y Y position of the anchor.
void OledMenu::setDisplayAnchor(int x, int y)
{
    page_info->anchorX = x;
    page_info->anchorY = y + display_hal.getMaxCharHeight();
}

/// @brief Get the anchor position.
/// @param x X position of the anchor.
/// @param y Y position of the anchor.
void OledMenu::getDisplayAnchor(int &x, int &y)
{
    x = page_info->anchorX;
    y = page_info->anchorY;
}

/// @brief Set the cursor position.
/// @param x X position of the cursor.
/// @param y Y position of the cursor.
void OledMenu::setCursorPosition(int x, int y)
{
    page_info->cursorX = x;
    page_info->cursorY = y;
}

/// @brief Get cursor position.
/// @param x X position of the cursor.
/// @param y Y position of the cursor.
void OledMenu::getCursorPosition(int &x, int &y)
{
    x = page_info->cursorX;
    y = page_info->cursorY;
}

void OledMenu::scroll(int x, int y)
{
    // Calculate text width and height based on max_chars_on_line and num_lines
    int textWidth = getFontCharacterWidth() * page_info->max_chars_on_line;
    int textHeight = display_hal.getMaxCharHeight() * page_info->num_lines;

    // Calculate new cursor position
    int newCursorX = page_info->cursorX + x;
    int newCursorY = page_info->cursorY + y;

    // Ensure cursor stays within the bounds of the display
    if (newCursorX < 0)
    {
        newCursorX = 0;
    }
    else if (newCursorX > maxWidth - getFontCharacterWidth())
    {
        newCursorX = maxWidth - getFontCharacterWidth();
    }

    if (newCursorY < 0)
    {
        newCursorY = 0;
    }
    else if (newCursorY > maxHeight - display_hal.getMaxCharHeight())
    {
        newCursorY = maxHeight - display_hal.getMaxCharHeight();
    }

    // Update cursor position
    page_info->cursorX = newCursorX;
    page_info->cursorY = newCursorY;

    // Calculate offsets if cursor is at the edge
    int offsetX = page_info->anchorX;
    int offsetY = page_info->anchorY;

    if (newCursorX == 0 || newCursorX == maxWidth - getFontCharacterWidth())
    {
        offsetX = (maxWidth - textWidth) / 2 + x;
    }

    if (newCursorY == 0 || newCursorY == maxHeight - display_hal.getMaxCharHeight())
    {
        offsetY = (maxHeight - textHeight) / 2 + y;
    }

    // Store offsets for later use in rendering
    page_info->anchorX = offsetX;
    page_info->anchorY = offsetY;
}

/// @brief Render text for the current menu page
void OledMenu::renderMenuPageText()
{
    page_info = getMenuPageInfo(current_page_displayed);
    if (page_info->callback)
    {
        page_info->callback(page_info);
    }
    buffer = page_info->buffer;
    bufferSize = page_info->needs_buffer_size;
    displayText(false);
}

/// @brief Render text for the current error page
void OledMenu::renderErrorPageText()
{
    MENU::structs::errorPageInfo *error_page_info = getErrorPageInfo(current_page_displayed);
    buffer = error_page_info->buffer;
    bufferSize = error_page_info->needs_buffer_size;
    displayText(false);
}

/// @brief Function to display connection information on the OLED menu
/// @param page_info Pointer to the menuPageInfo struct
void MENU::builtin_pages::connectionInfo(MENU::structs::menuPageInfo *page_info)
{
    IPAddress ip = WiFi.localIP();
    const char *page = "%s\n"
                       "%d.%d.%d.%d\n"
                       "RSSI: %d\n"
                       "%s\n";

    sprintf(page_info->buffer, page, WiFi.SSID().c_str(), ip[0], ip[1], ip[2], ip[3], WiFi.RSSI(), WiFi.getHostname());
}

/// @brief Function to display OTA update information on the OLED menu
/// @param page Pointer to the menuPageInfo struct
void MENU::builtin_pages::OTAInfo(MENU::structs::menuPageInfo *page)
{
    static uint32_t spinner_timer = millis();                   ///< Timer for spinner animation
    static byte spinner = 0;                                    ///< Spinner index
    const char *spinner_text[] = {" | ", " / ", "---", " \\ "}; ///< Spinner text
    const char *page_text = "Updating... %s\n"
                            "Progress: %lu%%\n%s%s";

    // Update spinner animation every 100 milliseconds
    if ((millis() - spinner_timer) >= 100)
    {
        spinner = (spinner + 1) % NELEMS(spinner_text);
        spinner_timer = millis();
    }
    uint8_t *parameters = reinterpret_cast<uint8_t *>(page->parameters);
    // Calculate progress percentage, protect against division by zero
    int progress = (parameters[0] != 0) ? (parameters[0] / (parameters[1] / 100)) : 0;

    // Format the page content with spinner, progress, and status messages
    int len = sprintf_P(page->buffer, page_text, spinner_text[spinner], progress,
                        (progress == 100) ? "Update Complete." : "",
                        (progress == 100) ? "Restarting..." : "");
}