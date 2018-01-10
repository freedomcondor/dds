#include<math.h>
#include"function_opengl.h"
#include"packet_control_interface.h"

#include <iostream>

#define pi 3.1415926

#define Max_plot 10000000

///////////////////  function definations /////////////////
//
//
double buffer_draw2 = 0;
double plot_y_max = 30;
double plot_x_max = 30;
double datalog[Max_plot];

///////////////////  dds  /////////////////////////
CPacketControlInterface *ddsInterface, *pmInterface;
unsigned int targetLeftSpeed, targetRightSpeed;
unsigned int currentLeftSpeed, currentRightSpeed;
unsigned int fbLeftSpeed, fbRightSpeed;
unsigned int uptime = 0;

///////////////////  functions  ///////////////////////////
int function_exit()
{
	uint8_t pnStopDriveSystemData[] = {0, 0, 0, 0};
	ddsInterface->SendPacket(
			CPacketControlInterface::CPacket::EType::SET_DDS_SPEED,
			pnStopDriveSystemData,
			sizeof(pnStopDriveSystemData));
#ifdef windows
	exit(0);
#else
	exit(0);
#endif
}
int function_init(int SystemWeight, int SystemHeight)
{
	ddsInterface = 
		      new CPacketControlInterface("dds", "/dev/ttyUSB1", 57600);

	pmInterface = 
		      new CPacketControlInterface("pm", "/dev/ttyUSB0", 57600);

	printf("---Establish Connection with MCUs---------------------\n");

	if (!ddsInterface->Open())
	{	printf("USB1 not open\n");	return -1;	}

	if (!pmInterface->Open())
	{	printf("USB0 not open\n");	return -1;	}


	printf("Both interfaces are open\n");

	// get uptime and know which is which
	ddsInterface->SendPacket(CPacketControlInterface::CPacket::EType::GET_UPTIME);
	printf("ID check:\n");
	while(1)
	{
		ddsInterface->ProcessInput();

		if(ddsInterface->GetState() == CPacketControlInterface::EState::RECV_COMMAND)
		{
			const CPacketControlInterface::CPacket& cPacket = ddsInterface->GetPacket();
			if (cPacket.GetType() == CPacketControlInterface::CPacket::EType::GET_UPTIME)
			{
				if(cPacket.GetDataLength() == 4) 
				{
					const uint8_t* punPacketData = cPacket.GetDataPointer();
					uint32_t unUptime = 
						(punPacketData[0] << 24) |
						(punPacketData[1] << 16) |
						(punPacketData[2] << 8)  |
						(punPacketData[3] << 0); 
					printf("dds uptime: %u\n",unUptime);
					if (unUptime != 0)
					{
						// means reverse, exchange pm and dds
						printf("dds get non-0 uptime, it should be pm\n");
						CPacketControlInterface* tempInterface;
						tempInterface = pmInterface;
						pmInterface = ddsInterface;
						ddsInterface = tempInterface;
					}
				} 
				break;
			}
		}
	}

	printf("ID check complete:\n");

	// Read Battery
	pmInterface->SendPacket(CPacketControlInterface::CPacket::EType::GET_BATT_LVL);
	if(pmInterface->WaitForPacket(1000, 5)) 
	//while (1)
	{
		const CPacketControlInterface::CPacket& cPacket = pmInterface->GetPacket();
		if(cPacket.GetType() == CPacketControlInterface::CPacket::EType::GET_BATT_LVL &&
				cPacket.GetDataLength() == 2) 
		{
			std::cout << "System battery: "
		              << 17u * cPacket.GetDataPointer()[0]
					  << "mV" << std::endl;
			std::cout << "Actuator battery: "
					  << 17u * cPacket.GetDataPointer()[1]
					  << "mV" << std::endl;
			//break;
		}
	}
	///*
	else 
	{
		std::cout << "Warning: Could not read the system/actuator battery levels" << std::endl;
	}
	//*/

	// Speed
	printf("---Initialize Actuator---------------------\n");
	enum class EActuatorInputLimit : uint8_t {
		LAUTO = 0, L100 = 1, L150 = 2, L500 = 3, L900 = 4
	};

	//unsigned int unTargetId = -1;
	//unsigned int unControlTick = -1;

	/* Override actuator input limit to 100mA */
	pmInterface->SendPacket(
			CPacketControlInterface::CPacket::EType::SET_ACTUATOR_INPUT_LIMIT_OVERRIDE,
			static_cast<const uint8_t>(EActuatorInputLimit::L100));

	/* Enable the actuator power domain */
	pmInterface->SendPacket(
			CPacketControlInterface::CPacket::EType::SET_ACTUATOR_POWER_ENABLE, 
			true);
				      
	/* Power up the differential drive system */
	ddsInterface->SendPacket(CPacketControlInterface::CPacket::EType::SET_DDS_ENABLE, true);

	/* Initialize the differential drive system */
	uint8_t pnStopDriveSystemData[] = {0, 0, 0, 0};
	ddsInterface->SendPacket(
			CPacketControlInterface::CPacket::EType::SET_DDS_SPEED,
			pnStopDriveSystemData,
			sizeof(pnStopDriveSystemData));
	printf("initialize complete\n");

	currentRightSpeed = targetRightSpeed;
	currentLeftSpeed = targetLeftSpeed;

	return 0;
}

int function_draw()
{
	return 0;
}

