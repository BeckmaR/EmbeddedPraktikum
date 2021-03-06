#include <string>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include "LabCarClient.h"

class driveSim {
public:
	driveSim(std::string address);
	~driveSim();
	void accelerate();
	void brake();
	void steerLeft();
	void steerRight();
private:
	float acceleration;
	float brakeforce;
	float steeringAngle;

	float getAccel();
	float getBrakeforce();
	float getSteeringAngle();

	void setAccel(float a);
	void setBrakeforce(float b);
	void setSteeringAngle(float a);

	std::vector<std::string> sendCmd(std::string cmd);

	LabCarClient client;
};
