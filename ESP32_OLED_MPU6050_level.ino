#include "arduino_secrets.h"

#include <Wire.h>
#include <U8g2lib.h>
#include <MPU6050_light.h>
#include <BMP180.h>

// OLED
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);
//  MPU6050
MPU6050 mpu(Wire);
// BMP 180
BMP180 bmp180;

// MPU6050 Smoothing
#define SMOOTHING_WINDOW 10
float pitch_vals[SMOOTHING_WINDOW] = {0};
float roll_vals[SMOOTHING_WINDOW]  = {0};
int smooth_index = 0;

// Dead zone for motion and level check
#define MOTION_DEAD_ZONE 1.5
#define LEVEL_TOLERANCE 3.0  // degrees from center to be considered level

void setup() {
  Wire.begin();  // SDA = 21, SCL = 22 on ESP32
  Serial.begin(115200);
  u8g2.begin();

  //  MPU6050 start
  byte status = mpu.begin();
  if (status != 0) {
    Serial.print("MPU6050 init failed with code: ");
    Serial.println(status);
    while (1);
  }

  delay(1000);
  mpu.calcOffsets(true, true);

  //  BMP180 start
      if (!bmp180.begin())
    {
        Serial.println("BMP180 not found!");
        while (1);
    }
  
}

void loop() {
//  MPU6050
  mpu.update();

  // Read angles
  float pitch = mpu.getAngleX();
  float roll  = mpu.getAngleY();

  // Smoothing
  pitch_vals[smooth_index] = pitch;
  roll_vals[smooth_index] = roll;
  smooth_index = (smooth_index + 1) % SMOOTHING_WINDOW;

  float smooth_pitch = 0;
  float smooth_roll = 0;
  for (int i = 0; i < SMOOTHING_WINDOW; i++) {
    smooth_pitch += pitch_vals[i];
    smooth_roll  += roll_vals[i];
  }
  smooth_pitch /= SMOOTHING_WINDOW;
  smooth_roll  /= SMOOTHING_WINDOW;

  // Apply dead zone for display motion
  if (abs(smooth_pitch) < MOTION_DEAD_ZONE) smooth_pitch = 0;
  if (abs(smooth_roll) < MOTION_DEAD_ZONE) smooth_roll = 0;

  // Convert angles to screen coordinates
  int centerX = 32;
  int centerY = 32;
  int x = constrain(map((int)smooth_roll, -45, 45, centerX - 30, centerX + 30), 0, 63);
  int y = constrain(map((int)smooth_pitch, -45, 45, centerY - 30, centerY + 30), 0, 63);

  // Check if level
  bool isLevel = abs(smooth_pitch) < LEVEL_TOLERANCE && abs(smooth_roll) < LEVEL_TOLERANCE;

//  BMP180
    Serial.print("Pressure: ");
    Serial.print(bmp180.readPressure());
    Serial.println(" Pa");
  
    Serial.print("Temperature: ");
    Serial.print(bmp180.readTemperature());
    Serial.println(" Â°C");
      
    Serial.print("Altitude: ");
    Serial.print(bmp180.readAltitude());
    Serial.println(" Pa");
  
  // Draw everything
  u8g2.clearBuffer();

  // Draw crosshairs
  u8g2.drawLine(centerX - 5, centerY, centerX + 5, centerY);
  u8g2.drawLine(centerX, centerY - 5, centerX, centerY + 5);

  // Draw level zone (target area)
  u8g2.drawCircle(centerX, centerY, 5, U8G2_DRAW_ALL); // 10px zone

  // Draw frame
  u8g2.drawRFrame(0, 0, 65, 64, 8);
  
  // Draw the moving dot
  if (isLevel) {
    // Draw filled dot to indicate level
    u8g2.drawDisc(x, y, 3, U8G2_DRAW_ALL);
  } else {
    u8g2.drawCircle(x, y, 3, U8G2_DRAW_ALL);
  }
  
  // Text indicators
  u8g2.setFont(u8g2_font_5x7_tf);
  //  MPU6050
  u8g2.setCursor(70, 10); u8g2.print("Pitch: "); u8g2.print(smooth_pitch, 1);
  u8g2.setCursor(70, 20); u8g2.print("Roll : ");  u8g2.print(smooth_roll, 1);
  
  if (isLevel) {
    u8g2.setCursor(5, 10);
    u8g2.print("-- Level --");
  }
  //  BMP180
  u8g2.setCursor(70, 40); u8g2.print("Skirt");
  u8g2.setCursor(70, 50); u8g2.print(bmp180.readPressure()); u8g2.print("Pa");
  u8g2.setCursor(70, 60); u8g2.print(bmp180.readTemperature()); u8g2.print(" Â°C");
  
  u8g2.sendBuffer();
  delay(40);
}
