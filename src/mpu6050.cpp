// mpu6050.cpp - Driver for the MPU6050 accelerometer and gyroscope.
// https://github.com/maarten-pennings/MPU6050
// Created by Maarten Pennings, based on
//   https://github.com/Th-Havy/Simple-MPU6050-Arduino and 
//   https://howtomechatronics.com/tutorials/arduino/arduino-and-mpu6050-accelerometer-and-gyroscope-tutorial/ for updateDirection()
//   https://www.nxp.com/files-static/sensors/doc/app_note/AN3461.pdf for angle_x() and angle_y()

#include <Wire.h>
#include "MPU6050.h"

// Selected MPU6050 register addresses, see 
// https://43zrtwysvxb2gf29r5o0athu-wpengine.netdna-ssl.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf
#define MPU6050_REGISTER_SMPRT_DIV    0x19 // Sample rate divider
#define MPU6050_REGISTER_CONFIG       0x1A // DLPF config
#define MPU6050_REGISTER_GYRO_CONFIG  0x1B
#define MPU6050_REGISTER_ACCEL_CONFIG 0x1C
#define MPU6050_REGISTER_ACCEL_XOUT_H 0x3B // Accelerometer data (6 bytes)
#define MPU6050_REGISTER_TEMP_OUT_H   0x41 // Temperature data (2 bytes)
#define MPU6050_REGISTER_GYRO_XOUT_H  0x43 // Gyroscope data (6 bytes)
#define MPU6050_REGISTER_PWR_MGMT_1   0x6B // Power management
#define MPU6050_REGISTER_WHO_AM_I     0x75 // Contains address of the device (0x68)

// Default I2C address of the MPU6050 (AD0 is assumed low; if AD0 is high then 0x69)
#define MPU6050_DEFAULT_ADDRESS       0x68

// Constant to convert raw temperature to Celsius degrees
#define MPU6050_TEMP_LINEAR_COEF      (1.0/340.00)
#define MPU6050_TEMP_OFFSET           36.53

// Constant to convert raw gyroscope to degree/s
#define MPU6050_GYRO_FACTOR_250       (1.0/131.0)
#define MPU6050_GYRO_FACTOR_500       (1.0/65.5)
#define MPU6050_GYRO_FACTOR_1000      (1.0/32.8)
#define MPU6050_GYRO_FACTOR_2000      (1.0/16.4)

// Constant to convert raw acceleration to m/s^2
#define MPU6050_ACCEL_FACTOR_2        (MPU6050_GRAVITY_EARTH / 16384.0)
#define MPU6050_ACCEL_FACTOR_4        (MPU6050_GRAVITY_EARTH / 8192.0)
#define MPU6050_ACCEL_FACTOR_8        (MPU6050_GRAVITY_EARTH / 4096.0)
#define MPU6050_ACCEL_FACTOR_16       (MPU6050_GRAVITY_EARTH / 2048.0)

MPU6050::MPU6050() {
    _devaddress= MPU6050_DEFAULT_ADDRESS;
}

int MPU6050::begin(int calibrationsamples, MPU6050_AccelRange ar, MPU6050_GyroRange gr, MPU6050_DLPFBandwidth bw, MPU6050_SampleRateDiv sr) {
    int error;
    error= absent();                      if( error ) return error;
    error= wake();                        if( error ) return error;
    error= setSampleRateDivider(sr);      if( error ) return error;
    error= setAccelRange(ar);             if( error ) return error;
    error= setGyroRange(gr);              if( error ) return error;
    error= setDLPFBandwidth(bw);          if( error ) return error;
    
    if( calibrationsamples==0 ) {
        // No corrections will be applied
        _calibrate.accel_x= _calibrate.accel_y= _calibrate.gyro_x= _calibrate.gyro_y= _calibrate.gyro_z= 0;
    } else {
        error= calibrate(calibrationsamples); if( error ) return error;
    }

    _direction.gyro_time_ms= millis();         
    _direction.gyro_angle_x= 0;
    _direction.gyro_angle_y= 0;
    _direction.gyro_angle_z= 0;
    
    return 0;
}

MPU6050_t MPU6050::get() {
    MPU6050_t all;

    all.accel= readAcceleration();
    all.gyro = readGyroscope();
    updateDirection( all.accel, all.gyro);
    all.dir  = _direction;
    
    all.temp = readTemperature();

    return all;
}

