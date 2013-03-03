typedef struct
{
	routedObject ro; //first item so struct can be cast to a routedObject
	bool enabled;
	int pgain_x;
	int igain_x;
	int dgain_x;
	int script[1024];
	int stack[256];

} Pilot;

void createPilot(Pilot* pilot);
void pilotDoMix(Pilot* pilot,Rx* rx,IMU* imu);
