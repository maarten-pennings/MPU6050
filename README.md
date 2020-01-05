# MPU6050
Arduino library for the MPU6050 accelerometer and gyroscope sensor with I2C interface from InvenSense.

## Introduction
The MPU6050 is a motion tracking [device](https://www.invensense.com/products/motion-tracking/6-axis/mpu-6050/) 
from InvenSense. It is I2C controlled, contains a 3-axis gyroscope and a 3-axis Accelerometer (and a thermometer). 
InvenSense has published a
[datasheet](https://43zrtwysvxb2gf29r5o0athu-wpengine.netdna-ssl.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf)
but for driver development, the 
[register map](https://43zrtwysvxb2gf29r5o0athu-wpengine.netdna-ssl.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf)
is also crucial.

### Background
An [accelerometer](https://en.wikipedia.org/wiki/Accelerometer) is a device that measures acceleration. 
For example, an accelerometer at rest on the surface of the Earth will measure an acceleration due to 
Earth's gravity of g ≈ 9.81 m/s2. In a typical implementation a mass is hung with one degree of freedom;
it moves due to acceleration. Accelerometers are used in mobile phone and digital cameras so that images 
on screens are always displayed upright. Accelerometers are used in drones for flight stabilisation and 
for drop detection.

A [gyroscope](https://en.wikipedia.org/wiki/Gyroscope) is a device used for measuring or maintaining orientation.
A typical implementation is a spinning wheel in which the axis of rotation is free to assume any orientation by itself. 
Applications of gyroscopes include  navigation systems.

Gyroscope suffer from drift, so accelerometers are used to compensate for that.

### This project
This project implements an Arduino library for the MPU6050. It comes with some examples.

Note that the MPU6050 requires a supply voltage of 3.3V. So, Arduino's 5.0V *not* OK. 
However, some boards, like the GY521 have an on-board voltage regulator and level shifter.



