typedef struct
{
	int pgain;
	int igain;
	int ilimit;
	int dgain;
	int ierror;
	int lasterror;
} PID;

typedef struct
{
	routedObject ro; //first item so struct can be cast to a routedObject
	bool enabled;
	int script[1024];
	int stack[256];
	int vars[40];//general purpose variables accessible by script
	// and maintained between frames

	PID rollPID;
	PID pitchPID;
	PID yawPID;
	PID zPID;

} Pilot;


void createPilot(Pilot* pilot);
void startIMU(IMU* imu);
void pilotDoMix(Pilot* pilot,Rx* rx,IMU* imu);


