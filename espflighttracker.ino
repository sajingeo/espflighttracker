/*
 * ESP Flight Tracker Clock
 * For ESP32 with 2.4" TFT Touch Screen (CYD - Cheap Yellow Display)
 * 
 * Features:
 * - Real-time clock with NTP sync
 * - Weather display with OpenWeatherMap API
 * - Flight tracking with OpenSky Network API
 * - Web-based configuration portal
 * - Touch screen interface
 * 
 * IMPORTANT: TFT_eSPI Library Configuration Required!
 * ====================================================
 * The TFT_eSPI library MUST be configured by editing its User_Setup.h file.
 * Defining configuration in the sketch does NOT work reliably.
 * 
 * Steps to configure:
 * 1. Find the TFT_eSPI library folder:
 *    - Arduino IDE: Arduino/libraries/TFT_eSPI/
 *    - PlatformIO: .pio/libdeps/esp32dev/TFT_eSPI/
 * 
 * 2. Edit User_Setup.h and add this configuration:
 * 
 * #define ILI9341_2_DRIVER  // Use alternative driver for CYD32S024C
 * #define TFT_MISO 12
 * #define TFT_MOSI 13
 * #define TFT_SCLK 14
 * #define TFT_CS   15
 * #define TFT_DC    2
 * #define TFT_RST  -1
 * #define TFT_BL   27  // Backlight GPIO 27
 * #define TFT_BACKLIGHT_ON HIGH
 * 
 * #define TOUCH_CS 33  // Touch chip select
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
 * 
 * 3. Save the file and compile this sketch
 */

// ==========================================
// Include Libraries
// ==========================================
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <SD.h>

// ==========================================
// Hardware Pin Definitions
// ==========================================
// Check if touch is enabled in TFT_eSPI
#ifdef TOUCH_CS
  #define USE_TFT_ESPI_TOUCH 1
  // Touch calibration data for CYD32S024C
  uint16_t calData[5] = { 270, 3578, 305, 3481, 1 };
#else
  #define USE_TFT_ESPI_TOUCH 0
  #include <XPT2046_Touchscreen.h>
  #define TOUCH_CS_PIN 33
  #define TOUCH_IRQ_PIN 36
  XPT2046_Touchscreen touch(TOUCH_CS_PIN, TOUCH_IRQ_PIN);
#endif

// SD Card and other hardware
#define SD_CS 5
#define BACKLIGHT_PIN 27 // Backlight control

// Optional CYD hardware
#define RGB_LED_R 4     // RGB LED Red (Type-C variant)
#define RGB_LED_G 16    // RGB LED Green
#define RGB_LED_B 17    // RGB LED Blue
#define LIGHT_SENSOR 34 // Light sensor (analog)
#define SPEAKER 26      // Speaker output (DAC)
#define BOOT_BUTTON 0   // Boot button

// Create display object
TFT_eSPI tft = TFT_eSPI();

// WiFi AP Configuration
const char* AP_SSID = "FlightTracker";
const char* AP_PASSWORD = "12345678";

// Web Server
WebServer server(80);
DNSServer dnsServer;

// Preferences for persistent storage
Preferences preferences;

// Configuration variables
String userName = "";
String wifiSSID = "";
String wifiPassword = "";
String weatherAPIKey = "";
String flightAwareAPIKey = "";  // FlightAware API key
float latitude = 0.0;
float longitude = 0.0;
String locationName = "";

// State variables
bool isConfigured = false;
bool isAPMode = false;
int wifiRetryCount = 0;
const int MAX_WIFI_RETRIES = 3;

// Time variables
time_t lastWeatherUpdate = 0;

// Weather data
float temperature = 0.0;
int humidity = 0;
String weatherDescription = "";
String weatherIcon = "";

// Flight data structure
struct FlightInfo {
  String callsign;
  String airline;
  String flightNumber;
  String aircraft;
  String origin;
  String originCity;
  String destination;
  String destinationCity;
  float altitude;
  float distance;
  float heading;
  float speed;
  float latitude;
  float longitude;
};

FlightInfo nearbyFlights[3];
int flightCount = 0;

// Function declarations
void setupDisplay();
void setupTouch();
void loadConfiguration();
void saveConfiguration();
void startAPMode();
void startWebServer();
void connectToWiFi();
void updateTime();
void updateWeather();
void updateFlights();
void updateFlightsOpenSky();
void parseFlightAwareData(String json);
void parseOpenSkyData(String json);
float calculateDistance(float lat1, float lon1, float lat2, float lon2);
void sortFlightsByDistance();
String getAirlineFromCallsign(String callsign);
void drawWelcomeScreen();
void drawMainScreen();
void drawWeatherIcon(int x, int y, String icon);
void drawFlightInfo(int x, int y, FlightInfo& flight);
void drawAirplane(int x, int y);
void drawAPModeScreen();
void handleTouch();
void handleRoot();
void handleSave();
void handleReset();
String getConfigHTML();
String htmlEncode(String str);

