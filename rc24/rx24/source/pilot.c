#include <jendefs.h>
#include <string.h>
#include <AppHardwareApi.h>
#include "routedMessage.h"
#include "commonCommands.h"
#include "routedObject.h"
#include "rxmain.h"
#include "imu.h"
#include "pilot.h"
#include "fixed.hpp"
#include "blunt.h"

// select visible parameters

ccParameter pilotParameters[]=
{
	//name,type,paramPtr,paramOffset,array length,set function,get function
	{ "enabled",CC_BOOL,NULL,offsetof(Pilot,enabled),0,CC_NO_GETTER,CC_NO_SETTER},
	{ "pgain_x",CC_INT32,NULL,offsetof(Pilot,pgain_x),0,CC_NO_GETTER,CC_NO_SETTER},
	{ "script",CC_INT32_ARRAY,NULL,offsetof(Pilot,script),1024,CC_NO_GETTER,CC_NO_SETTER},
	{ "stack",CC_INT32_ARRAY,NULL,offsetof(Pilot,stack),256,CC_NO_GETTER,CC_NO_SETTER},

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
//called every frame 2ms before servo output
//by default it calls the loaded script
//but could equally run custom c code

void pilotDoMix(Pilot* pilot,Rx* rx,IMU* imu)
{

	if(!pilot->enabled)return defaultMix(pilot,rx,imu);
	pilot->enabled=FALSE;

	//objects passed to script
	pilot->stack[0]=0;//null pointer for tx object as not handled yet
	pilot->stack[1]=(int)rx;
	pilot->stack[2]=(int)pilot;
	pilot->stack[3]=(int)imu;

	run(pilot->script,pilot->stack,4);


	return;

	IMUreadSensors(imu);
	//try pitch stablization on elevons on channels l&r
	int l=2;
	int r=3;
	int i;
	for(i=0;i<20;i++)
	{
		if(i!=l && i!=r)rx->rxMixedDemands[i]=rx->rxDemands[i];
	}
	//demanded pitch rate is mean of values
	int demandedPitchRate=rx->rxDemands[l]+rx->rxDemands[r]-4096;
	int actualPitchRate=imu->gyro_x;
	int correction=(actualPitchRate-demandedPitchRate)*pilot->pgain_x/10000;



	int ldemand=rx->rxDemands[l]+correction;
	int rdemand=rx->rxDemands[r]+correction;

	if(rx->rxMixedDemands[r]>=4095)rx->rxMixedDemands[r]=4095;
	if(rx->rxMixedDemands[r]<0)rx->rxMixedDemands[r]=0;
	if(rx->rxMixedDemands[l]>=4095)rx->rxMixedDemands[l]=4095;
	if(rx->rxMixedDemands[l]<0)rx->rxMixedDemands[l]=0;


	//TODO cap values and allow for unreceived channels

}
