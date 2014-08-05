#include <jendefs.h>
#include <stdint.h>
#include <string.h>
#include "routedMessage.h"
#include "commonCommands.h"
#include "routedObject.h"
#include "ffloat.h"
#include "imu.h"
#include "mpu6050.h"
#include <math.h>

// select visible parameters

int aaa=23;
ccParameter imuParameters[]=
{
	//name,type,paramPtr,paramOffset,array length,set function,get function
	{ "enabled",CC_BOOL,NULL,offsetof(IMU,enabled),0,CC_NO_GETTER,CC_NO_SETTER,CC_STORE},
	{ "accel x",CC_INT32,NULL,offsetof(IMU,accel_x),0,CC_NO_GETTER,CC_NO_SETTER,CC_NO_STORE},
	{ "accel y",CC_INT32,NULL,offsetof(IMU,accel_y),0,CC_NO_GETTER,CC_NO_SETTER,CC_NO_STORE},
	{ "accel z",CC_INT32,NULL,offsetof(IMU,accel_z),0,CC_NO_GETTER,CC_NO_SETTER,CC_NO_STORE},
	{ "gyro x",CC_INT32,NULL,offsetof(IMU,gyro_x),0,CC_NO_GETTER,CC_NO_SETTER,CC_NO_STORE},
	{ "gyro y",CC_INT32,NULL,offsetof(IMU,gyro_y),0,CC_NO_GETTER,CC_NO_SETTER,CC_NO_STORE},
	{ "gyro z",CC_INT32,NULL,offsetof(IMU,gyro_z),0,CC_NO_GETTER,CC_NO_SETTER,CC_NO_STORE},
	{ "roll",CC_INT32,NULL,offsetof(IMU,roll),0,CC_NO_GETTER,CC_NO_SETTER,CC_NO_STORE},
	{ "pitch",CC_INT32,NULL,offsetof(IMU,pitch),0,CC_NO_GETTER,CC_NO_SETTER,CC_NO_STORE},
	{ "yaw",CC_INT32,NULL,offsetof(IMU,yaw),0,CC_NO_GETTER,CC_NO_SETTER,CC_NO_STORE},
	{ "timer",CC_INT32,NULL,offsetof(IMU,timer),0,CC_NO_GETTER,CC_NO_SETTER,CC_NO_STORE},
	{ "read sensors",CC_VOID_FUNCTION,NULL,0,0,CC_NO_GETTER,(VOID_FN)IMUreadSensors},
};
ccParameterList imuParameterList=
{
	imuParameters,
	sizeof(imuParameters)/sizeof(imuParameters[0])
};
mpu6050 mpu;
MS5611 altimeter;

void createIMU(IMU* imu)
{
	imu->ro.parameters=imuParameterList;
	imu->ro.messageHandler=&routedObjectHandleMessage;
	imu->ro.version=2;
	mpu.I2Caddress=0x68;
	mpu.compassI2Caddress=0x0c;
	altimeter.addr=0x77;

}
void startIMU(IMU* imu)
{
	if(imu->enabled)
	{
		if(initMpu6050(&mpu)==TRUE)
		{

			imu->roll=0;
			imu->pitch=0;
			imu->yaw=0;

			dbgPrintf("mpu %i ",mpu.whoami);
			readMpu6050(&mpu);
			if(readMpu6050(&mpu)==TRUE)
			{
				dbgPrintf("mpu r %i %i %i %i",mpu.rawValues.x_accel,mpu.rawValues.x_gyro,mpu.rawValues.temperature,mpu.rawValues.y_mag);


			}
		}
		else dbgPrintf("mpu init failed ");
		if(initMS5611(&altimeter)==TRUE)dbgPrintf("alt ok ");
		else dbgPrintf("altimeter init failed ");
	}
}

#define PI_FP12    12869
#define PIBY2_FP12  6434

#define PI_FP19    1647099
#define PIBY2_FP19  823550

#define ONE_FP12 4096
#define ONE_FP15 32786

//http://stackoverflow.com/questions/1100090/looking-for-an-efficient-integer-square-root-algorithm-for-arm-thumb2
//http://ww1.microchip.com/downloads/en/AppNotes/91040a.pdf
int32_t sqrt_fp(uint32_t x)
{
    uint16_t res=0;
    uint16_t add= 0x8000;
    int i;
    for(i=0;i<16;i++)
    {
        uint16_t temp=res | add;
        uint32_t g2=temp*temp;
        if (x>=g2)
        {
            res=temp;
        }
        add>>=1;
    }
    int32_t sres= res;
	return sres;
}

