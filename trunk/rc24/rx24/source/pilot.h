typedef struct
{
	routedObject ro; //first item so struct can be cast to a routedObject
	bool enabled;
	int script[1024];
	int stack[256];
	int vars[40];//general purpose variables accessible by script
	// and maintained between frames
} Pilot;

void createPilot(Pilot* pilot);
void pilotDoMix(Pilot* pilot,Rx* rx,IMU* imu);
