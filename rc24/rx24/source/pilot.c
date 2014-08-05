#include <jendefs.h>
#include <string.h>
#include <AppHardwareApi.h>
#include "routedMessage.h"
#include "commonCommands.h"
#include "routedObject.h"
#include "rxmain.h"
#include "ffloat.h"
#include "imu.h"
#include "pilot.h"
#include "blunt.h"
#include "ffloat.h"

// select visible parameters

ccParameter pilotParameters[]=
{
	//name,type,paramPtr,paramOffset,array length,set function,get function
	{ "enabled",CC_BOOL,NULL,offsetof(Pilot,enabled),0,CC_NO_GETTER,CC_NO_SETTER,CC_STORE},
	{ "script",CC_INT32_ARRAY,NULL,offsetof(Pilot,script),1024,CC_NO_GETTER,CC_NO_SETTER,CC_NO_STORE},
	{ "stack",CC_INT32_ARRAY,NULL,offsetof(Pilot,stack),256,CC_NO_GETTER,CC_NO_SETTER,CC_NO_STORE},
	{ "vars", CC_INT32_ARRAY,NULL,offsetof(Pilot,vars),40 ,CC_NO_GETTER,CC_NO_SETTER,CC_NO_STORE},

	{ "roll_pgain",CC_INT32,NULL,offsetof(Pilot,rollPID)+offsetof(PID,pgain),0,CC_NO_GETTER,CC_NO_SETTER,CC_STORE},
	{ "roll_igain",CC_INT32,NULL,offsetof(Pilot,rollPID)+offsetof(PID,igain),0,CC_NO_GETTER,CC_NO_SETTER,CC_STORE},
	{ "roll_ilimit",CC_INT32,NULL,offsetof(Pilot,rollPID)+offsetof(PID,ilimit),0,CC_NO_GETTER,CC_NO_SETTER,CC_STORE},
	{ "roll_dgain",CC_INT32,NULL,offsetof(Pilot,rollPID)+offsetof(PID,dgain),0,CC_NO_GETTER,CC_NO_SETTER,CC_STORE},
	{ "pitch_pgain",CC_INT32,NULL,offsetof(Pilot,pitchPID)+offsetof(PID,pgain),0,CC_NO_GETTER,CC_NO_SETTER,CC_STORE},
	{ "pitch_igain",CC_INT32,NULL,offsetof(Pilot,pitchPID)+offsetof(PID,igain),0,CC_NO_GETTER,CC_NO_SETTER,CC_STORE},
	{ "pitch_ilimit",CC_INT32,NULL,offsetof(Pilot,pitchPID)+offsetof(PID,ilimit),0,CC_NO_GETTER,CC_NO_SETTER,CC_STORE},
	{ "pitch_dgain",CC_INT32,NULL,offsetof(Pilot,pitchPID)+offsetof(PID,dgain),0,CC_NO_GETTER,CC_NO_SETTER,CC_STORE},
	{ "yaw_pgain",CC_INT32,NULL,offsetof(Pilot,yawPID)+offsetof(PID,pgain),0,CC_NO_GETTER,CC_NO_SETTER,CC_STORE},
	{ "yaw_igain",CC_INT32,NULL,offsetof(Pilot,yawPID)+offsetof(PID,igain),0,CC_NO_GETTER,CC_NO_SETTER,CC_STORE},
	{ "yaw_ilimit",CC_INT32,NULL,offsetof(Pilot,yawPID)+offsetof(PID,ilimit),0,CC_NO_GETTER,CC_NO_SETTER,CC_STORE},
	{ "yaw_dgain",CC_INT32,NULL,offsetof(Pilot,yawPID)+offsetof(PID,dgain),0,CC_NO_GETTER,CC_NO_SETTER,CC_STORE},
	{ "z_pgain",CC_INT32,NULL,offsetof(Pilot,zPID)+offsetof(PID,pgain),0,CC_NO_GETTER,CC_NO_SETTER,CC_STORE},
	{ "z_igain",CC_INT32,NULL,offsetof(Pilot,zPID)+offsetof(PID,igain),0,CC_NO_GETTER,CC_NO_SETTER,CC_STORE},
	{ "z_ilimit",CC_INT32,NULL,offsetof(Pilot,zPID)+offsetof(PID,ilimit),0,CC_NO_GETTER,CC_NO_SETTER,CC_STORE},
	{ "z_dgain",CC_INT32,NULL,offsetof(Pilot,zPID)+offsetof(PID,dgain),0,CC_NO_GETTER,CC_NO_SETTER,CC_STORE},

};
ccParameterList pilotParameterList=
{
	pilotParameters,
	sizeof(pilotParameters)/sizeof(pilotParameters[0])
};

void createPilot(Pilot* pilot)
{
	pilot->ro.parameters=pilotParameterList;
	pilot->ro.messageHandler=&routedObjectHandleMessage;
}

void defaultMix(Pilot* pilot,Rx* rx,IMU* imu)
{
	int i;
	//copy demands across unchanged
	for(i=0;i<20;i++)
	{
		rx->rxMixedDemands[i]=rx->rxDemands[i];
	}
}