void MPU6050::updateDirection(MPU6050_Accel_t accel,MPU6050_Gyro_t gyro) {
    // Don't update if there is an I2C error
    if( accel.error!=0 ) { _direction.error= accel.error; return; }
    if( gyro.error!=0 )  { _direction.error= gyro.error; return; }
    _direction.error= 0;
    // Compute angle from accelerometer
    float accel_angle_x= angle_x(accel);
    float accel_angle_y= angle_y(accel);
    // Compute elapsed time in seconds since previous call
    uint32_t now= millis();
    float delta_time= (now - _direction.gyro_time_ms ) / 1000.0; 
    _direction.gyro_time_ms= now;
    // Integrate gyro angle - multiply degrees/sec with sec
    _direction.gyro_angle_x += gyro.x * delta_time; 
    _direction.gyro_angle_y += gyro.y * delta_time;
    _direction.gyro_angle_z += gyro.z * delta_time;
    // Complementary filter - combine accelerometer and gyroscope angle values
    _direction.roll  = 0.96 * _direction.gyro_angle_x  +  0.04 * accel_angle_x;
    _direction.pitch = 0.96 * _direction.gyro_angle_y  +  0.04 * accel_angle_y;
    _direction.yaw   = 1.00 * _direction.gyro_angle_z;
}

MPU6050_Accel_t MPU6050::readAcceleration() {
    // Read acceleration data from MPU6050
    int16_t accel_raw_x, accel_raw_y, accel_raw_z;
    int error= read3x16( MPU6050_REGISTER_ACCEL_XOUT_H, (uint16_t*)&accel_raw_x, (uint16_t*)&accel_raw_y, (uint16_t*)&accel_raw_z ); // 16 bit regs are signed
    // Convert and pack into data struct
    MPU6050_Accel_t accel;
    accel.error= error;
    accel.x = rawAccelerationToMps2(accel_raw_x);
    accel.y = rawAccelerationToMps2(accel_raw_y);
    accel.z = rawAccelerationToMps2(accel_raw_z);
    // Return data
    return accel;
}

MPU6050_Gyro_t MPU6050::readGyroscope() {
    // Read gyroscope data from MPU6050
    int16_t gyro_raw_x, gyro_raw_y, gyro_raw_z;
    int error= read3x16( MPU6050_REGISTER_GYRO_XOUT_H, (uint16_t*)&gyro_raw_x, (uint16_t*)&gyro_raw_y, (uint16_t*)&gyro_raw_z ); // 16 bit regs are signed
    // Pack into data struct, convert and apply calibration
    MPU6050_Gyro_t gyro;
    gyro.error= error;
    gyro.x = rawGyroscopeToDps(gyro_raw_x) - _calibrate.gyro_x;
    gyro.y = rawGyroscopeToDps(gyro_raw_y) - _calibrate.gyro_y;
    gyro.z = rawGyroscopeToDps(gyro_raw_z) - _calibrate.gyro_z;
    // Return data
    return gyro;
}

MPU6050_Temp_t MPU6050::readTemperature() {
    // Read temperature data from MPU6050
    int16_t temp_raw_t;
    int error= read16(MPU6050_REGISTER_TEMP_OUT_H,(uint16_t*)&temp_raw_t); // 16 bit reg is signed
    // Convert and pack into data struct
    MPU6050_Temp_t temp;
    temp.error= error;
    temp.t= rawTemperatureToCelsius(temp_raw_t);
    // Return data
    return temp;
}

int MPU6050::setAccelRange(MPU6050_AccelRange range) {
    uint8_t field = static_cast<int>(range) << 3;
    // We need a read-modify-write to update the field
    uint8_t val;
    int error1= read8(MPU6050_REGISTER_ACCEL_CONFIG, &val);
    if( error1!=0 ) return error1;
    val = (val & 0b11100111) | field;
    int error2= write8(MPU6050_REGISTER_ACCEL_CONFIG, val);
    if( error2!=0 ) return error2;
    _accelrange = range;
    return 0;
}

int MPU6050::setGyroRange(MPU6050_GyroRange range) {
    uint8_t field = static_cast<int>(range) << 3;
    // We need a read-modify-write to update the field
    uint8_t val;
    int error1= read8(MPU6050_REGISTER_GYRO_CONFIG, &val);
    if( error1!=0 ) return error1;
    val = (val & 0b11100111) | field;
    int error2= write8(MPU6050_REGISTER_GYRO_CONFIG, val);
    if( error2!=0 ) return error2;
    _gyrorange = range;
    return 0;
}

