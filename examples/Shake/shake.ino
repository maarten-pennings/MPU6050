// shake.ino - a shake detector using MPU6050
// https://github.com/maarten-pennings/MPU6050
#include <mpu6050.h>

MPU6050 sensor;

void setup()  {
  Serial.begin(115200);
  Serial.println("\n\nWelcome to the MPU6050 shake detector");
  Serial.println("Driver version " MPU6050_VERSION);

  int error= sensor.begin();
  Serial.print("MPU6050: ");
  Serial.println(sensor.error_str(error));

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.println("When the device is shaken, its LED will blink");
}

void loop() {
  MPU6050_t data= sensor.get();
  if( data.accel.error==0 ) {
    // Compute magnitude of accelaration over all three axis
    float magnitude= sqrt(data.accel.x*data.accel.x + data.accel.y*data.accel.y + data.accel.z*data.accel.z); 
    // Turn on LED if acceleration exceeds 2g
    if( magnitude > 2*MPU6050_GRAVITY_EARTH ) {
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.println("Shaken");
    } else {
      digitalWrite(LED_BUILTIN, LOW);
    }
  }
}