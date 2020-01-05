// rotate.ino - Keep track of direction for an Arduino robot using MPU6050
// https://github.com/maarten-pennings/MPU6050
#include <mpu6050.h>

MPU6050 sensor;

float dir;

void setup()  {
  Serial.begin(115200);
  Serial.println("\n\nWelcome to the MPU6050 rotation tracker");
  Serial.println("Driver version " MPU6050_VERSION);

  Serial.println("Starting calibration; leave device flat and still");
  int error= sensor.begin(5000); // 5000 calibration steps instead of default 500
  Serial.print("MPU6050: ");
  Serial.println(sensor.error_str(error));

  Serial.println("Rotate device to see new direction");
  MPU6050_t data= sensor.get();
  dir= data.dir.yaw;
  Serial.println(dir,0);
}

void loop() {
  MPU6050_t data= sensor.get();
  if( data.accel.error==0 ) {
    if( abs(data.dir.yaw-dir)>1.0 ) {
      // There is change in direction
      dir= data.dir.yaw;
      Serial.println(dir,0);
    }
  }
}