int MPU6050::setDLPFBandwidth(MPU6050_DLPFBandwidth bandwidth) {
    uint8_t field = static_cast<uint8_t>(bandwidth);
    // We need a read-modify-write to update the field
    uint8_t val;
    int error1= read8(MPU6050_REGISTER_CONFIG, &val);
    if( error1!=0 ) return error1;
    val = (val & 0b11111000) | field;
    int error2= write8(MPU6050_REGISTER_CONFIG, val);
    return error2;
}

int MPU6050::setSampleRateDivider(MPU6050_SampleRateDiv divider) {
    uint8_t field = static_cast<uint8_t>(divider);
    int error= write8(MPU6050_REGISTER_SMPRT_DIV, field);
    return error;
}


float MPU6050::angle_x(MPU6050_Accel_t accel) {
    float radian= atan( +accel.y / sqrt(pow(accel.x,2)+pow(accel.z,2)) );
    return radian * 180/PI - _calibrate.accel_x;
}

float MPU6050::angle_y(MPU6050_Accel_t accel) {
    float radian= atan( -accel.x / sqrt(pow(accel.y,2)+pow(accel.z,2)) );
    return radian * 180/PI - _calibrate.accel_y;
}

#define MPU6050_MINSAMPLES 10
int MPU6050::calibrate(int numsamples) {
    MPU6050_Calibrate_t cal;
    // Check that numsamples is big "enough"
    if( numsamples<MPU6050_MINSAMPLES ) return 13;
    
    // Calibrate Accelerometer
   
    // Reset accel calibration parameters; they are used in readAcceleration()
    _calibrate.accel_x= 0;
    _calibrate.accel_y= 0;
    // Now take 'numsamples' accel samples
    cal.accel_x= 0;
    cal.accel_y= 0;
    int numerrors= 0;
    for( int sample=0; sample<numsamples; sample++ ) {
        MPU6050_Accel_t accel= readAcceleration();
        if( accel.error!=0 ) { numerrors++; continue; } // Reject sample on I2C error
        cal.accel_x+= angle_x(accel);
        cal.accel_y+= angle_y(accel);
    }
    // Reject calibration if too many I2C errors
    if( numsamples-numerrors < MPU6050_MINSAMPLES ) return 13;
    // Store new accel calibration parameters
    _calibrate.accel_x= cal.accel_x/numsamples;
    _calibrate.accel_y= cal.accel_y/numsamples;
    
    // Calibrate Gyroscope
    
    // Reset gyro calibration parameters; they are used in readGyroscope()
    _calibrate.gyro_x= 0;
    _calibrate.gyro_y= 0;
    _calibrate.gyro_z= 0;
    // Now take 'numsamples' gyro samples
    cal.gyro_x= 0;
    cal.gyro_y= 0;
    cal.gyro_z= 0;
    numerrors= 0;
    for( int sample=0; sample<numsamples; sample++ ) {
        MPU6050_Gyro_t gyro= readGyroscope();
        if( gyro.error!=0 ) { numerrors++; continue; } // Reject sample on I2C error
        cal.gyro_x+= gyro.x;
        cal.gyro_y+= gyro.y;
        cal.gyro_z+= gyro.z;
    }
    // Reject calibration if too many I2C errors
    if( numsamples-numerrors < MPU6050_MINSAMPLES ) return 13;
    // Store new gyro calibration parameters
    _calibrate.gyro_x= cal.gyro_x/numsamples;
    _calibrate.gyro_y= cal.gyro_y/numsamples;
    _calibrate.gyro_z= cal.gyro_z/numsamples;
    
    // Successful calibration
    // Serial.println(_calibrate.accel_x);
    // Serial.println(_calibrate.accel_y);
    // Serial.println(_calibrate.gyro_x);
    // Serial.println(_calibrate.gyro_y);
    // Serial.println(_calibrate.gyro_z);
    return 0;
}

// Resets all registers; begin() should be called to use the device after reset()
int MPU6050::reset() {
    return write8(MPU6050_REGISTER_PWR_MGMT_1, 0b10000000);
}

// Returns 0 if the MPU6050 can be reached via I2C, and has correct who-am-i 
int MPU6050::absent() {
    uint8_t val;
    int error= read8(MPU6050_REGISTER_WHO_AM_I,&val);
    if( error!=0 ) return error;
    if( val!= MPU6050_DEFAULT_ADDRESS ) return 13;
    return 0;
}

// Sensor by default in sleep (power saving mode, no data measurements); begin() causes wake()
int MPU6050::sleep() {
    return write8(MPU6050_REGISTER_PWR_MGMT_1, 0b01000000);
}

// Wake device up
int MPU6050::wake() {
    return write8(MPU6050_REGISTER_PWR_MGMT_1, 0b00000000);
}

