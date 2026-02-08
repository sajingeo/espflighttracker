# Design

This is ESP32 powered clock with a flight tracker using an ESP32 connected to 2.4 inches (240x320 px) TFT touch screen.

The pin out for this CYD can be found here
https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/PINS.md

More code exmaples here
https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/tree/main/Examples/Basics


## Features Implemented

### Start up
When it starts up without WiFi credentials for the first time, it shows "Welcome to flight tracker clock by sajin" with a decorative animation.

### Onboarding
The ESP32 advertises an AP (Access Point) that users can connect to:
- **SSID**: WeatherPuck
- **Password**: 12345678
- **Configuration URL**: http://192.168.4.1

The configuration webpage allows users to enter:
- User's name (for personalized greeting)
- WiFi network credentials (SSID and password)
- Latitude and longitude coordinates
- Maybe an API key to track the flights and weather

The screen displays the AP SSID and password during setup mode. Once configured, the device saves all settings to flash memory or the SD card and restarts.

### Regular Startup
On power on, the device:
1. Shows "Hello [USER NAME]!" with a animation for 3 seconds
2. Attempts to connect to WiFi (up to 3 attempts)
3. If connection fails after 3 attempts, automatically falls back to AP mode
4. On successful connection, displays:
   - Current time (updates every second)
   - Date
   - Location name
   - Weather icon (animated based on conditions)
   - Temperature in Fahrenheit
   - Weather description
   - Humidity percentage
   - Device IP address (small text at bottom)
   - text saying press button to get the flight details of the flight right above you.
    - when the user presses the button on the touch screen, it should use an API to get the near by flight information and display the flight closest to the lat / long that was configured.

### Web Configuration Portal
The device runs a web server continuously when connected to WiFi:
- Accessible at the device's IP address
- Pre-fills form with current configuration values
- Allows updating any settings without factory reset
- Includes factory reset button to wipe all settings

### Weather Display
- Updates weather data every hour
- Updates time display every second
- Shows various weather icons:
  - Sun (clear weather)
  - Clouds (cloudy conditions)
  - Rain drops (rain/drizzle)
  - Snowflakes (snow)
  - Lightning bolt (thunderstorm)
  - Partly cloudy (sun with clouds)
  - Mist/fog lines

### Flight Display
- The flight tail numbers based, the logo of the flight, and From and To destinations
 - If more than one flight is found, show the top 3. Show the one closeses to the configured lat / long

### Data Persistence
All configuration is stored in ESP32's flash memory using the Preferences library:
- WiFi credentials
- API key
- Location information
- User name
- the CYD does have an SD card to store logs of flights and other assets