inline int limit(a,min,max)
{
	if(a>max)return max;
	else if(a<min)return min;
	return a;
}
int doPID(PID* pid,int demand,int actual,int dt)
{
	int error=demand-actual;
	pid->ierror=limit(pid->ierror+error*dt,-pid->ilimit,pid->ilimit);

	int ret= (error*pid->pgain>>10) +
		(pid->ierror*pid->igain>>16) +
		((error-pid->lasterror)*pid->dgain/dt>>4);
	pid->lasterror=error;
	return ret;
}
void initPID(PID* pid)
{
	pid->lasterror=0;
	pid->ierror=0;

}

#define MOTOR1 0
#define MOTOR2 1
#define MOTOR3 2
#define MOTOR4 5


void quadcopterMix(Pilot* pilot,Rx* rx,IMU* imu)
{
	//TODO use quaternions from imu directly rather than roll pitch and yaw angles that loose meaning at extreme angles
	static int armed=0;
	static int altHold=0;
	static int altDemand=0;

	if(rx->rxDemands[2]<400 && rx->rxDemands[3]<400)
	{
		armed=0;
	}
	else if(rx->rxDemands[2]<300 && rx->rxDemands[3]>4096-300)
	{
		armed=1;
		startZeroing(imu);
	}
	int throttle=(int)rx->rxDemands[2]-2048; //for center sprung throttle
	throttle*=2;
	if(armed==0 )
	{
		rx->rxMixedDemands[MOTOR1]=0;
		rx->rxMixedDemands[MOTOR2]=0;
		rx->rxMixedDemands[MOTOR3]=0;
		rx->rxMixedDemands[MOTOR4]=0;
		return;
	}
	//failsafe
	if(rx->missedFrames>500) //10seconds
	{
		//todo try auto land using altimeter
		rx->rxMixedDemands[MOTOR1]=0;
		rx->rxMixedDemands[MOTOR2]=0;
		rx->rxMixedDemands[MOTOR3]=0;
		rx->rxMixedDemands[MOTOR4]=0;
		armed=0;
		return;
	}


	IMUreadSensors(imu);


	if(imu->zeroing==0)
	{
		if((int)rx->rxDemands[3]>3000)
		{
			if(altHold==0)
			{
				altHold=1;
				altDemand=lshift(imu->z,7);
				initPID(&pilot->zPID);
				//assume current throttle is about right
				if(pilot->zPID.igain!=0)pilot->zPID.ierror= throttle/pilot->zPID.igain;
			}
			throttle=doPID(&pilot->zPID,altDemand,lshift(imu->z,7),imu->dt>>12);

		}
		else
		{
			altHold=0;
		}

		//int rollOp=doPID(&pilot->rollPID,0,imu->roll,imu->dt>>12) + (int)rx->rxDemands[0]-2048;
		//int pitchOp=doPID(&pilot->pitchPID,0,imu->pitch,imu->dt>>12) + (int)rx->rxDemands[1]-2048;

		int rollOp=doPID(&pilot->rollPID,((int)rx->rxDemands[0]-2048)<<1,imu->roll,imu->dt>>12);
		int pitchOp=doPID(&pilot->pitchPID,((int)rx->rxDemands[1]-2048)<<1,imu->pitch,imu->dt>>12);



		int yawOp=doPID(&pilot->yawPID,0,imu->yaw,imu->dt>>12) + (int)rx->rxDemands[3]-2048;


		int maxpower=3072;
		rx->rxMixedDemands[MOTOR1]=limit(throttle+rollOp-pitchOp-yawOp ,0,maxpower);
		rx->rxMixedDemands[MOTOR2]=limit(throttle-rollOp-pitchOp+yawOp ,0,maxpower);
		rx->rxMixedDemands[MOTOR3]=limit(throttle-rollOp+pitchOp-yawOp ,0,maxpower);
		rx->rxMixedDemands[MOTOR4]=limit(throttle+rollOp+pitchOp+yawOp ,0,maxpower);

		if(throttle<400)
		{
			rx->rxMixedDemands[MOTOR1]=0;
			rx->rxMixedDemands[MOTOR2]=0;
			rx->rxMixedDemands[MOTOR3]=0;
			rx->rxMixedDemands[MOTOR4]=0;
		}
	}
	else
	{
		initPID(&pilot->rollPID);
		initPID(&pilot->pitchPID);
		initPID(&pilot->yawPID);
		initPID(&pilot->zPID);
	}

	static int go=0;



	go++;
	if(go>10)
	{
		go=0;

		int ttt[6];

		ttt[0]=(int)lshift(imu->z,12	);
		ttt[1]=throttle;
		ttt[2]=0;

		ttt[3]=0;
		ttt[4]=0;
		ttt[5]=0;


		dbgLog(&ttt[0],24);
	}
}
//called every frame  before servo output
//by default it calls the loaded script
//but could equally run custom c code

void pilotDoMix(Pilot* pilot,Rx* rx,IMU* imu)
{

	if(!pilot->enabled)return defaultMix(pilot,rx,imu);
//	return quadcopterMix(pilot,rx,imu);


	//objects passed to script
	pilot->stack[0]=0;//null pointer for pc object as not handled yet
	pilot->stack[1]=0;//null pointer for tx object as not handled yet
	pilot->stack[2]=(int)rx;
	pilot->stack[3]=(int)pilot;
	pilot->stack[4]=(int)imu;

	run(pilot->script,pilot->stack,5);

}