void setup() {
  Serial.begin(115200);
  Serial.println("ESP Flight Tracker Starting...");
  
  // Initialize display
  setupDisplay();
  
  // Initialize touch
  setupTouch();
  
  // Initialize RGB LED (Type-C variant on GPIO 4)
  pinMode(RGB_LED_R, OUTPUT);
  pinMode(RGB_LED_G, OUTPUT);
  pinMode(RGB_LED_B, OUTPUT);
  
  // Turn off RGB LED (active LOW on Type-C variant)
  digitalWrite(RGB_LED_R, HIGH);
  digitalWrite(RGB_LED_G, HIGH);
  digitalWrite(RGB_LED_B, HIGH);
  
  // Load saved configuration
  loadConfiguration();
  
  // Show welcome screen
  drawWelcomeScreen();
  delay(3000);
  
  if (isConfigured) {
    // Try to connect to WiFi
    connectToWiFi();
    
    if (WiFi.status() == WL_CONNECTED) {
      // Start web server for configuration
      startWebServer();
      
      // Configure time for EST (Boston)
      // EST is UTC-5, EDT is UTC-4
      // Using POSIX timezone string for automatic DST handling
      configTime(0, 0, "pool.ntp.org", "time.nist.gov");
      setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
      tzset();
      
      // Initial data fetch
      updateTime();
      updateWeather();
      // Don't fetch flights automatically - wait for user touch
    } else {
      // Fall back to AP mode
      startAPMode();
    }
  } else {
    // First time setup - start AP mode
    startAPMode();
  }
}

void loop() {
  // Handle DNS requests in AP mode
  if (isAPMode) {
    dnsServer.processNextRequest();
  }
  
  // Handle web server
  server.handleClient();
  
  // Handle touch input
  handleTouch();
  
  // Update display based on mode
  if (isAPMode) {
    static unsigned long lastAPDraw = 0;
    if (millis() - lastAPDraw > 1000) {
      drawAPModeScreen();
      lastAPDraw = millis();
    }
  } else {
    // Main screen with time, weather, and flight
    static unsigned long lastMainDraw = 0;
    static int lastMinute = -1;
    
    // Update display every minute (when minute changes)
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      if (timeinfo.tm_min != lastMinute) {
        lastMinute = timeinfo.tm_min;
        drawMainScreen();
        updateTime();
        lastMainDraw = millis();
      }
    }
    
    // Update weather every hour (3600 seconds)
    if (time(nullptr) - lastWeatherUpdate > 3600) {
      updateWeather();
      lastWeatherUpdate = time(nullptr);
      drawMainScreen(); // Redraw to show new weather
    }
    
    // Flights are only updated on touch - no automatic refresh
  }
}

void setupDisplay() {
  tft.init();
  
  // Set to landscape mode (rotation 1 for CYD)
  tft.setRotation(1);
  
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  
  // Turn on backlight - GPIO 27 for CYD32S024C
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);
  
  // Set display properties
  tft.setTextWrap(false);
  
  // Display info
  Serial.printf("Display initialized: %dx%d pixels\n", tft.width(), tft.height());
  Serial.println("Driver: ILI9341_2");
}

void setupTouch() {
#if USE_TFT_ESPI_TOUCH == 1
  // Using TFT_eSPI touch support
  tft.setTouch(calData);
  Serial.println("Touch screen initialized with TFT_eSPI");
  Serial.printf("Calibration: { %d, %d, %d, %d, %d }\n", 
                calData[0], calData[1], calData[2], calData[3], calData[4]);
#else
  // Using standalone XPT2046 library
  touch.begin();
  touch.setRotation(1); // Match display rotation
  Serial.println("Touch screen initialized with XPT2046 library");
#endif
}

void loadConfiguration() {
  preferences.begin("flighttrack", false);
  
  isConfigured = preferences.getBool("configured", false);
  if (isConfigured) {
    userName = preferences.getString("userName", "");
    wifiSSID = preferences.getString("wifiSSID", "");
    wifiPassword = preferences.getString("wifiPass", "");
    weatherAPIKey = preferences.getString("weatherKey", "");
    flightAwareAPIKey = preferences.getString("flightKey", "");
    latitude = preferences.getFloat("latitude", 0.0);
    longitude = preferences.getFloat("longitude", 0.0);
    locationName = preferences.getString("location", "");
  }
  
  preferences.end();
}

void saveConfiguration() {
  preferences.begin("flighttrack", false);
  
  preferences.putBool("configured", true);
  preferences.putString("userName", userName);
  preferences.putString("wifiSSID", wifiSSID);
  preferences.putString("wifiPass", wifiPassword);
  preferences.putString("weatherKey", weatherAPIKey);
  preferences.putString("flightKey", flightAwareAPIKey);
  preferences.putFloat("latitude", latitude);
  preferences.putFloat("longitude", longitude);
  preferences.putString("location", locationName);
  
  isConfigured = true;
  
  preferences.end();
}

void startAPMode() {
  Serial.println("Starting AP Mode...");
  isAPMode = true;
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  // Start DNS server for captive portal
  dnsServer.start(53, "*", IP);
  
  // Start web server
  startWebServer();
  
  // Show AP info on screen
  drawAPModeScreen();
}

void drawAPModeScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN);
  tft.drawString("Setup Mode", 60, 20);
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Connect to WiFi:", 20, 60);
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString(AP_SSID, 20, 80);
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Password:", 20, 110);
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString(AP_PASSWORD, 20, 130);
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Then open browser to:", 20, 170);
  tft.setTextColor(TFT_GREEN);
  tft.drawString("http://192.168.4.1", 20, 190);
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  Serial.println("SSID: " + wifiSSID);
  Serial.println("Password length: " + String(wifiPassword.length()));
  
  // Disconnect any existing connection
  WiFi.disconnect(true);
  delay(100);
  
  WiFi.mode(WIFI_STA);
  
  // Enable WPA3 support if available (ESP32 2.0.0+)
  // This allows connection to WPA3 networks while maintaining WPA2 compatibility
  WiFi.setMinSecurity(WIFI_AUTH_WPA_PSK);  // Allow WPA, WPA2, and WPA3
  
  // Handle SSIDs with spaces or special characters
  // The SSID and password are already strings, so they should handle spaces correctly
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {  // Increased timeout for WPA3
    delay(500);
    Serial.print(".");
    attempts++;
    
    // Print status every 10 attempts
    if (attempts % 10 == 0) {
      Serial.print(" Status: ");
      switch(WiFi.status()) {
        case WL_NO_SSID_AVAIL:
          Serial.println("SSID not found");
          break;
        case WL_CONNECT_FAILED:
          Serial.println("Connection failed");
          break;
        case WL_CONNECTION_LOST:
          Serial.println("Connection lost");
          break;
        case WL_DISCONNECTED:
          Serial.println("Disconnected");
          break;
        default:
          Serial.println(WiFi.status());
      }
    }
    
    // Show connecting status
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Connecting to WiFi", 40, 100);
    
    String dots = "";
    for (int i = 0; i < (attempts % 4); i++) {
      dots += ".";
    }
    tft.drawString(dots, 40, 130);
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    isAPMode = false;
    wifiRetryCount = 0;
  } else {
    Serial.println("\nWiFi connection failed!");
    wifiRetryCount++;
    
    if (wifiRetryCount >= MAX_WIFI_RETRIES) {
      startAPMode();
    }
  }
}

void drawWelcomeScreen() {
  tft.fillScreen(TFT_BLACK);
  
  int screenWidth = tft.width();
  int screenHeight = tft.height();
  
  // Draw decorative border
  tft.drawRect(5, 5, screenWidth - 10, screenHeight - 10, TFT_CYAN);
  tft.drawRect(7, 7, screenWidth - 14, screenHeight - 14, TFT_BLUE);
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  
  if (isConfigured && userName.length() > 0) {
    String greeting = "Hello " + userName + "!";
    int textWidth = greeting.length() * 12; // Approximate width
    tft.drawString(greeting, (screenWidth - textWidth) / 2, 80);
  } else {
    tft.drawString("Welcome to", (screenWidth - 120) / 2, 60);
    tft.setTextColor(TFT_CYAN);
    tft.drawString("Flight Tracker Clock", (screenWidth - 240) / 2, 90);
    tft.setTextSize(1);
    tft.setTextColor(TFT_YELLOW);
    tft.drawString("by sajin", (screenWidth - 48) / 2, 120);
  }
  
  // Draw animated airplane
  for (int x = -20; x < screenWidth + 20; x += 5) {
    tft.fillRect(x - 25, 160, 50, 20, TFT_BLACK);
    drawAirplane(x, 160);
    delay(20);
  }
}

void drawAirplane(int x, int y) {
  // Simple airplane icon
  tft.fillTriangle(x, y + 10, x - 20, y + 5, x - 20, y + 15, TFT_WHITE);
  tft.fillRect(x - 20, y + 8, 15, 4, TFT_WHITE);
  tft.fillTriangle(x - 10, y, x - 15, y + 8, x - 5, y + 8, TFT_WHITE);
  tft.fillTriangle(x - 10, y + 20, x - 15, y + 12, x - 5, y + 12, TFT_WHITE);
}

// ============================================
// Part 2: Display and Data Processing Functions
// ============================================

void drawMainScreen() {
  // Don't clear entire screen to reduce flicker - use background colors
  int screenWidth = tft.width();
  int screenHeight = tft.height();
  
  // Clear only the areas that change
  tft.fillRect(0, 0, screenWidth, 50, TFT_BLACK); // Time area
  tft.fillRect(0, 50, screenWidth, 70, TFT_BLACK); // Date and weather area
  tft.fillRect(0, 120, screenWidth, 100, TFT_BLACK); // Flight area
  
  // Draw time - larger and centered (no seconds)
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char timeStr[10];
    strftime(timeStr, sizeof(timeStr), "%I:%M", &timeinfo); // 12-hour format
    tft.setTextSize(4);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    
    // Calculate width for centering (approximately 30 pixels per character at size 4)
    int timeWidth = strlen(timeStr) * 30;
    tft.drawString(timeStr, (screenWidth - timeWidth) / 2, 8);
    
    // Add AM/PM and timezone
    char ampmStr[10];
    strftime(ampmStr, sizeof(ampmStr), "%p", &timeinfo);
    tft.setTextSize(2);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(ampmStr, (screenWidth - timeWidth) / 2 + timeWidth + 5, 15);
    
    // Add EST/EDT indicator
    tft.setTextSize(1);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    String tzStr = timeinfo.tm_isdst ? "EDT" : "EST";
    tft.drawString(tzStr, screenWidth - 30, 10);
    
    // Draw date
    char dateStr[20];
    strftime(dateStr, sizeof(dateStr), "%B %d, %Y", &timeinfo);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(dateStr, (screenWidth - strlen(dateStr) * 6) / 2, 45);
  }
  
  // Draw location and weather on same line
  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(locationName, 10, 65);
  
  // Draw temperature on right side
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  String tempStr = String(temperature, 0) + "F";
  tft.drawString(tempStr, screenWidth - 60, 60);
  
  // Draw weather info
  drawWeatherIcon(10, 85, weatherIcon);
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(weatherDescription, 60, 90);
  
  String humStr = "Humidity: " + String(humidity) + "%";
  tft.drawString(humStr, 60, 105);
  
  // Draw separator line
  tft.drawLine(10, 125, screenWidth - 10, 125, TFT_DARKGREY);
  
  // Draw closest flight info
  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("Nearest Flight:", 10, 135);
  
  if (flightCount > 0) {
    drawFlightInfo(10, 150, nearbyFlights[0]);
  } else {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("Touch screen to check for flights", 10, 160);
  }
  
  // Draw hint at bottom
  tft.fillRect(0, screenHeight - 25, screenWidth, 25, TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("Touch to check flights", (screenWidth - 130) / 2, screenHeight - 20);
  
  // Draw IP address in corner
  String ipStr = WiFi.localIP().toString();
  tft.drawString(ipStr, 5, screenHeight - 10);
}

