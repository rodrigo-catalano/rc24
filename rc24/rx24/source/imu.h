typedef struct
{
	int32 x;
	int32 y;
	int32 z;
} vector3_fp12;

typedef struct
{
	routedObject ro; //first item so struct can be cast to a routedObject
	bool enabled;
	int32 accel_x;
	int32 accel_y;
	int32 accel_z;
	int32 accel_x_offset;
	int32 accel_y_offset;
	int32 accel_z_offset;
	int32 gyro_x;
	int32 gyro_y;
	int32 gyro_z;
	int32 gyro_x_offset;
	int32 gyro_y_offset;
	int32 gyro_z_offset;
	int32 mag_x_offset;
	int32 mag_y_offset;
	int32 mag_z_offset;
	int32 gravity;
	int32 accel_xLPF;
	int32 accel_yLPF;
	int32 accel_zLPF;

	int32 roll;
	int32 pitch;
	int32 yaw;
	float x;
	float y;
	float z;
	float vx;
	float vy;
	float vz;
	float az;

	int temp_zero;
	int pressure_zero;
	float altz;

	vector3_fp12 down;
	vector3_fp12 north;


	quaternionf attitude;
	quaternionf attitudeLPF;
	quaternionf	gmLPF;

	int32 timer;
	int32 dt;
	uint32 lastClock;
	int zeroing;
} IMU;


void createIMU(IMU* imu);
void IMUreadSensors(IMU* imu);
void startZeroing(IMU* imu);
