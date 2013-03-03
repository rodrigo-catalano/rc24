#include <jendefs.h>
#include <string.h>
#include "routedMessage.h"
#include "commonCommands.h"
#include "routedObject.h"
#include "imu.h"
#include "mpu6050.h"

// select visible parameters

int aaa=23;
ccParameter imuParameters[]=
{
	//name,type,paramPtr,paramOffset,array length,set function,get function
	{ "enabled",CC_BOOL,NULL,offsetof(IMU,enabled),0,CC_NO_GETTER,CC_NO_SETTER},
	{ "accel x",CC_INT32,NULL,offsetof(IMU,accel_x),0,CC_NO_GETTER,CC_NO_SETTER},
	{ "accel y",CC_INT32,NULL,offsetof(IMU,accel_y),0,CC_NO_GETTER,CC_NO_SETTER},
	{ "accel z",CC_INT32,NULL,offsetof(IMU,accel_z),0,CC_NO_GETTER,CC_NO_SETTER},
	{ "gyro x",CC_INT32,NULL,offsetof(IMU,gyro_x),0,CC_NO_GETTER,CC_NO_SETTER},
	{ "gyro y",CC_INT32,NULL,offsetof(IMU,gyro_y),0,CC_NO_GETTER,CC_NO_SETTER},
	{ "gyro z",CC_INT32,NULL,offsetof(IMU,gyro_z),0,CC_NO_GETTER,CC_NO_SETTER},
	{ "read sensors",CC_VOID_FUNCTION,NULL,0,0,CC_NO_GETTER,(VOID_FN)IMUreadSensors},
};
ccParameterList imuParameterList=
{
	imuParameters,
	sizeof(imuParameters)/sizeof(imuParameters[0])
};
mpu6050 mpu;
void createIMU(IMU* imu)
{
	imu->ro.parameters=imuParameterList;
	imu->ro.messageHandler=&routedObjectHandleMessage;
	mpu.I2Caddress=0x68;
	if(imu->enabled)initMpu6050(&mpu);
}
void IMUreadSensors(IMU* imu)
{
	if(readMpu6050(&mpu)==TRUE)
	{
		imu->accel_x=mpu.rawValues.x_accel;
		imu->accel_y=mpu.rawValues.y_accel;
		imu->accel_z=mpu.rawValues.z_accel;
		imu->gyro_x=mpu.rawValues.x_gyro;
		imu->gyro_y=mpu.rawValues.y_gyro;
		imu->gyro_z=mpu.rawValues.z_gyro;
	}
}