void drawFlightInfo(int x, int y, FlightInfo& flight) {
  tft.setTextSize(1);
  
  // Callsign and airline
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  String flightStr = flight.callsign;
  if (flight.flightNumber.length() > 0) {
    flightStr = flight.airline + " " + flight.flightNumber;
  } else if (flight.airline.length() > 0) {
    flightStr += " (" + flight.airline + ")";
  }
  tft.drawString(flightStr, x, y);
  
  // Route with airport codes first, then cities in parentheses
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  String routeStr = "";
  if (flight.origin.length() > 0) {
    // Format: KBOS (Boston)
    routeStr = flight.origin;
    if (flight.originCity.length() > 0) {
      // Extract just the city name if it contains comma (e.g., "Boston, MA" -> "Boston")
      String cityName = flight.originCity;
      int commaPos = cityName.indexOf(',');
      if (commaPos > 0) {
        cityName = cityName.substring(0, commaPos);
      }
      routeStr += " (" + cityName + ")";
    }
    
    if (flight.destination.length() > 0) {
      routeStr += " -> " + flight.destination;
      if (flight.destinationCity.length() > 0) {
        // Extract just the city name
        String destCity = flight.destinationCity;
        int commaPos = destCity.indexOf(',');
        if (commaPos > 0) {
          destCity = destCity.substring(0, commaPos);
        }
        routeStr += " (" + destCity + ")";
      }
    }
  } else if (flight.destination.length() > 0) {
    routeStr = "To: " + flight.destination;
    if (flight.destinationCity.length() > 0) {
      // Extract just the city name
      String destCity = flight.destinationCity;
      int commaPos = destCity.indexOf(',');
      if (commaPos > 0) {
        destCity = destCity.substring(0, commaPos);
      }
      routeStr += " (" + destCity + ")";
    }
  }
  
  // Truncate if too long for screen (320 pixels wide, ~6 pixels per char)
  if (routeStr.length() > 50) {
    // Show abbreviated format: KBOS -> KLAX
    routeStr = flight.origin + " -> " + flight.destination;
  }
  
  if (routeStr.length() > 0) {
    tft.drawString(routeStr, x, y + 15);
  }
  
  // Aircraft type, altitude and distance
  String infoStr = "";
  if (flight.aircraft.length() > 0) {
    // Shorten aircraft type if needed (e.g., "Boeing 737-800" -> "B738")
    String aircraft = flight.aircraft;
    if (aircraft.length() > 8) {
      // Try to extract just the model number
      if (aircraft.startsWith("Boeing 737")) aircraft = "B737";
      else if (aircraft.startsWith("Boeing 747")) aircraft = "B747";
      else if (aircraft.startsWith("Boeing 757")) aircraft = "B757";
      else if (aircraft.startsWith("Boeing 767")) aircraft = "B767";
      else if (aircraft.startsWith("Boeing 777")) aircraft = "B777";
      else if (aircraft.startsWith("Boeing 787")) aircraft = "B787";
      else if (aircraft.startsWith("Airbus A3")) aircraft = aircraft.substring(7, 11);
      else if (aircraft.startsWith("Airbus")) aircraft = aircraft.substring(7);
      else aircraft = aircraft.substring(0, 8);
    }
    infoStr = aircraft + " | ";
  }
  infoStr += String((int)flight.altitude) + "ft | ";
  infoStr += String(flight.distance, 1) + "km";
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(infoStr, x, y + 30);
  
  // Speed
  String speedStr = "Speed: " + String((int)flight.speed) + " km/h";
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString(speedStr, x, y + 45);
}

