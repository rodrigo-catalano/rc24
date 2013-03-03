typedef struct
{
	routedObject ro; //first item so struct can be cast to a routedObject
	bool enabled;
	int32 accel_x;
	int32 accel_y;
	int32 accel_z;
	int32 gyro_x;
	int32 gyro_y;
	int32 gyro_z;

} IMU;

void createIMU(IMU* imu);
void IMUreadSensors(IMU* imu);