int function_draw2()
{
	int drawWindow = 100;

	int drawStart = 0;
	if (uptime < plot_x_max)
		drawStart = 0;
	else
		drawStart = uptime - drawWindow;

	for (int i = drawStart; i <= uptime; i++)
	{
		if (uptime < plot_x_max)
		{
		glBegin(GL_LINES);
			glVertex3f( (i-1)/plot_x_max,	datalog[i-1]/plot_y_max,0.0f);
			glVertex3f(  i/plot_x_max,		datalog[i] / plot_y_max,0.0f);
		glEnd();
		}
		else
		{
		glBegin(GL_LINES);
			glVertex3f( 1.0*(i-1-drawStart)/drawWindow,	datalog[i-1]/plot_y_max,0.0f);
			glVertex3f( 1.0*(i - drawStart)/drawWindow,	datalog[i] / plot_y_max,0.0f);
		glEnd();
		}
	}
		
	return 0;
}

int function_step()
{
	if (uptime < Max_plot)
		uptime++;

	if (targetLeftSpeed != currentLeftSpeed)
	{
		currentLeftSpeed = targetLeftSpeed;
		//uint8_t temp = reinterpret_cast<uint8_t>(currentLeftSpeed & 255);
		uint8_t temp = currentLeftSpeed & 255;

		uint8_t pnStopDriveSystemData[] = {0, temp, 0, 0};
		ddsInterface->SendPacket(
			CPacketControlInterface::CPacket::EType::SET_DDS_SPEED,
			pnStopDriveSystemData,
			sizeof(pnStopDriveSystemData));
		printf("currentspeed : %u\n",temp);
	}
	printf("currentspeed : %u\n",currentLeftSpeed);

	printf("Asking speed");
	ddsInterface->SendPacket(CPacketControlInterface::CPacket::EType::GET_DDS_SPEED);
	while (1)
	{
		ddsInterface->ProcessInput();
		if(ddsInterface->GetState() == CPacketControlInterface::EState::RECV_COMMAND)
		{
			printf("received command\n");
			//std::cout << "received command" << std::endl;
			const CPacketControlInterface::CPacket& cPacket = ddsInterface->GetPacket();
			if (cPacket.GetType() == CPacketControlInterface::CPacket::EType::GET_UPTIME)
			{
				printf("packettype: 0x%x\n",(int)cPacket.GetType());
				if(cPacket.GetDataLength() == 4) 
				{
					const uint8_t* punPacketData = cPacket.GetDataPointer();
					uint32_t unUptime = 
						(punPacketData[0] << 24) |
						(punPacketData[1] << 16) |
						(punPacketData[2] << 8)  |
						(punPacketData[3] << 0); 
					printf("uptime: %u\n",unUptime);
				} 
				//break;
			}
			if (cPacket.GetType() == CPacketControlInterface::CPacket::EType::GET_DDS_SPEED)
			{
				printf("packettype: 0x%x\n",(int)cPacket.GetType());
				if(cPacket.GetDataLength() == 4) 
				{
					const uint8_t* punPacketData = cPacket.GetDataPointer();
					printf("speed: %u\n",punPacketData[0]);
					printf("speed: %u\n",punPacketData[1]);
					printf("speed: %u\n",punPacketData[2]);
					printf("speed: %u\n",punPacketData[3]);
					fbLeftSpeed = punPacketData[1];
				} 

				datalog[uptime] = fbLeftSpeed;
				break;
			}
		}
	}// end of while 1

	return 0;
}



////////////////////////////// drawings  /////////////////////////
int drawSphere(double x, double y, double z, double r)
{
	glTranslatef(x,y,z);
	glutSolidSphere(r,10,10);
	glTranslatef(-x,-y,-z);

	return 0;
}

int drawCube(double x, double y, double z, double half)
{
	glTranslatef(x,y,z);
	glutSolidCube(half);
	glTranslatef(-x,-y,-z);

	return 0;
}

int drawCylinder(	double base, double top, double height,
				double lx,	double ly, double lz,
				double ex,	double ey, double ez
			)
{
	double xaxis,yaxis,zaxis,angleaxis;
	double xbase,ybase,zbase;
	double e;
	int rotateflag = 1;
	GLUquadricObj *quadratic;
	double error = 0.001;

	quadratic = gluNewQuadric();

	//printf("l: %lf %lf %lf\n",lx,ly,lz);
	//printf("e: %lf %lf %lf\n",ex,ey,ez);

	if (((ex-0)*(ex-0)<error) && ((ey-0)*(ey-0)<error) && ((ez-1)*(ez-1)<error))
		rotateflag = 0;

	if (rotateflag == 1)
	{
		e = sqrt(ex * ex + ey * ey + ez * ez);
		if (e == 0) return -1;

		xbase = 0; ybase = 0; zbase = 1;
		xaxis = ybase * ez - zbase * ey;
		yaxis = zbase * ex - xbase * ez;
		zaxis = xbase * ey - ybase * ex;
		//angleaxis = acos((xbase*ex+ybase*ey+zbase*ez)/e) + pi;
		angleaxis = acos((xbase*ex+ybase*ey+zbase*ez)/e);

		//printf("%lf %lf %lf %lf\n",angleaxis,xaxis,yaxis,zaxis);

	}

	glTranslatef(lx,ly,lz);

	if (rotateflag == 1)
		glRotatef(angleaxis*180/pi,xaxis,yaxis,zaxis);

	gluCylinder(quadratic,base,top,height,32,32);//»­Ô²Öù	base top height
	if (rotateflag == 1)
		glRotatef(-angleaxis*180/pi,xaxis,yaxis,zaxis);

	glTranslatef(-lx,-ly,-lz);
	return 0;
}