void drawWeatherIcon(int x, int y, String icon) {
  // Clear the icon area first
  tft.fillRect(x, y, 45, 35, TFT_BLACK);
  
  if (icon == "01d") {
    // Clear day - draw sun
    tft.fillCircle(x + 20, y + 17, 10, TFT_YELLOW);
    // Sun rays
    for (int i = 0; i < 8; i++) {
      float angle = i * PI / 4;
      int x1 = x + 20 + cos(angle) * 14;
      int y1 = y + 17 + sin(angle) * 14;
      int x2 = x + 20 + cos(angle) * 18;
      int y2 = y + 17 + sin(angle) * 18;
      tft.drawLine(x1, y1, x2, y2, TFT_YELLOW);
    }
  } else if (icon == "01n") {
    // Clear night - draw moon
    tft.fillCircle(x + 20, y + 17, 10, TFT_WHITE);
    tft.fillCircle(x + 25, y + 17, 10, TFT_BLACK); // Create crescent
  } else if (icon == "02d" || icon == "02n") {
    // Few clouds
    tft.fillCircle(x + 15, y + 12, 6, TFT_YELLOW);
    tft.fillCircle(x + 20, y + 20, 8, TFT_LIGHTGREY);
    tft.fillCircle(x + 28, y + 20, 6, TFT_LIGHTGREY);
    tft.fillRect(x + 20, y + 18, 8, 6, TFT_LIGHTGREY);
  } else if (icon == "03d" || icon == "03n" || icon == "04d" || icon == "04n") {
    // Clouds
    tft.fillCircle(x + 15, y + 17, 8, TFT_LIGHTGREY);
    tft.fillCircle(x + 25, y + 17, 9, TFT_LIGHTGREY);
    tft.fillCircle(x + 32, y + 17, 7, TFT_LIGHTGREY);
    tft.fillRect(x + 15, y + 15, 17, 8, TFT_LIGHTGREY);
  } else if (icon.startsWith("09") || icon.startsWith("10")) {
    // Rain
    tft.fillCircle(x + 20, y + 12, 8, TFT_DARKGREY);
    tft.fillCircle(x + 28, y + 12, 6, TFT_DARKGREY);
    tft.fillRect(x + 20, y + 10, 8, 6, TFT_DARKGREY);
    // Rain drops
    for (int i = 0; i < 4; i++) {
      int rx = x + 12 + i * 6;
      tft.drawLine(rx, y + 20, rx - 2, y + 28, TFT_CYAN);
      tft.drawLine(rx + 1, y + 20, rx - 1, y + 28, TFT_CYAN);
    }
  } else if (icon == "11d" || icon == "11n") {
    // Thunderstorm
    tft.fillCircle(x + 20, y + 10, 8, TFT_DARKGREY);
    tft.fillCircle(x + 28, y + 10, 6, TFT_DARKGREY);
    tft.fillRect(x + 20, y + 8, 8, 6, TFT_DARKGREY);
    // Lightning bolt
    tft.drawLine(x + 22, y + 18, x + 18, y + 24, TFT_YELLOW);
    tft.drawLine(x + 18, y + 24, x + 22, y + 24, TFT_YELLOW);
    tft.drawLine(x + 22, y + 24, x + 18, y + 30, TFT_YELLOW);
    tft.drawLine(x + 23, y + 18, x + 19, y + 24, TFT_YELLOW);
    tft.drawLine(x + 19, y + 24, x + 23, y + 24, TFT_YELLOW);
    tft.drawLine(x + 23, y + 24, x + 19, y + 30, TFT_YELLOW);
  } else if (icon == "13d" || icon == "13n") {
    // Snow
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("*", x + 10, y + 10);
    tft.drawString("*", x + 25, y + 5);
    tft.drawString("*", x + 15, y + 20);
  } else if (icon == "50d" || icon == "50n") {
    // Mist/Fog
    for (int i = 0; i < 3; i++) {
      int yy = y + 8 + i * 8;
      tft.drawLine(x + 10, yy, x + 35, yy, TFT_LIGHTGREY);
      tft.drawLine(x + 8, yy + 2, x + 37, yy + 2, TFT_LIGHTGREY);
    }
  } else {
    // Unknown - draw question mark
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.drawString("?", x + 18, y + 10);
  }
}

void updateTime() {
  // Time is updated automatically by NTP
  // This function can be used for additional time-related tasks
}

void updateWeather() {
  if (weatherAPIKey.length() == 0) return;
  
  // Turn on red LED to indicate API call
  digitalWrite(RGB_LED_R, LOW);  // Active LOW on Type-C variant
  
  HTTPClient http;
  String url = "http://api.openweathermap.org/data/2.5/weather?";
  url += "lat=" + String(latitude);
  url += "&lon=" + String(longitude);
  url += "&appid=" + weatherAPIKey;
  url += "&units=imperial";
  
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    
    temperature = doc["main"]["temp"];
    humidity = doc["main"]["humidity"];
    weatherDescription = doc["weather"][0]["description"].as<String>();
    weatherIcon = doc["weather"][0]["icon"].as<String>();
    
    Serial.println("Weather updated successfully");
  } else {
    Serial.println("Weather update failed: " + String(httpCode));
  }
  
  http.end();
  
  // Blink red LED off after API call
  delay(100);
  digitalWrite(RGB_LED_R, HIGH);  // Active LOW on Type-C variant
}

void updateFlights() {
  if (flightAwareAPIKey.length() == 0) {
    Serial.println("No FlightAware API key configured");
    // Fall back to OpenSky
    updateFlightsOpenSky();
    return;
  }
  
  Serial.println("Fetching flight data from FlightAware...");
  
  // Turn on red LED to indicate API call
  digitalWrite(RGB_LED_R, LOW);  // Active LOW on Type-C variant
  
  HTTPClient http;
  
  // FlightAware AeroAPI v4 endpoint for nearby flights
  // Search within approximately 50km radius
  String url = "https://aeroapi.flightaware.com/aeroapi/flights/search";
  url += "?query=-latlong \"" + String(latitude - 0.45) + " " + String(longitude - 0.45);
  url += " " + String(latitude + 0.45) + " " + String(longitude + 0.45) + "\"";
  url += "&max_pages=1";
  
  http.begin(url);
  http.addHeader("x-apikey", flightAwareAPIKey);
  
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String payload = http.getString();
    parseFlightAwareData(payload);
    Serial.println("Flight data updated: " + String(flightCount) + " flights found");
  } else {
    Serial.println("FlightAware API failed: " + String(httpCode));
    // Fall back to OpenSky if FlightAware fails
    updateFlightsOpenSky();
  }
  
  http.end();
  
  // Blink red LED off after API call
  delay(100);
  digitalWrite(RGB_LED_R, HIGH);  // Active LOW on Type-C variant
}

