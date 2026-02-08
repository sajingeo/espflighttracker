# CYD (Cheap Yellow Display) Setup Guide

## Important: TFT_eSPI Library Configuration

The TFT_eSPI library **requires** configuration in its `User_Setup.h` file. Defining configuration in the sketch does NOT work reliably.

### Step 1: Locate the TFT_eSPI Library

Find the TFT_eSPI library folder:
- **Arduino IDE**: `Arduino/libraries/TFT_eSPI/`
- **PlatformIO**: `.pio/libdeps/esp32dev/TFT_eSPI/`

### Step 2: Configure User_Setup.h

Edit `User_Setup.h` in the TFT_eSPI folder and add this configuration:

```cpp
// ==========================================
// CYD (Cheap Yellow Display) Configuration
// ==========================================
#define ILI9341_2_DRIVER  // Use alternative ILI9341 driver (IMPORTANT!)

// Display pins
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1  // Not connected

// Backlight control
#define TFT_BL   27  // GPIO27 for backlight
#define TFT_BACKLIGHT_ON HIGH

// Touch screen chip select
#define TOUCH_CS 33

// Color order
#define TFT_RGB_ORDER TFT_RGB  // or TFT_BGR if colors are swapped

// Inversion
#define TFT_INVERSION_ON  // Many CYD boards need this

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
#define SPI_FREQUENCY  40000000      // 40MHz for display
#define SPI_READ_FREQUENCY  20000000 // 20MHz for reading
#define SPI_TOUCH_FREQUENCY  2500000 // 2.5MHz for touch
```

### Step 3: Save and Compile

Save the `User_Setup.h` file and compile your sketch.

## Hardware Specifications

### ESP32 Cheap Yellow Display (CYD)

The CYD comes in several variants. The most common are:

1. **ESP32-2432S028R** - Standard CYD with ILI9341 display
2. **CYD32S024C** - Type-C variant with ILI9341 display (requires ILI9341_2_DRIVER)

### Key Components

- **Microcontroller**: ESP32-WROOM-32
- **Display**: 2.4" TFT LCD (320x240 pixels)
- **Display Driver**: ILI9341 (use ILI9341_2_DRIVER for some variants)
- **Touch Controller**: XPT2046 (Resistive touch)
- **Flash**: 4MB
- **PSRAM**: None (standard version)
- **SD Card**: Micro SD card slot
- **USB**: Micro-USB or USB-C (depending on variant)
- **Audio**: Speaker connection available
- **Light Sensor**: LDR (Light Dependent Resistor)
- **RGB LED**: GPIO 4 (on Type-C variants)
- **Backlight**: GPIO 27

### Pin Mappings

| Function | GPIO Pin | Notes |
|----------|----------|-------|
| TFT_MISO | 12 | SPI MISO |
| TFT_MOSI | 13 | SPI MOSI |
| TFT_SCLK | 14 | SPI Clock |
| TFT_CS | 15 | TFT Chip Select |
| TFT_DC | 2 | Data/Command |
| TFT_RST | -1 | Not connected |
| TFT_BL | 27 | Backlight control |
| TOUCH_CS | 33 | Touch chip select |
| TOUCH_IRQ | 36 | Touch interrupt |
| SD_CS | 5 | SD card chip select |
| SD_MOSI | 23 | SD card MOSI |
| SD_MISO | 19 | SD card MISO |
| SD_CLK | 18 | SD card clock |
| LDR | 34 | Light sensor |
| SPEAKER | 26 | Speaker/Buzzer |
| RGB_LED | 4 | RGB LED (Type-C variant) |

## Troubleshooting

### Display Not Working

1. **Check User_Setup.h**: Ensure the TFT_eSPI library's User_Setup.h is properly configured
2. **Driver Selection**: Use `ILI9341_2_DRIVER` if standard `ILI9341_DRIVER` doesn't work
3. **Backlight Pin**: GPIO 27 for most CYD boards
4. **Wiring**: Verify all connections if using external display

### Display Shows 240x240 Instead of 320x240

1. **Wrong Driver**: Use `ILI9341_2_DRIVER` instead of `ILI9341_DRIVER`
2. **Width/Height Defines**: Comment out or adjust `TFT_WIDTH` and `TFT_HEIGHT` in User_Setup.h
3. **Rotation**: Use `tft.setRotation(1)` for landscape mode

### Touch Not Working

1. **Calibration**: Touch screen may need calibration
2. **SPI Conflict**: Ensure TOUCH_CS is on GPIO 33
3. **Pressure**: Resistive touch requires firm pressure

### Common Issues

- **Blank Screen**: Usually incorrect backlight pin or missing User_Setup.h configuration
- **Inverted Colors**: Add `#define TFT_INVERSION_ON` to User_Setup.h
- **Mirror Image**: Add `#define TFT_ROTATION 1` (or try 0, 2, 3)
- **Touch Offset**: Touch calibration needed

## Testing

Use the included `hardware_test.ino` sketch to verify:
1. Display functionality
2. Touch response
3. SD card (if present)
4. WiFi connectivity

## References

- [ESP32 Cheap Yellow Display Repository](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)
- [TFT_eSPI Library](https://github.com/Bodmer/TFT_eSPI)
- [XPT2046 Touch Library](https://github.com/PaulStoffregen/XPT2046_Touchscreen)