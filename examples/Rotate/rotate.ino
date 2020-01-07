// rotate.ino - Keep track of direction for an Arduino robot using MPU6050
// https://github.com/maarten-pennings/MPU6050
#include <Wire.h>
#include <mpu6050.h>

MPU6050 mpu6050;

float dir;

void setup()  {
  Serial.begin(115200);
  Serial.println("\n\nWelcome to the MPU6050 rotation tracker");
  Serial.println("Driver version " MPU6050_VERSION);
  Wire.begin();

  Serial.println("Starting calibration; leave device flat and still");
  int error= mpu6050.begin(5000); // 5000 calibration steps instead of default 500
  Serial.print("MPU6050: ");
  Serial.println(mpu6050.error_str(error));

  Serial.println("Rotate device to see new direction");
  MPU6050_t data= mpu6050.get();
  dir= data.dir.yaw;
  Serial.println(dir,0);
}

void loop() {
  MPU6050_t data= mpu6050.get();
  if( data.dir.error==0 ) {
    if( abs(data.dir.yaw-dir)>1.0 ) {
      // There is change in direction
      dir= data.dir.yaw;
      Serial.println(dir,0);
    }
  }
}