void updateFlightsOpenSky() {
  // Fallback to OpenSky Network free API
  Serial.println("Falling back to OpenSky Network...");
  
  // Turn on red LED to indicate API call
  digitalWrite(RGB_LED_R, LOW);  // Active LOW on Type-C variant
  
  HTTPClient http;
  
  // Calculate bounding box (approximately 50km radius)
  float latDelta = 0.45; // ~50km at equator
  float lonDelta = 0.45;
  
  String url = "https://opensky-network.org/api/states/all?";
  url += "lamin=" + String(latitude - latDelta);
  url += "&lomin=" + String(longitude - lonDelta);
  url += "&lamax=" + String(latitude + latDelta);
  url += "&lomax=" + String(longitude + lonDelta);
  
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode == 200) {
    String payload = http.getString();
    parseOpenSkyData(payload);
    Serial.println("OpenSky data updated: " + String(flightCount) + " flights found");
  } else {
    Serial.println("OpenSky update failed: " + String(httpCode));
  }
  
  http.end();
  
  // Blink red LED off after API call
  delay(100);
  digitalWrite(RGB_LED_R, HIGH);  // Active LOW on Type-C variant
}

void parseFlightAwareData(String json) {
  DynamicJsonDocument doc(16384);
  deserializeJson(doc, json);
  
  JsonArray flights = doc["flights"];
  flightCount = 0;
  
  for (JsonObject flight : flights) {
    if (flightCount >= 3) break;
    
    FlightInfo& info = nearbyFlights[flightCount];
    
    // Get flight identification
    info.callsign = flight["ident"].as<String>();
    info.callsign.trim();
    
    // Get airline and flight number
    if (flight.containsKey("operator")) {
      info.airline = flight["operator"].as<String>();
    } else {
      info.airline = getAirlineFromCallsign(info.callsign);
    }
    
    if (flight.containsKey("flight_number")) {
      info.flightNumber = flight["flight_number"].as<String>();
    }
    
    // Get aircraft type
    if (flight.containsKey("aircraft_type")) {
      info.aircraft = flight["aircraft_type"].as<String>();
    }
    
    // Get origin airport
    if (flight.containsKey("origin")) {
      JsonObject origin = flight["origin"];
      info.origin = origin["code"].as<String>();
      info.originCity = origin["city"].as<String>();
    }
    
    // Get destination airport
    if (flight.containsKey("destination")) {
      JsonObject dest = flight["destination"];
      info.destination = dest["code"].as<String>();
      info.destinationCity = dest["city"].as<String>();
    }
    
    // Get last position
    if (flight.containsKey("last_position")) {
      JsonObject pos = flight["last_position"];
      info.latitude = pos["latitude"].as<float>();
      info.longitude = pos["longitude"].as<float>();
      info.altitude = pos["altitude"].as<float>() * 100; // Convert FL to feet
      info.speed = pos["groundspeed"].as<float>() * 1.852; // Convert knots to km/h
      info.heading = pos["heading"].as<float>();
      
      // Calculate distance
      info.distance = calculateDistance(latitude, longitude, info.latitude, info.longitude);
    }
    
    flightCount++;
  }
  
  // Sort by distance
  sortFlightsByDistance();
}

void parseOpenSkyData(String json) {
  DynamicJsonDocument doc(8192);
  deserializeJson(doc, json);
  
  JsonArray states = doc["states"];
  flightCount = 0;
  
  for (JsonArray state : states) {
    if (flightCount >= 3) break;
    
    FlightInfo& flight = nearbyFlights[flightCount];
    
    flight.callsign = state[1].as<String>();
    flight.callsign.trim();
    
    flight.latitude = state[6].as<float>();
    flight.longitude = state[5].as<float>();
    flight.altitude = state[13].as<float>() * 3.28084; // Convert m to ft
    flight.speed = state[9].as<float>() * 3.6; // Convert m/s to km/h
    flight.heading = state[10].as<float>();
    
    // Calculate distance
    flight.distance = calculateDistance(latitude, longitude, flight.latitude, flight.longitude);
    
    // Get airline from callsign
    flight.airline = getAirlineFromCallsign(flight.callsign);
    flight.flightNumber = "";
    flight.aircraft = "";
    
    // OpenSky doesn't provide origin/destination
    flight.origin = "";
    flight.originCity = "";
    flight.destination = "";
    flight.destinationCity = "";
    
    flightCount++;
  }
  
  // Sort by distance
  sortFlightsByDistance();
}


float calculateDistance(float lat1, float lon1, float lat2, float lon2) {
  // Haversine formula
  float R = 6371; // Earth radius in km
  float dLat = (lat2 - lat1) * PI / 180;
  float dLon = (lon2 - lon1) * PI / 180;
  float a = sin(dLat/2) * sin(dLat/2) +
            cos(lat1 * PI / 180) * cos(lat2 * PI / 180) *
            sin(dLon/2) * sin(dLon/2);
  float c = 2 * atan2(sqrt(a), sqrt(1-a));
  return R * c;
}

void sortFlightsByDistance() {
  // Simple bubble sort for 3 elements
  for (int i = 0; i < flightCount - 1; i++) {
    for (int j = 0; j < flightCount - i - 1; j++) {
      if (nearbyFlights[j].distance > nearbyFlights[j + 1].distance) {
        FlightInfo temp = nearbyFlights[j];
        nearbyFlights[j] = nearbyFlights[j + 1];
        nearbyFlights[j + 1] = temp;
      }
    }
  }
}

String getAirlineFromCallsign(String callsign) {
  // Basic airline code mapping
  if (callsign.startsWith("AAL")) return "American";
  if (callsign.startsWith("DAL")) return "Delta";
  if (callsign.startsWith("UAL")) return "United";
  if (callsign.startsWith("SWA")) return "Southwest";
  if (callsign.startsWith("JBU")) return "JetBlue";
  if (callsign.startsWith("ASA")) return "Alaska";
  if (callsign.startsWith("BAW")) return "British Airways";
  if (callsign.startsWith("DLH")) return "Lufthansa";
  if (callsign.startsWith("AFR")) return "Air France";
  if (callsign.startsWith("KLM")) return "KLM";
  return "";
}

