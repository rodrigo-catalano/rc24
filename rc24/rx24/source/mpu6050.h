typedef struct
  {
     int16 x_accel;
     int16 y_accel;
     int16 z_accel;
     int16 temperature;
     int16 x_gyro;
     int16 y_gyro;
     int16 z_gyro;
   } mpu6050_raw;


typedef struct
{
	uint8 I2Caddress;
	uint8 whoami;
	mpu6050_raw rawValues;
}mpu6050;

bool initMpu6050(mpu6050* mpu);
bool readMpu6050(mpu6050* mpu);
