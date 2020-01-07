// pid.ino - Two-wheel drive robot with gyro and PID for going straight
// Uses MPU6050 gyro
// Uses motor driver L2930D and two TT motors


// MPU6050 ===================================================
// Robot has a yaw (direction) sensor

#include <Wire.h>
#include <mpu6050.h>

MPU6050 mpu6050;

void mpu6050_begin()  {
  Wire.begin();
  Serial.println("Starting calibration; leave device flat and still");
  int error= mpu6050.begin(2000); // 2000 calibration steps instead of default 500
  Serial.print("MPU6050: ");
  Serial.println(mpu6050.error_str(error));
}

float mpu6050_yaw() {
  MPU6050_t data= mpu6050.get();
  while( data.dir.error!=0 ) { 
    // I suffer from a lot of I2C problems
    Serial.println(mpu6050.error_str(data.dir.error));
    // Reset I2C
    TWCR= 0; Wire.begin();
    // Reread
    data= mpu6050.get();
  }
  return data.dir.yaw;
}

// Button ===================================================
// Robot has an emergency button to start and stop it

#define BUTTON_PIN  13

void button_begin() {
  pinMode(BUTTON_PIN,INPUT);
  Serial.println("Button : ok");
}

int button_down() {
  return  digitalRead(BUTTON_PIN);
}

// Motors ===================================================
// Robot has two motors, one for the right wheel and one for the left

#define MOTOR_A_IN1  3
#define MOTOR_A_IN2  2
#define MOTOR_A_ENA  5

#define MOTOR_B_IN1  7
#define MOTOR_B_IN2  8
#define MOTOR_B_ENA  6

void motor_begin() {
  analogWrite(MOTOR_A_ENA, 0);
  analogWrite(MOTOR_B_ENA, 0);
  
  pinMode(MOTOR_A_IN1, OUTPUT);
  pinMode(MOTOR_A_IN2, OUTPUT);
  pinMode(MOTOR_A_ENA, OUTPUT);

  pinMode(MOTOR_B_IN1, OUTPUT);
  pinMode(MOTOR_B_IN2, OUTPUT);
  pinMode(MOTOR_B_ENA, OUTPUT);  

  Serial.println("Motors : ok");
}

// Set motor A to given speed (-255..+255); 0 switches off
void motor_A_set( int speed ) {
  if( speed==0 ) {
    digitalWrite( MOTOR_A_IN1, HIGH );
    digitalWrite( MOTOR_A_IN2, HIGH );
    analogWrite(MOTOR_A_ENA, 0);    
  } else if( speed>0 ) {
    digitalWrite( MOTOR_A_IN1, HIGH );
    digitalWrite( MOTOR_A_IN2, LOW );
    analogWrite ( MOTOR_A_ENA, speed);        
  } else {
    digitalWrite( MOTOR_A_IN1, LOW );
    digitalWrite( MOTOR_A_IN2, HIGH );
    analogWrite ( MOTOR_A_ENA, -speed);            
  }
}

// Set motor B to given speed (-255..+255); 0 switches off
void motor_B_set( int speed ) {
  if( speed==0 ) {
    digitalWrite( MOTOR_B_IN1, HIGH );
    digitalWrite( MOTOR_B_IN2, HIGH );
    analogWrite(MOTOR_B_ENA, 0);    
  } else if( speed>0 ) {
    digitalWrite( MOTOR_B_IN1, HIGH );
    digitalWrite( MOTOR_B_IN2, LOW );
    analogWrite ( MOTOR_B_ENA, speed);        
  } else {
    digitalWrite( MOTOR_B_IN1, LOW );
    digitalWrite( MOTOR_B_IN2, HIGH );
    analogWrite ( MOTOR_B_ENA, -speed);            
  }
}

#define MOTOR_NOMINAL  200
#define MOTOR_DELTAMAX  50

// Switches both motors off
void motor_off() {
  motor_A_set(0);
  motor_B_set(0);
}

// Switches both motors to forward (to speed MOTOR_NOMINAL), but B motor 'delta' 
// faster than A motor (delta in range -MOTOR_DELTAMAX..+-MOTOR_DELTAMAX)
void motor_forward(int delta) {
  if( delta>+MOTOR_DELTAMAX ) delta= +MOTOR_DELTAMAX;
  if( delta<-MOTOR_DELTAMAX ) delta= -MOTOR_DELTAMAX;
  motor_A_set(MOTOR_NOMINAL-delta);
  motor_B_set(MOTOR_NOMINAL+delta);
}


// PID ===================================================

#define PID_K_p 5
#define PID_K_i 0
#define PID_K_d 0

float i_input;
float d_last;

void pid_begin() {
  i_input= 0;
  d_last= 0;  
  Serial.println("PID    : ok");
}

int pid(float error) {
  float p_input;
  float d_input;
    
  p_input= error;
  i_input+= error;
  d_input= error-d_last; d_last=error;

  return p_input*PID_K_p + i_input*PID_K_i + d_input*PID_K_d;
}

// Main ===================================================

int state;

void setup() {
  Serial.begin(115200);
  Serial.println("Welcome at PID robot");
  button_begin();
  motor_begin();
  pid_begin();
  mpu6050_begin();

  motor_off();
  state= 0;
}

float target_dir;

void loop() {
  float current_dir;
  int steer;
  switch( state ) {
    case 0:
      // Motors off, prompt user
      Serial.println("Waiting for button press");
      state= 1;
      break;
    case 1:
      // Motors off, waiting for button press
      if( button_down() ) state=2;
      break;
    case 2:
      // Motors were off, user pressed button, switch on
      delay(500); // give user some time to release button
      target_dir= mpu6050_yaw(); // Take current direction as target
      motor_forward(0); // Motors on, 0=steering straight
      Serial.println("Motors started");
      state= 3;
      break;
    case 3:
      // Motors running, PID active
      current_dir= mpu6050_yaw();
      Serial.print(" dir="); Serial.print(current_dir,0);
      Serial.print(" tgt="); Serial.print(target_dir,0);
      steer= pid( target_dir - current_dir );
      Serial.print(" steer="); Serial.println(steer);
      motor_forward(steer);  
      if( button_down() ) state=4;
      break;
    case 4:
      // Motors were on, user pressed button, switch off
      motor_off();
      Serial.println("Stopped"); 
      delay(500); // give user some time to release button
      state= 0;
      break;
  }
}
