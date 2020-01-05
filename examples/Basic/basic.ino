// basic.ino - Basic example, printing all data from MPU6050
// https://github.com/maarten-pennings/MPU6050
#include <mpu6050.h>

MPU6050 sensor;

void setup()  {
  Serial.begin(115200);
  Serial.println("\n\nWelcome to the MPU6050 basic example");
  Serial.println("Driver version " MPU6050_VERSION);

  Serial.print("MPU6050: sensor is ... ");
  Serial.println( sensor.absent() ? "absent" : "present" ); 
  
  int error= sensor.begin();
  Serial.print("MPU6050: ");
  Serial.println(sensor.error_str(error));

  // Give user some time to see potential error messages from startup
  delay(5000);
}

void loop() {
  MPU6050_t data= sensor.get();
  Serial.print(" ae="); Serial.print(data.accel.error);
  Serial.print(" ax="); Serial.print(data.accel.x);
  Serial.print(" ay="); Serial.print(data.accel.y);
  Serial.print(" az="); Serial.print(data.accel.z);
  Serial.print(" /");

  Serial.print(" ge="); Serial.print(data.gyro.error);
  Serial.print(" gx="); Serial.print(data.gyro.x);
  Serial.print(" gy="); Serial.print(data.gyro.y);
  Serial.print(" gz="); Serial.print(data.gyro.z);
  Serial.print(" /");

  Serial.print(" de="); Serial.print(data.dir.error);
  Serial.print(" dx="); Serial.print(data.dir.roll);
  Serial.print(" dy="); Serial.print(data.dir.pitch);
  Serial.print(" dz="); Serial.print(data.dir.yaw);
  Serial.print(" /");

  Serial.print(" te="); Serial.print(data.temp.error);
  Serial.print(" tt="); Serial.print(data.temp.t);

  Serial.println();
}
