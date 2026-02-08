# ESP Flight Tracker Clock

A sophisticated ESP32-based flight tracker and weather clock with a 2.4" TFT touch screen display. This project combines real-time flight tracking, weather information, and time display in an elegant interface.

## 🎯 Designed for ESP32 Cheap Yellow Display (CYD)

This project is specifically configured for CYD boards with ILI9341 display driver:
- 2.4" TFT Display (240x320, ILI9341)
- Resistive Touch Screen (XPT2046)
- SD Card Slot
- Speaker Connection

## ⚠️ IMPORTANT: Display Configuration Required

**The TFT_eSPI library requires configuration in its User_Setup.h file.**  
Defining configuration in the sketch does NOT work reliably.

### Configure TFT_eSPI Library

1. **Find the TFT_eSPI library folder:**
   - Arduino IDE: `Arduino/libraries/TFT_eSPI/`
   - PlatformIO: `.pio/libdeps/esp32dev/TFT_eSPI/`

2. **Edit `User_Setup.h`** and replace/add this configuration:

```cpp
// CYD (Cheap Yellow Display) Configuration
#define ILI9341_2_DRIVER  // Use alternative driver (required for some CYD variants)

// Display pins
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1

// Backlight pin
#define TFT_BL   27
#define TFT_BACKLIGHT_ON HIGH

// Touch screen chip select
#define TOUCH_CS 33

// Color settings
#define TFT_RGB_ORDER TFT_RGB
#define TFT_INVERSION_ON

// Fonts to load
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

// SPI Frequencies
#define SPI_FREQUENCY  40000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000
```

3. **Save the file** before compiling

## Features

- ✈️ **Touch-Activated Flight Tracking**: Touch to refresh nearby flights using free OpenSky Network API
- 🌤️ **Weather Display**: Current temperature, conditions, humidity with animated icons
- 🕐 **NTP-Synchronized Clock**: Accurate time display with automatic synchronization
- 📱 **Touch Interface**: Interactive touch screen for navigating between screens
- 🔧 **Web Configuration**: Easy setup through web interface
- 💾 **Persistent Storage**: Saves all settings to flash memory
- 📡 **Captive Portal**: Automatic setup mode with WiFi access point
- 🗂️ **SD Card Support**: Optional logging and asset storage

## Hardware Requirements

- **ESP32 Development Board** (ESP32-WROOM-32)
- **2.4" TFT Touch Display** (ILI9341 driver, 240x320 pixels)
- **XPT2046 Touch Controller**
- **SD Card Module** (optional)
- **Power Supply** (5V USB or battery)

### Pin Connections (CYD with ILI9341)

| Component | ESP32 Pin | Description |
|-----------|-----------|-------------|
| TFT_MISO | GPIO 12 | SPI MISO |
| TFT_MOSI | GPIO 13 | SPI MOSI |
| TFT_SCLK | GPIO 14 | SPI Clock |
| TFT_CS | GPIO 15 | TFT Chip Select |
| TFT_DC | GPIO 2 | TFT Data/Command |
| TFT_RST | -1 | TFT Reset (not connected) |
| TFT_BL | GPIO 21 | TFT Backlight |
| TOUCH_CS | GPIO 33 | Touch Chip Select |
| TOUCH_IRQ | GPIO 36 | Touch Interrupt |
| SD_CS | GPIO 5 | SD Card Chip Select |

## Software Dependencies

### Arduino IDE Setup

1. Install Arduino IDE (1.8.x or 2.x)
2. Add ESP32 board support:
   - Go to File → Preferences
   - Add to Additional Board Manager URLs:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to Tools → Board → Board Manager
   - Search for "esp32" and install

### Required Libraries

Install via Arduino Library Manager:

- **TFT_eSPI** by Bodmer - Display driver (MUST configure User_Setup.h!)
- **XPT2046_Touchscreen** by Paul Stoffregen - Touch controller
- **ArduinoJson** by Benoit Blanchon - JSON parsing

**⚠️ Remember:** You MUST edit the TFT_eSPI library's User_Setup.h file as described above!

## Installation