int atan2_fp12( int y, int x )
{
	int atan;
	if ( abs( x ) >abs( y ) )
	{
		if ( x == 0 )
		{
			if ( y > 0 ) return PIBY2_FP12;
			if ( y == 0 ) return 0;
			return -PIBY2_FP12;
		}

		int z = (y<<12)/x;

		atan = (z<<12)/(ONE_FP12 + ((((1147*z)>>12)*z)>>12));
		if ( x < 0 )
		{
			if ( y < 0 ) return atan - PI_FP12;
			return atan + PI_FP12;
		}
	}
	else
	{
		if ( y == 0 )
		{
			if ( x > 0 ) return PIBY2_FP12;
			if ( x == 0 ) return 0;
			return -PIBY2_FP12;
		}
		int z = (x<<12)/y;
		atan = PIBY2_FP12-(z<<12)/(ONE_FP12 + ((((1147*z)>>12)*z)>>12));
		if ( y < 0 )
		{
			return atan - PI_FP12;
		}
	}
	return atan;
}

int asin_fp12(int x)
{
	return atan2_fp12(x, sqrt_fp((ONE_FP12 + x )* (ONE_FP12 - x)));
}
int acos_fp12(int x)
{
	return atan2_fp12(sqrt_fp((ONE_FP12 + x )* (ONE_FP12 - x)),x);
}

void rotate_fp12( vector3_fp12 *v,vector3_fp12 *angles)
{
	vector3_fp12 t=*v;

	v->x +=  ((angles->y * t.z)>>19)  - ((angles->z * t.y)>>19);
	v->y +=  -((angles->x * t.z)>>19)  + ((angles->z * t.x)>>19);
	v->z +=  ((angles->x * t.y)>>19)  - ((angles->y * t.x)>>19);
}
void startZeroing(IMU* imu)
{
	imu->zeroing=512;
	imu->gyro_x_offset=0;
	imu->gyro_y_offset=0;
	imu->gyro_z_offset=0;

	imu->mag_x_offset=-10841;
	imu->mag_y_offset=4292;
	imu->mag_z_offset=-37730;
	imu->gravity=0;
	imu->vx=0;
	imu->x=0;
	imu->vy=0;
	imu->y=0;
	imu->vz=0;
	imu->z=0;
	imu->altz=0;


}
void initIMUfq(IMU* imu,vector3f* acc,vector3f* mag)
{
	imu->attitude= q_FromGM(acc,mag);
	imu->attitudeLPF=imu->attitude;
	imu->gmLPF=imu->attitude;
	imu->vx=0;
	imu->x=0;
	imu->vy=0;
	imu->y=0;
	imu->vz=0;
	imu->z=0;
	imu->altz=0;
	imu->accel_x_offset=13;
	imu->accel_y_offset=-39;
	imu->accel_z_offset=251;

	imu->accel_xLPF=acc->x;
	imu->accel_yLPF=acc->y;
	imu->accel_zLPF=acc->z;


}

void stepIMUfq(IMU* imu,vector3f* gyro,vector3f* acc,vector3f* mag,float dt)
{
	static int tt=0;

	tt++;

	float t=0.001;


	 float scaling =(float)imu->pressure_zero / (float)altimeter.pressure;
	 float temp  = ffadd(((float)imu->temp_zero)/100 , 273.15f);
	 float altz = ffmul(ffmul(fflog(scaling) , temp) , 29.271267f);


	 imu->altz=altz;//rshift(ffadd(ffmul(imu->altz,127),altz),7);

	 imu->z=imu->altz;

//	 float altv = ffsub(imu->altz,oldHeight)/dt;


	float accScale=ffmul(dt,8*9.81/32768);



	float lastv=imu->vz;

//	imu->vz=ffadd(imu->vz,ffmul(ffsub(worldacc.z,imu->gravity),accScale));

	float lastz=imu->z;
	float avvz=ffadd(rshift(imu->vz,1),rshift(lastz,1));

	imu->z = ffadd(ffmul(ffadd(imu->z,ffmul(avvz,dt)),0.998),ffmul(imu->altz,0.002)) ;
	float z = imu->z;//ffadd(imu->z,ffmul(imu->vz,dt));

	//apply effect of filter to vz
//	imu->vz = ffsub(imu->z,lastz)/dt;
	float vz = ffsub(z,lastz)/dt;
//	if(vz< -1000)dbgPrintf("\r\n%i %i %i",(int)ffmul(imu->vz,100),(int)ffmul(z,100),(int)ffmul(lastz,100));

	imu->vz =vz;



	vector3f accLPF;
	accLPF.x=imu->accel_xLPF;
	accLPF.y=imu->accel_yLPF;
	accLPF.z=imu->accel_zLPF;


	float scale=ffmul(M_PI/180.0f*2000.0f/32768.0f/512,dt);

	q_rotate(&imu->attitude,ffmul((float)gyro->x,scale),ffmul((float)gyro->y,scale),ffmul((float)gyro->z,scale));

	float totalacc=ffadd(ffadd(ffmul(accLPF.x,accLPF.x),ffmul(accLPF.y,accLPF.y)),ffmul(accLPF.z,accLPF.z));
	totalacc=ffmul(totalacc,8.0f*9.81f/32768.0f);

	if(totalacc<41000 && totalacc>39000)
	{
		quaternionf gm=q_FromGM(&accLPF,mag);
		imu->gmLPF=gm;
		q_lowpass(&imu->attitude,&gm,t);
	}


//	q_mul(&error,&imu->attitude,&temp);
//	imu->attitude=temp;
/*
	quaternion_mul(&error,&imu->attitudeLPF,&temp);
	imu->attitude=temp;
*/

	//error has been corrected so set gyro estimate to accel/mag filtered value
//	imu->attitudeLPF=imu->gmLPF;

//	q_lowpass(&imu->attitudeLPF,&imu->attitude,t);

//	quaternionf gm= q_FromGM(acc,mag);
//	q_lowpass(&imu->gmLPF,&gm,t);


//	(imu->attitudeLPF)=*&imu->attitude;
//	(imu->gmLPF)=gm;


}



