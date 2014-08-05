typedef struct
  {
     int16 x_accel;
     int16 y_accel;
     int16 z_accel;
     int16 temperature;
     int16 x_gyro;
     int16 y_gyro;
     int16 z_gyro;
     int x_mag;
     int y_mag;
     int z_mag;
     int x_mag_sens;
     int y_mag_sens;
     int z_mag_sens;

  } mpu6050_raw;


typedef struct
{
	uint8 I2Caddress;
	uint8 compassI2Caddress;
	uint8 whoami;
	mpu6050_raw rawValues;
}mpu6050;

bool initMpu6050(mpu6050* mpu);
bool readMpu6050(mpu6050* mpu);

typedef struct
{
	uint8 addr;
	uint16 sens;
	uint16 off;
	uint16 tcs;
	uint16 tco;
	uint16 tref;
	uint16 tempsens;
	int temp;
	int pressure;
	int state;
	int dt;
} MS5611;
