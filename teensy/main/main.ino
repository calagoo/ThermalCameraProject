#define TFT_MISO  12
#define TFT_MOSI  11  //a12
#define TFT_SCK   13  //a13
#define TFT_DC   6 
#define TFT_CS   5  
#define TFT_RST  9
#include <Adafruit_GFX.h>    // Core graphics library
#include <ST7735_t3.h> // Hardware-specific library
#include <ST7789_t3.h> // Hardware-specific library
#include <SPI.h>
#include <Wire.h>
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"
#include <vector>

ST7735_t3 tft = ST7735_t3(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST);
float p = 3.1415926;
const byte MLX90640_address = 0x33; //Default 7-bit unshifted address of the MLX90640

#define TA_SHIFT 8 //Default shift for MLX90640 in open air

float mlx90640To[768];
paramsMLX90640 mlx90640;

const byte calcStart = 33; //Pin that goes high/low when calculations are complete
//This makes the timing visible on the logic analyzer

uint16_t interpolateColor(float minVal, float maxVal, float val, uint16_t minColor, uint16_t maxColor) {
    // Extract the individual color components from the 16-bit color values
    uint8_t minR = (minColor >> 11) * 8;
    uint8_t minG = ((minColor >> 5) & 0x3F) * 4;
    uint8_t minB = (minColor & 0x1F) * 8;

    uint8_t maxR = (maxColor >> 11) * 8;
    uint8_t maxG = ((maxColor >> 5) & 0x3F) * 4;
    uint8_t maxB = (maxColor & 0x1F) * 8;

    // Scale the value to be between 0 and 1
    float scale = (val - minVal) / (maxVal - minVal);

    // Calculate the interpolated color components
    uint8_t r = (uint8_t)((maxR - minR) * scale + minR);
    uint8_t g = (uint8_t)((maxG - minG) * scale + minG);
    uint8_t b = (uint8_t)((maxB - minB) * scale + minB);

    // Combine the components back into a 16-bit color
    return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);
}

void interpolate(float* original, float* result, int origCols, int origRows, int newCols, int newRows) {
    for (int y = 0; y < newRows; y++) {
        for (int x = 0; x < newCols; x++) {
            float gx = ((float)x / (newCols - 1)) * (origCols - 1);
            float gy = ((float)y / (newRows - 1)) * (origRows - 1);
            int gxi = (int)gx;
            int gyi = (int)gy;

            float c00 = original[gyi * origCols + gxi];
            float c10 = original[gyi * origCols + gxi + 1];
            float c01 = original[(gyi + 1) * origCols + gxi];
            float c11 = original[(gyi + 1) * origCols + gxi + 1];

            float remX = gx - gxi;
            float remY = gy - gyi;
            float m0 = (1 - remX) * c00 + remX * c10;
            float m1 = (1 - remX) * c01 + remX * c11;
            result[y * newCols + x] = (1 - remY) * m0 + remY * m1;
        }
    }
}

void setup(void) {
  // Serial.begin(9600);
  // Serial.print("hello!");

  // Use this initializer if you're using a 1.8" TFT 128x160 displays
  tft.initR(INITR_BLACKTAB);
  // Serial.println("init");

  uint16_t time = millis();
  tft.fillScreen(ST7735_BLACK);
  time = millis() - time;

  // Serial.println(time, DEC);
  // delay(500);

  tft.setRotation(3);


  pinMode(calcStart, OUTPUT);

  Wire.begin();
  Wire.setClock(400000); //Increase I2C clock speed to 400kHz

  // Serial.begin(115200); //Fast serial as possible

  // while (!Serial); //Wait for user to open terminal
  //Serial.println("MLX90640 IR Array Example");

  if (isConnected() == false)
  {
    // Serial.println("MLX90640 not detected at default I2C address. Please check wiring. Freezing.");
    while (1);
  }

  //Get device parameters - We only have to do this once
  int status;
  uint16_t eeMLX90640[832];
  status = MLX90640_DumpEE(MLX90640_address, eeMLX90640);
  if (status != 0)
    tft.setCursor(0, 0);
    tft.setTextColor(ST7735_RED);
    tft.setTextWrap(true);
    tft.print("Failed to load system parameters");
    // Serial.println("Failed to load system parameters");

  status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
  if (status != 0)
    tft.setCursor(0, 0);
    tft.setTextColor(ST7735_RED);
    tft.setTextWrap(true);
    tft.print("Parameter extraction failed");
    // Serial.println("Parameter extraction failed");

  //Once params are extracted, we can release eeMLX90640 array

  //Set refresh rate
  MLX90640_SetRefreshRate(MLX90640_address, 0x06); //Set rate to 4Hz effective - Works

  //Once EEPROM has been read at 400kHz we can increase to 1MHz
  Wire.setClock(1000000); //Teensy will now run I2C at 800kHz (because of clock division)
}

void loop()
{
  // START THERMAL SECTION
  long startTime = millis();
  for (byte x = 0 ; x < 2 ; x++)
  {
    uint16_t mlx90640Frame[834];
    int status = MLX90640_GetFrameData(MLX90640_address, mlx90640Frame);

    digitalWrite(calcStart, HIGH);
    float vdd = MLX90640_GetVdd(mlx90640Frame, &mlx90640);
    float Ta = MLX90640_GetTa(mlx90640Frame, &mlx90640);

    float tr = Ta - TA_SHIFT; //Reflected temperature based on the sensor ambient temperature
    float emissivity = 0.95;

    MLX90640_CalculateTo(mlx90640Frame, &mlx90640, emissivity, tr, mlx90640To);
    digitalWrite(calcStart, LOW);
    //Calculation time on a Teensy 3.5 is 71ms
  }
  long stopReadTime = millis();

  // Set the number of columns and rows
  int columns = 32;
  int rows = 24;
  int newColumns = 160;
  int newRows = 128;
  int resolution = newColumns * newRows;
  float new_mlx90640To[newColumns * newRows]; // Array to hold the result

  interpolate(mlx90640To,new_mlx90640To,columns,rows,newColumns,newRows);

  int _i = 0;
  int _j = 0;
  for (int x = 0 ; x < resolution ; x++)
  {
    _i++;
    if (_i >= newColumns)
    {

      _i = 0;
      _j++;
    }
    uint16_t interpolatedColor = interpolateColor(25, 35, new_mlx90640To[x], ST7735_BLUE, ST7735_RED);
    tft.drawPixel(_i, _j, interpolatedColor);
  }
  long stopPrintTime = millis();

  // Serial.print("Read rate: ");
  // Serial.print( 1000.0 / (stopReadTime - startTime), 2);
  // Serial.println(" Hz");
  // Serial.print("Read plus print rate: ");
  // Serial.print( 1000.0 / (stopPrintTime - startTime), 2);
  // Serial.println(" Hz");
  // END THERMAL SECTION
  
  // draw blue pixels on top, red on bottom
  // for (int16_t i=0; i<tft.width(); i++) {
  //   for (int16_t j=0; j<tft.height(); j++) {
  //     if (j < tft.height()/2) {
  //       tft.drawPixel(i, j, ST7735_BLUE);
  //     }
  //     else{
  //       tft.drawPixel(i, j, ST7735_RED);
  //     }
  //   }
  // }
  // tft.fillScreen(ST7735_BLACK);
}

//Returns true if the MLX90640 is detected on the I2C bus
boolean isConnected()
{
  Wire.beginTransmission((uint8_t)MLX90640_address);
  if (Wire.endTransmission() != 0)
    return (false); //Sensor did not ACK
  return (true);
}