void handleTouch() {
#if USE_TFT_ESPI_TOUCH == 1
  // Using TFT_eSPI touch
  uint16_t x = 0, y = 0;
  bool touched = tft.getTouch(&x, &y);
  
  if (touched && !isAPMode) {
    // Any touch triggers flight update
    updateFlights();
    
    // Visual feedback
    tft.fillCircle(x, y, 5, TFT_WHITE);
    delay(100);
    drawMainScreen(); // Redraw to show updated flight
    
    delay(200); // Debounce
  }
#else
  // Using XPT2046 library
  if (touch.touched()) {
    TS_Point p = touch.getPoint();
    
    // Check for valid touch
    if (p.x > 100 && p.x < 4000 && p.y > 100 && p.y < 4000 && p.z > 200) {
      // Map touch coordinates using calibration values
      int x = map(p.y, 305, 3481, 0, tft.width());
      int y = map(p.x, 270, 3578, tft.height(), 0);
      
      // Constrain to screen bounds
      x = constrain(x, 0, tft.width() - 1);
      y = constrain(y, 0, tft.height() - 1);
      
      if (!isAPMode) {
        // Any touch triggers flight update
        updateFlights();
        
        // Visual feedback
        tft.fillCircle(x, y, 5, TFT_WHITE);
        delay(100);
        drawMainScreen(); // Redraw to show updated flight
        
        delay(200); // Debounce
      }
    }
  }
#endif
}

// ============================================
// Part 3: Web Server and Configuration Functions
// ============================================

void startWebServer() {
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/reset", HTTP_POST, handleReset);
  server.begin();
  
  Serial.println("Web server started");
}

void handleRoot() {
  server.send(200, "text/html", getConfigHTML());
}

void handleSave() {
  // Get form data - server.arg() automatically handles URL decoding
  userName = server.arg("userName");
  wifiSSID = server.arg("wifiSSID");
  wifiPassword = server.arg("wifiPassword");
  weatherAPIKey = server.arg("weatherKey");
  flightAwareAPIKey = server.arg("flightKey");
  latitude = server.arg("latitude").toFloat();
  longitude = server.arg("longitude").toFloat();
  locationName = server.arg("location");
  
  // Debug output
  Serial.println("Saving configuration:");
  Serial.println("SSID: " + wifiSSID);
  Serial.println("Password length: " + String(wifiPassword.length()));
  Serial.println("Location: " + locationName);
  
  // Save configuration
  saveConfiguration();
  
  // Send response
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:Arial;text-align:center;padding:20px;}";
  html += ".success{color:green;font-size:20px;margin:20px;}";
  html += ".btn{background:#4CAF50;color:white;padding:10px 20px;border:none;";
  html += "border-radius:5px;font-size:16px;cursor:pointer;margin:10px;}";
  html += "</style></head><body>";
  html += "<h1>Configuration Saved!</h1>";
  html += "<p class='success'>Settings saved successfully</p>";
  html += "<p>The device will restart and connect to your WiFi network.</p>";
  html += "<button class='btn' onclick='window.location=\"/\"'>Back to Settings</button>";
  html += "<script>setTimeout(function(){location.href='/';},5000);</script>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
  
  // Restart after a delay
  delay(2000);
  ESP.restart();
}

void handleReset() {
  // Clear all preferences
  preferences.begin("flighttrack", false);
  preferences.clear();
  preferences.end();
  
  // Send response
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:Arial;text-align:center;padding:20px;}";
  html += ".warning{color:red;font-size:20px;margin:20px;}";
  html += "</style></head><body>";
  html += "<h1>Factory Reset Complete</h1>";
  html += "<p class='warning'>All settings have been cleared</p>";
  html += "<p>The device will restart in setup mode.</p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
  
  // Restart after a delay
  delay(2000);
  ESP.restart();
}