void IMUreadSensors(IMU* imu)
{
	static int alt=0;
	int st=getSysClock();
	imu->dt=st-imu->lastClock;
	imu->lastClock=st;

	int gyroRange=2000;//degrees/sec
	if(readMpu6050(&mpu)==TRUE)
	{

		if(alt==6)
		{
			if(readMS5611(&altimeter)==TRUE)
			{

			}
			alt=0;
		}
		else alt++;

		imu->accel_x=mpu.rawValues.x_accel-imu->accel_x_offset;
		imu->accel_y=mpu.rawValues.y_accel-imu->accel_y_offset;
		imu->accel_z=mpu.rawValues.z_accel-imu->accel_z_offset;

		imu->accel_xLPF=(imu->accel_xLPF*15+imu->accel_x)>>4;
		imu->accel_yLPF=(imu->accel_yLPF*15+imu->accel_y)>>4;
		imu->accel_zLPF=(imu->accel_zLPF*15+imu->accel_z)>>4;

		if(imu->zeroing>0)
		{
			imu->gyro_x_offset+=mpu.rawValues.x_gyro;
			imu->gyro_y_offset+=mpu.rawValues.y_gyro;
			imu->gyro_z_offset+=mpu.rawValues.z_gyro;

		//	imu->gravity+=sqrt_fp(imu->accel_x*imu->accel_x+imu->accel_y*imu->accel_y+imu->accel_z*imu->accel_z);
			imu->gravity=4096;

			imu->zeroing--;
			if(imu->zeroing==0)
			{
				imu->gyro_x_offset=imu->gyro_x_offset;//>>9;
				imu->gyro_y_offset=imu->gyro_y_offset;//>>9;
				imu->gyro_z_offset=imu->gyro_z_offset;//>>9;


				vector3f acc,mag,gyro;
				acc.x=imu->accel_x;
				acc.y=imu->accel_y;
				acc.z=imu->accel_z;

			//	imu->gravity=imu->gravity>>9;

				mag.x=-(mpu.rawValues.y_mag-imu->mag_y_offset);
				mag.y=-(mpu.rawValues.x_mag-imu->mag_x_offset);
				mag.z=(mpu.rawValues.z_mag-imu->mag_z_offset);

				initIMUfq(imu,&acc,&mag);

				imu->pressure_zero=altimeter.pressure;
				imu->temp_zero=altimeter.temp;


			}
		}
		else
		{
			vector3f acc,mag,gyro;
			acc.x=imu->accel_x;
			acc.y=imu->accel_y;
			acc.z=imu->accel_z;

			mag.x=-(mpu.rawValues.y_mag-imu->mag_y_offset);
			mag.y=-(mpu.rawValues.x_mag-imu->mag_x_offset);
			mag.z=(mpu.rawValues.z_mag-imu->mag_z_offset);


			gyro.x=-((mpu.rawValues.x_gyro<<9)-imu->gyro_x_offset);
			gyro.y=-((mpu.rawValues.y_gyro<<9)-imu->gyro_y_offset);
			gyro.z=-((mpu.rawValues.z_gyro<<9)-imu->gyro_z_offset);


			stepIMUfq(imu,&gyro,&acc,&mag,ffmul(imu->dt,1.0f/16000000.0f));


			quaternioni q=q_toInt(&imu->attitude,12);

			q.x=-q.x;
			q.y=-q.y;
			q.z=-q.z;

			imu->roll=asin_fp12(2*((q.w*q.x + q.y*q.z)>>12) )*18000/PI_FP12;
			imu->pitch=-asin_fp12(2*((q.x*q.z - q.w*q.y)>>12)  )*18000/PI_FP12;
			imu->yaw=atan2_fp12( 2*((q.x*q.y + q.w*q.z)>>12)  ,  q.w*q.w + q.x*q.x - q.y*q.y - q.z*q.z >>12  )*18000/PI_FP12;

		}
	}
	imu->timer=getSysClock()-st;


}
