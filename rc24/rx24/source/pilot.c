#include <jendefs.h>
#include <string.h>
#include <AppHardwareApi.h>
#include "routedMessage.h"
#include "commonCommands.h"
#include "routedObject.h"
#include "rxmain.h"
#include "imu.h"
#include "pilot.h"
#include "blunt.h"

// select visible parameters

ccParameter pilotParameters[]=
{
	//name,type,paramPtr,paramOffset,array length,set function,get function
	{ "enabled",CC_BOOL,NULL,offsetof(Pilot,enabled),0,CC_NO_GETTER,CC_NO_SETTER},
	{ "script",CC_INT32_ARRAY,NULL,offsetof(Pilot,script),1024,CC_NO_GETTER,CC_NO_SETTER},
	{ "stack",CC_INT32_ARRAY,NULL,offsetof(Pilot,stack),256,CC_NO_GETTER,CC_NO_SETTER},
	{ "vars", CC_INT32_ARRAY,NULL,offsetof(Pilot,vars),40 ,CC_NO_GETTER,CC_NO_SETTER}

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
//	pilot->enabled=FALSE;

	//objects passed to script
	pilot->stack[0]=0;//null pointer for pc object as not handled yet
	pilot->stack[1]=0;//null pointer for tx object as not handled yet
	pilot->stack[2]=(int)rx;
	pilot->stack[3]=(int)pilot;
	pilot->stack[4]=(int)imu;

	run(pilot->script,pilot->stack,5);

}