String getConfigHTML() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Flight Tracker Configuration</title>";
  html += "<style>";
  html += "body{font-family:'Segoe UI',Arial,sans-serif;margin:0;padding:20px;";
  html += "background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:#333;}";
  html += ".container{max-width:500px;margin:0 auto;background:white;";
  html += "border-radius:10px;padding:30px;box-shadow:0 10px 30px rgba(0,0,0,0.2);}";
  html += "h1{color:#667eea;text-align:center;margin-bottom:30px;}";
  html += ".form-group{margin-bottom:20px;}";
  html += "label{display:block;margin-bottom:5px;font-weight:bold;color:#555;}";
  html += "input[type='text'],input[type='password'],input[type='number']{";
  html += "width:100%;padding:10px;border:2px solid #ddd;border-radius:5px;";
  html += "box-sizing:border-box;font-size:16px;}";
  html += "input:focus{outline:none;border-color:#667eea;}";
  html += ".btn{background:#667eea;color:white;padding:12px 30px;border:none;";
  html += "border-radius:5px;font-size:16px;cursor:pointer;width:100%;margin-top:10px;}";
  html += ".btn:hover{background:#5a67d8;}";
  html += ".btn-danger{background:#e53e3e;}";
  html += ".btn-danger:hover{background:#c53030;}";
  html += ".info{background:#f7fafc;border-left:4px solid #667eea;padding:10px;";
  html += "margin:20px 0;border-radius:5px;}";
  html += ".section{border-top:2px solid #e2e8f0;margin-top:30px;padding-top:20px;}";
  html += ".help-text{font-size:12px;color:#718096;margin-top:5px;}";
  html += "</style></head><body>";
  
  html += "<div class='container'>";
  html += "<h1>✈️ Flight Tracker Configuration</h1>";
  
  html += "<form action='/save' method='POST' accept-charset='UTF-8'>";
  
  // Personal Settings
  html += "<div class='form-group'>";
  html += "<label for='userName'>Your Name</label>";
  html += "<input type='text' id='userName' name='userName' value='" + htmlEncode(userName) + "' required>";
  html += "<div class='help-text'>Used for personalized greeting</div>";
  html += "</div>";
  
  // WiFi Settings
  html += "<div class='section'>";
  html += "<h2>WiFi Settings</h2>";
  html += "<div class='form-group'>";
  html += "<label for='wifiSSID'>WiFi Network Name (SSID)</label>";
  html += "<input type='text' id='wifiSSID' name='wifiSSID' value='" + htmlEncode(wifiSSID) + "' required>";
  html += "<div class='help-text'>Supports spaces and special characters</div>";
  html += "</div>";
  
  html += "<div class='form-group'>";
  html += "<label for='wifiPassword'>WiFi Password</label>";
  html += "<input type='text' id='wifiPassword' name='wifiPassword' value='" + htmlEncode(wifiPassword) + "' required>";
  html += "<div class='help-text'>Supports WPA2/WPA3 networks</div>";
  html += "</div>";
  html += "</div>";
  
  // Location Settings
  html += "<div class='section'>";
  html += "<h2>Location Settings</h2>";
  html += "<div class='form-group'>";
  html += "<label for='location'>Location Name</label>";
  html += "<input type='text' id='location' name='location' value='" + htmlEncode(locationName) + "' placeholder='e.g., Boston, MA' required>";
  html += "</div>";
  
  html += "<div class='form-group'>";
  html += "<label for='latitude'>Latitude</label>";
  html += "<input type='number' id='latitude' name='latitude' value='" + String(latitude, 6) + "' step='0.000001' required>";
  html += "<div class='help-text'>Use Google Maps or latlong.net to find coordinates</div>";
  html += "</div>";
  
  html += "<div class='form-group'>";
  html += "<label for='longitude'>Longitude</label>";
  html += "<input type='number' id='longitude' name='longitude' value='" + String(longitude, 6) + "' step='0.000001' required>";
  html += "</div>";
  html += "</div>";
  
  // API Settings
  html += "<div class='section'>";
  html += "<h2>API Settings</h2>";
  html += "<div class='form-group'>";
  html += "<label for='weatherKey'>OpenWeatherMap API Key</label>";
  html += "<input type='text' id='weatherKey' name='weatherKey' value='" + htmlEncode(weatherAPIKey) + "'>";
  html += "<div class='help-text'>Get from openweathermap.org (free tier available)</div>";
  html += "</div>";
  
  html += "<div class='form-group'>";
  html += "<label for='flightKey'>FlightAware API Key</label>";
  html += "<input type='text' id='flightKey' name='flightKey' value='" + htmlEncode(flightAwareAPIKey) + "'>";
  html += "<div class='help-text'>Get from flightaware.com/aeroapi (provides flight origin/destination)</div>";
  html += "</div>";
  html += "</div>";
  
  // Info box
  html += "<div class='info'>";
  html += "<strong>ℹ️ Tips:</strong><br>";
  html += "• FlightAware API provides detailed flight info including origin/destination<br>";
  html += "• Falls back to OpenSky Network if FlightAware is unavailable<br>";
  html += "• Touch anywhere on screen to refresh flight data<br>";
  html += "• Weather requires OpenWeatherMap API key (free tier: 1000 calls/day)<br>";
  html += "• Device IP: " + WiFi.localIP().toString();
  html += "</div>";
  
  // Buttons
  html += "<button type='submit' class='btn'>Save Configuration</button>";
  html += "</form>";
  
  html += "<form action='/reset' method='POST' onsubmit='return confirm(\"Are you sure you want to factory reset?\");'>";
  html += "<button type='submit' class='btn btn-danger'>Factory Reset</button>";
  html += "</form>";
  
  html += "</div>";
  
  // Add JavaScript for coordinate lookup
  html += "<script>";
  html += "function getLocation() {";
  html += "  if (navigator.geolocation) {";
  html += "    navigator.geolocation.getCurrentPosition(function(position) {";
  html += "      document.getElementById('latitude').value = position.coords.latitude.toFixed(6);";
  html += "      document.getElementById('longitude').value = position.coords.longitude.toFixed(6);";
  html += "      alert('Location detected! Please enter a location name.');";
  html += "    });";
  html += "  }";
  html += "}";
  html += "</script>";
  
  // Add button for geolocation
  html += "<script>";
  html += "document.addEventListener('DOMContentLoaded', function() {";
  html += "  var latInput = document.getElementById('latitude');";
  html += "  if (navigator.geolocation && latInput) {";
  html += "    var btn = document.createElement('button');";
  html += "    btn.type = 'button';";
  html += "    btn.className = 'btn';";
  html += "    btn.style.marginTop = '10px';";
  html += "    btn.textContent = '📍 Use Current Location';";
  html += "    btn.onclick = getLocation;";
  html += "    latInput.parentNode.appendChild(btn);";
  html += "  }";
  html += "});";
  html += "</script>";
  
  html += "</body></html>";
  
  return html;
}

// HTML encode function to handle special characters in form inputs
String htmlEncode(String str) {
  String encoded = str;
  encoded.replace("&", "&amp;");
  encoded.replace("\"", "&quot;");
  encoded.replace("'", "&#39;");
  encoded.replace("<", "&lt;");
  encoded.replace(">", "&gt;");
  return encoded;
}