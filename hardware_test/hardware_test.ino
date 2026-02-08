/*
 * Hardware Test for ESP Flight Tracker
 * CYD (Cheap Yellow Display) Version
 * 
 * This sketch tests the basic hardware components:
 * - TFT Display (ILI9341 320x240)
 * - Touch Screen (XPT2046 via TFT_eSPI or standalone)
 * - RGB LED (if present)
 * - SD Card
 * - WiFi
 * 
 * IMPORTANT: TFT_eSPI Configuration
 * ===================================
 * The TFT_eSPI library requires configuration in its User_Setup.h file.
 * 
 * For Arduino IDE:
 * 1. Go to: Arduino/libraries/TFT_eSPI/
 * 2. Edit User_Setup.h
 * 3. Comment out any existing configuration
 * 4. Add the CYD configuration shown below
 * 
 * For PlatformIO:
 * 1. Go to: .pio/libdeps/esp32dev/TFT_eSPI/
 * 2. Edit User_Setup.h
 * 3. Add the configuration below
 * 
 * CYD Configuration for User_Setup.h:
 * ------------------------------------
 * #define ILI9341_2_DRIVER  // Use alternative driver for CYD32S024C
 * #define TFT_MISO 12
 * #define TFT_MOSI 13
 * #define TFT_SCLK 14
 * #define TFT_CS   15
 * #define TFT_DC    2
 * #define TFT_RST  -1
 * #define TFT_BL   27  // Backlight GPIO
 * #define TFT_BACKLIGHT_ON HIGH
 * 
 * // Enable touch support (optional but recommended)
 * #define TOUCH_CS 33     // Touch chip select pin
 * #define TOUCH_DRIVER 0x2046  // XPT2046 driver (optional for TFT_eSPI touch)
 * 
 * #define TFT_RGB_ORDER TFT_RGB
 * #define TFT_INVERSION_ON
 * 
 * #define LOAD_GLCD
 * #define LOAD_FONT2
 * #define LOAD_FONT4
 * #define LOAD_FONT6
 * #define LOAD_FONT7
 * #define LOAD_FONT8
 * #define LOAD_GFXFF
 * #define SMOOTH_FONT
 * 
 * #define SPI_FREQUENCY  40000000
 * #define SPI_READ_FREQUENCY  20000000
 * #define SPI_TOUCH_FREQUENCY  2500000
 */

#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>

// Check if touch is enabled in TFT_eSPI
#ifdef TOUCH_CS
  #define USE_TFT_ESPI_TOUCH 1
  // Touch calibration data for your CYD32S024C
  // Updated based on your touch readings
  uint16_t calData[5] = { 270, 3578, 305, 3481, 1 };  // Changed last value from 0 to 1
#else
  #define USE_TFT_ESPI_TOUCH 0
  #include <XPT2046_Touchscreen.h>
  #define TOUCH_CS_PIN 33
  #define TOUCH_IRQ_PIN 36
  XPT2046_Touchscreen touch(TOUCH_CS_PIN, TOUCH_IRQ_PIN);
#endif

// Pin definitions for CYD (ESP32-2432S028R / CYD32S024C)
#define SD_CS 5         // SD card chip select
#define BACKLIGHT_PIN 27 // Backlight control