1. **Clone or download this repository**
2. **Open `espflighttracker.ino` in Arduino IDE**
3. **Combine all parts**: Copy contents from `espflighttracker_part2.ino` and `espflighttracker_part3.ino` into the main file
4. **Select board**: Tools → Board → ESP32 Dev Module
5. **Configure board settings**:
   - Flash Size: 4MB
   - Partition Scheme: Default 4MB with spiffs
   - Upload Speed: 921600
6. **Select port**: Tools → Port → (Your ESP32 port)
7. **Upload**: Click Upload button

## Initial Setup

### First Time Configuration

1. **Power on the device** - You'll see "Welcome to Flight Tracker Clock"
2. **Connect to WiFi Access Point**:
   - Network: `FlightTracker`
   - Password: `12345678`
3. **Open configuration page**:
   - Navigate to `http://192.168.4.1`
   - Or any URL (captive portal redirects)
4. **Enter your settings**:
   - Your name (for greeting)
   - WiFi credentials
   - Location coordinates
   - API keys (optional)
5. **Save configuration** - Device restarts and connects

### Getting API Keys

#### OpenWeatherMap (Required for weather)
1. Sign up at [openweathermap.org](https://openweathermap.org)
2. Go to API Keys in your account
3. Generate new API key
4. Free tier: 1000 calls/day

#### Flight Tracking
- **No API key needed!** Uses free OpenSky Network API
- 100 requests/day limit (perfect for touch-activated updates)
- Real-time ADS-B data with 10-15 second delay

### Finding Your Coordinates

**Option 1: Google Maps**
1. Right-click your location
2. Click coordinates to copy

**Option 2: Browser Geolocation**
1. Click "Use Current Location" button in config page
2. Allow browser location access

**Option 3: Online Tools**
- Visit [latlong.net](https://www.latlong.net)

## Usage

### Main Screen
- Displays current time, date, and weather
- Shows location and weather conditions
- Touch "Show Nearby Flights" button for flight info

### Flight Screen
- Lists up to 3 nearest flights within ~50km radius
- Shows callsign, airline, altitude, distance, and speed
- **Touch anywhere on screen to refresh flight data**
- Touch "Back" button to return to main screen

### Web Configuration
- Access anytime at device IP (shown on screen)
- Update settings without factory reset
- Pre-fills current configuration

## Troubleshooting

### Display Issues
- Check all wire connections
- Verify TFT_eSPI configuration
- Ensure proper power supply (5V)

### WiFi Connection Failed
- Verify credentials are correct
- Ensure 2.4GHz network (not 5GHz)
- Device auto-enters setup mode after 3 failures

### No Weather Data
- Check OpenWeatherMap API key
- Verify internet connection
- Check API quota hasn't exceeded

### No Flight Data
- Normal - depends on air traffic in area
- Increase search radius in code if needed
- Check internet connectivity

### Touch Not Working
- Verify touch controller connections
- Check TOUCH_CS and TOUCH_IRQ pins
- Calibrate touch if needed

## Customization

### Change AP Credentials
Edit in code:
```cpp
const char* AP_SSID = "FlightTracker";
const char* AP_PASSWORD = "12345678";
```

### Adjust Update Intervals
```cpp
const int WEATHER_UPDATE_INTERVAL = 3600; // seconds
const int FLIGHT_UPDATE_INTERVAL = 60; // seconds
```

### Modify Search Radius
In `updateFlightsOpenSky()`:
```cpp
float latDelta = 0.45; // Increase for larger radius
float lonDelta = 0.45;
```

## PlatformIO Configuration

If using PlatformIO instead of Arduino IDE, create `platformio.ini`:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps = 
    bodmer/TFT_eSPI
    paulstoffregen/XPT2046_Touchscreen
    bblanchon/ArduinoJson
```

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues.

## License

This project is open source and available under the MIT License.

## Acknowledgments

- OpenSky Network for free flight data API
- OpenWeatherMap for weather API
- Bodmer for excellent TFT_eSPI library
- ESP32 community for examples and support

## Future Enhancements

- [ ] Add flight history logging to SD card
- [ ] Implement airline logos display
- [ ] Add more weather animations
- [ ] Support for multiple locations
- [ ] Flight alerts for specific airlines/routes
- [ ] Integration with FlightRadar24 API
- [ ] Battery operation support
- [ ] Sleep mode for power saving
- [ ] OTA (Over-The-Air) updates

## Support

For issues, questions, or suggestions, please open an issue on GitHub.