float MPU6050::rawTemperatureToCelsius(int16_t temp_raw_t) {
    return temp_raw_t * MPU6050_TEMP_LINEAR_COEF + MPU6050_TEMP_OFFSET;
}

float MPU6050::rawGyroscopeToDps(int16_t rawGyroscope) {
    switch( _gyrorange ) {
        case Max250Dps : return rawGyroscope * MPU6050_GYRO_FACTOR_250;
        case Max500Dps : return rawGyroscope * MPU6050_GYRO_FACTOR_500;
        case Max1000Dps: return rawGyroscope * MPU6050_GYRO_FACTOR_1000;
        case Max2000Dps: return rawGyroscope * MPU6050_GYRO_FACTOR_2000;
        default        : return 0.0;
    }
}

float MPU6050::rawAccelerationToMps2(int16_t rawAcceleration) {
    switch( _accelrange ) {
        case Max2g : return rawAcceleration * MPU6050_ACCEL_FACTOR_2;
        case Max4g : return rawAcceleration * MPU6050_ACCEL_FACTOR_4;
        case Max8g : return rawAcceleration * MPU6050_ACCEL_FACTOR_8;
        case Max16g: return rawAcceleration * MPU6050_ACCEL_FACTOR_16;
        default    : return 0.0;
    }
}

// Read 'value' from the registers starting at 'addr'. Returns error; see error_str() for explanation.
int MPU6050::read8(uint8_t addr, uint8_t *value) {
    *value=0;
    Wire.beginTransmission(_devaddress);
    int r1=Wire.write(addr);                                       if( r1!=1 ) return 10;
    int r2=Wire.endTransmission(false);                            if( r2!=0 ) return r2;
    int r3=Wire.requestFrom(_devaddress,(uint8_t)1,(uint8_t)true); if( r3!=1 ) return 11;
    *value=Wire.read();
    return 0;
}

// Read 'value' from the registers starting at 'addr'. Returns error; see error_str() for explanation.
int MPU6050::read16(uint8_t addr, uint16_t *value ) {
    *value=0;
    Wire.beginTransmission(_devaddress);
    int r1=Wire.write(addr);                                       if( r1!=1 ) return 10;
    int r2=Wire.endTransmission(false);                            if( r2!=0 ) return r2;
    int r3=Wire.requestFrom(_devaddress,(uint8_t)2,(uint8_t)true); if( r3!=2 ) return 11;
    *value=Wire.read()<<8 | Wire.read(); 
    return 0;
}

// Read 3x'value' from the 3 registers starting at 'addr'. Returns error; see error_str() for explanation.
int MPU6050::read3x16(uint8_t addr, uint16_t *value0,  uint16_t *value1, uint16_t *value2 ) {
    *value0= *value1= *value2= 0;
    Wire.beginTransmission(_devaddress);
    int r1=Wire.write(addr);                                       if( r1!=1 ) return 10;
    int r2=Wire.endTransmission(false);                            if( r2!=0 ) return r2;
    int r3=Wire.requestFrom(_devaddress,(uint8_t)6,(uint8_t)true); if( r3!=6 ) return 11;
    *value0=Wire.read()<<8 | Wire.read(); 
    *value1=Wire.read()<<8 | Wire.read(); 
    *value2=Wire.read()<<8 | Wire.read(); 
    return 0;
}

// Write 'value' to register at address 'addr'. Returns error; see error_str() for explanation.
int MPU6050::write8(uint8_t addr, uint8_t value) {
    Wire.beginTransmission(_devaddress);
    int r1=Wire.write(addr);                                       if( r1!=1 ) return 10;
    int r2=Wire.write(value);                                      if( r2!=1 ) return 10;
    int r3=Wire.endTransmission(true);                             if( r3!=0 ) return r3;
    return 0;
}

// TODO: put these constants in F() and use PROGMEM. But how?
const char* MPU6050::error_str(int error ) {
    switch( error ) {
        case  0: return "success";
        case  1: return "endTransmission(): too much data";
        case  2: return "endTransmission(): NACK on address";
        case  3: return "endTransmission(): NACK on data";
        case  4: return "endTransmission(): other";
        case  5: return "endTransmission(): timeout"; // I hacked my twi.c to add timeout, which gives the error code
        case 10: return "write(): size mismatch";
        case 11: return "requestFrom(): size mismatch";
        case 12: return "present(): who-am-i mismatch";
        case 13: return "calibrate(): too few samples";
        default: return "<unknown>";        
    }
}