// Optional CYD hardware
#define RGB_LED_R 4     // RGB LED Red (Type-C variant)
#define RGB_LED_G 16    // RGB LED Green  
#define RGB_LED_B 17    // RGB LED Blue
#define LIGHT_SENSOR 34 // Light sensor (analog input)
#define SPEAKER 26      // Speaker output (DAC)
#define BOOT_BUTTON 0   // Boot button

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=== ESP Flight Tracker Hardware Test ===");
  Serial.println("=== CYD32S024C Type-C Variant ===\n");
  
  // Test 1: TFT Display
  Serial.println("Testing TFT Display...");
  Serial.println("Using ILI9341_2_DRIVER for proper initialization");
  
  // Initialize display
  tft.init();
  
  // Turn on backlight
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);
  Serial.printf("Backlight turned on (GPIO %d)\n", BACKLIGHT_PIN);
  
  // Test all rotations
  Serial.println("\nTesting display rotations:");
  for (int rot = 0; rot < 4; rot++) {
    tft.setRotation(rot);
    Serial.printf("  Rotation %d: Width=%d, Height=%d\n", rot, tft.width(), tft.height());
  }
  
  // Set to landscape (rotation 1 for most CYD boards)
  tft.setRotation(1);
  Serial.printf("\nUsing rotation 1 (landscape): %dx%d\n", tft.width(), tft.height());
  
  // Fill screen with test colors
  Serial.println("Testing color display...");
  tft.fillScreen(TFT_RED);
  delay(300);
  tft.fillScreen(TFT_GREEN);
  delay(300);
  tft.fillScreen(TFT_BLUE);
  delay(300);
  
  // Clear and draw test pattern
  tft.fillScreen(TFT_BLACK);
  
  // Draw border to show full screen area (2 pixels inside to be visible)
  tft.drawRect(0, 0, tft.width(), tft.height(), TFT_WHITE);
  tft.drawRect(1, 1, tft.width()-2, tft.height()-2, TFT_CYAN);
  
  // Draw colored rectangles in each corner
  tft.fillRect(0, 0, 60, 40, TFT_RED);
  tft.fillRect(tft.width()-60, 0, 60, 40, TFT_GREEN);
  tft.fillRect(0, tft.height()-40, 60, 40, TFT_BLUE);
  tft.fillRect(tft.width()-60, tft.height()-40, 60, 40, TFT_YELLOW);
  
  // Draw center cross
  tft.drawLine(tft.width()/2, 0, tft.width()/2, tft.height(), TFT_MAGENTA);
  tft.drawLine(0, tft.height()/2, tft.width(), tft.height()/2, TFT_MAGENTA);
  
  // Display header
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("CYD Hardware Test");
  
  // Display info
  tft.setTextSize(1);
  tft.setCursor(10, 35);
  tft.printf("Display: %dx%d pixels", tft.width(), tft.height());
  tft.setCursor(10, 45);
  tft.println("Driver: ILI9341_2");
  tft.setCursor(10, 55);
  tft.println("Touch: XPT2046");
  tft.setCursor(10, 65);
  tft.printf("Backlight: GPIO %d", BACKLIGHT_PIN);
  
  Serial.println("Display test complete\n");
  
  // Test 2: Touch Screen
  Serial.println("Testing Touch Screen...");
  
#if USE_TFT_ESPI_TOUCH == 1
  // Using TFT_eSPI touch support
  Serial.println("  Attempting to use TFT_eSPI integrated touch support");
  
  // Initialize touch with calibration data
  tft.setTouch(calData);
  
  // Test if touch is actually working
  uint16_t test_x = 0, test_y = 0;
  bool touch_works = false;
  
  // Try to get a touch reading to verify it works
  for (int i = 0; i < 5; i++) {
    if (tft.getTouch(&test_x, &test_y, 100)) { // 100ms threshold
      touch_works = true;
      break;
    }
    delay(10);
  }
  
  if (touch_works || true) { // Force to try even if initial test fails
    Serial.println("  TFT_eSPI touch initialized with calibration data:");
    Serial.printf("    { %d, %d, %d, %d, %d }\n", 
                  calData[0], calData[1], calData[2], calData[3], calData[4]);
    Serial.println("  Note: If touch doesn't work, check TOUCH_CS wiring");
  }
#else
  // Using standalone XPT2046 library
  Serial.println("  Using standalone XPT2046 library");
  Serial.println("  Note: For better integration, add to User_Setup.h:");
  Serial.println("    #define TOUCH_CS 33");
  
  touch.begin();
  touch.setRotation(1); // Match display rotation
  Serial.println("  XPT2046 touch initialized");
#endif
  
  // Touch test display
  tft.setTextSize(1);
  tft.setCursor(10, 85);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.println("Touch the screen to test");
  
  // Draw calibration targets
  drawCalibrationTarget(20, 20, TFT_RED);        // Top-left
  drawCalibrationTarget(tft.width()-20, 20, TFT_GREEN); // Top-right
  drawCalibrationTarget(20, tft.height()-20, TFT_BLUE);  // Bottom-left
  drawCalibrationTarget(tft.width()-20, tft.height()-20, TFT_YELLOW); // Bottom-right
  drawCalibrationTarget(tft.width()/2, tft.height()/2, TFT_WHITE); // Center
  
  delay(1000); // Give user time to see the targets
  
  // Test 3: SD Card
  Serial.println("\nTesting SD Card...");
  tft.setCursor(10, 105);
  
  // The CYD uses separate SPI bus for SD card
  // SD_CS = 5, SD_MOSI = 23, SD_MISO = 19, SD_CLK = 18
  bool sdOK = false;
  
  // Method 1: Try with custom SPI pins at slow speed
  Serial.println("  Trying custom SPI pins at 1MHz...");
  SPI.end(); // End default SPI first
  SPI.begin(18, 19, 23, 5); // SCK, MISO, MOSI, SS for SD card
  
  if (SD.begin(SD_CS, SPI, 1000000)) { // 1MHz for compatibility
    Serial.println("  SD Card OK with custom SPI at 1MHz");
    sdOK = true;
  } else {
    // Method 2: Try VSPI with explicit pins
    Serial.println("  Trying VSPI configuration...");
    SPIClass sdSPI(VSPI);
    sdSPI.begin(18, 19, 23, -1); // SCK, MISO, MOSI, -1 for SS
    
    if (SD.begin(SD_CS, sdSPI, 1000000)) {
      Serial.println("  SD Card OK with VSPI at 1MHz");
      sdOK = true;
    } else {
      // Method 3: Try default SPI with just CS pin
      Serial.println("  Trying default configuration...");
      SPI.begin(); // Reset to default pins
      
      if (SD.begin(SD_CS, SPI, 4000000)) {
        Serial.println("  SD Card OK with default SPI");
        sdOK = true;
      } else {
        Serial.println("  SD Card FAILED - Possible issues:");
        Serial.println("    - Card not formatted as FAT32/FAT16");
        Serial.println("    - Card size > 32GB (use SDHC formatted as FAT32)");
        Serial.println("    - Try different SD card brand");
        Serial.println("    - Check card works in computer");
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.println("SD Card: FAILED (try FAT32 format)");
      }
    }
  }
  
  if (sdOK) {
    showSDInfo();
  }
  
  // Restore default SPI for display/touch
  SPI.begin(14, 12, 13, 15); // SCK, MISO, MOSI, SS for display
  
  // Test 4: WiFi
  Serial.println("\nTesting WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  Serial.println("  Scanning for networks...");
  int n = WiFi.scanNetworks();
  
  tft.setCursor(10, 125);
  if (n == 0) {
    Serial.println("  No networks found");
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.println("WiFi: No networks found");
  } else {
    Serial.printf("  Found %d networks:\n", n);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.printf("WiFi: %d networks found", n);
    
    for (int i = 0; i < min(5, n); ++i) {
      Serial.printf("    %d: %s (RSSI: %d dBm)\n", i + 1, 
                    WiFi.SSID(i).c_str(), WiFi.RSSI(i));
    }
  }
  
  // Test 5: RGB LED (if present)
  Serial.println("\nTesting RGB LED (Type-C variant)...");
  pinMode(RGB_LED_R, OUTPUT);
  
  // Quick blink test
  digitalWrite(RGB_LED_R, HIGH);
  delay(200);
  digitalWrite(RGB_LED_R, LOW);
  Serial.println("  RGB LED test complete (red blink)");
  
  // Test 6: Memory
  Serial.println("\nSystem Information:");
  Serial.printf("  Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("  Chip Model: %s\n", ESP.getChipModel());
  Serial.printf("  Chip Cores: %d\n", ESP.getChipCores());
  Serial.printf("  CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("  Flash Size: %d bytes\n", ESP.getFlashChipSize());
  
  tft.setTextSize(1);
  tft.setCursor(10, 145);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.printf("Free RAM: %d KB", ESP.getFreeHeap() / 1024);
  
  // Test complete
  Serial.println("\n=== Hardware Test Complete ===");
  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(10, 165);
  tft.println("Test Complete!");
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(10, 190);
  tft.println("Touch screen to see coordinates");
  
  // Draw grid for touch calibration reference
  tft.drawLine(tft.width()/4, 0, tft.width()/4, tft.height(), TFT_DARKGREY);
  tft.drawLine(tft.width()/2, 0, tft.width()/2, tft.height(), TFT_DARKGREY);
  tft.drawLine(3*tft.width()/4, 0, 3*tft.width()/4, tft.height(), TFT_DARKGREY);
  tft.drawLine(0, tft.height()/4, tft.width(), tft.height()/4, TFT_DARKGREY);
  tft.drawLine(0, tft.height()/2, tft.width(), tft.height()/2, TFT_DARKGREY);
  tft.drawLine(0, 3*tft.height()/4, tft.width(), 3*tft.height()/4, TFT_DARKGREY);
}

void showSDInfo() {
  uint8_t cardType = SD.cardType();
  Serial.print("  Card Type: ");
  
  String cardTypeStr;
  if (cardType == CARD_MMC) {
    cardTypeStr = "MMC";
  } else if (cardType == CARD_SD) {
    cardTypeStr = "SDSC";
  } else if (cardType == CARD_SDHC) {
    cardTypeStr = "SDHC";
  } else {
    cardTypeStr = "UNKNOWN";
  }
  Serial.println(cardTypeStr);
  
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("  Card Size: %llu MB\n", cardSize);
  Serial.printf("  Used Space: %llu MB\n", SD.usedBytes() / (1024 * 1024));
  
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(10, 105);
  tft.printf("SD Card: OK (%s, %llu MB)", cardTypeStr.c_str(), cardSize);
}

void drawCalibrationTarget(int x, int y, uint16_t color) {
  tft.drawCircle(x, y, 10, color);
  tft.drawCircle(x, y, 8, color);
  tft.drawLine(x-15, y, x+15, y, color);
  tft.drawLine(x, y-15, x, y+15, color);
}

void loop() {
#if USE_TFT_ESPI_TOUCH == 1
  // Check for touch events using TFT_eSPI
  uint16_t x = 0, y = 0;
  bool touched = tft.getTouch(&x, &y);
  
  if (touched) {
    // Touch coordinates are already mapped to screen coordinates by TFT_eSPI
    Serial.printf("Touch: X=%d, Y=%d\n", x, y);
    
    // Clear previous touch point display area
    tft.fillRect(0, 210, tft.width(), 30, TFT_BLACK);
    
    // Display touch coordinates
    tft.setTextSize(1);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(10, 215);
    tft.printf("Touch: X=%d Y=%d", x, y);
    
    // Draw touch point
    static int lastX = -1, lastY = -1;
    if (lastX >= 0 && lastY >= 0) {
      tft.fillCircle(lastX, lastY, 3, TFT_BLACK);
      // Redraw grid lines if they were covered
      if (abs(lastX - tft.width()/4) < 4) tft.drawLine(tft.width()/4, 0, tft.width()/4, tft.height(), TFT_DARKGREY);
      if (abs(lastX - tft.width()/2) < 4) tft.drawLine(tft.width()/2, 0, tft.width()/2, tft.height(), TFT_DARKGREY);
      if (abs(lastX - 3*tft.width()/4) < 4) tft.drawLine(3*tft.width()/4, 0, 3*tft.width()/4, tft.height(), TFT_DARKGREY);
      if (abs(lastY - tft.height()/4) < 4) tft.drawLine(0, tft.height()/4, tft.width(), tft.height()/4, TFT_DARKGREY);
      if (abs(lastY - tft.height()/2) < 4) tft.drawLine(0, tft.height()/2, tft.width(), tft.height()/2, TFT_DARKGREY);
      if (abs(lastY - 3*tft.height()/4) < 4) tft.drawLine(0, 3*tft.height()/4, tft.width(), 3*tft.height()/4, TFT_DARKGREY);
    }
    
    tft.fillCircle(x, y, 3, TFT_MAGENTA);
    lastX = x;
    lastY = y;
    
    // Check if touch is on a calibration target
    if (abs(x - 20) < 15 && abs(y - 20) < 15) {
      Serial.println("  Top-left corner touched");
    } else if (abs(x - (tft.width()-20)) < 15 && abs(y - 20) < 15) {
      Serial.println("  Top-right corner touched");
    } else if (abs(x - 20) < 15 && abs(y - (tft.height()-20)) < 15) {
      Serial.println("  Bottom-left corner touched");
    } else if (abs(x - (tft.width()-20)) < 15 && abs(y - (tft.height()-20)) < 15) {
      Serial.println("  Bottom-right corner touched");
    } else if (abs(x - tft.width()/2) < 15 && abs(y - tft.height()/2) < 15) {
      Serial.println("  Center touched");
    }
    
    delay(50); // Short debounce
  }
  
#else
  // Using standalone XPT2046 library
  if (touch.touched()) {
    TS_Point p = touch.getPoint();
    
    // Check for valid touch
    if (p.x > 100 && p.x < 4000 && p.y > 100 && p.y < 4000 && p.z > 200) {
      // Map touch coordinates to screen coordinates
      int x = map(p.y, 305, 3481, 0, tft.width());
      int y = map(p.x, 270, 3578, tft.height(), 0);
      
      // Constrain to screen bounds
      x = constrain(x, 0, tft.width() - 1);
      y = constrain(y, 0, tft.height() - 1);
      
      Serial.printf("Touch: X=%d, Y=%d (Raw: %d,%d)\n", x, y, p.x, p.y);
      
      // Clear previous touch point display area
      tft.fillRect(0, 210, tft.width(), 30, TFT_BLACK);
      
      // Display touch coordinates
      tft.setTextSize(1);
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.setCursor(10, 215);
      tft.printf("Touch: X=%d Y=%d", x, y);
      
      // Draw touch point
      tft.fillCircle(x, y, 3, TFT_MAGENTA);
    }
    
    delay(50); // Short debounce
  }
#endif
  
  // Blink status indicator
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  
  if (millis() - lastBlink > 1000) {
    ledState = !ledState;
    
    // Draw a status indicator on screen
    if (ledState) {
      tft.fillCircle(tft.width() - 10, 10, 5, TFT_GREEN);
    } else {
      tft.fillCircle(tft.width() - 10, 10, 5, TFT_BLACK);
      tft.drawCircle(tft.width() - 10, 10, 5, TFT_GREEN);
    }
    
    // Also blink the RGB LED if present
    digitalWrite(RGB_LED_R, ledState ? HIGH : LOW);
    
    lastBlink = millis();
  }
}