/*******************************************************************************
 * $ Workfile  :  mqttjson.c
 * Project     :  PTS (People Tracking System)
 * Compiler    :  GCC
 * Author      :  Vaibhav Kambli$
 * Date        :  10 March 2017
 * Revision    :  1.0$
 * 
 * paho MQTT:- https://github.com/eclipse/paho.mqtt.c.git 
 * JANSSON:  https://github.com/akheron/jansson.git 
 * 
 * Compile with GCC (linked libraries:- pahoMQTT, Jansson, Math)
 * gcc -o mqttjson mqttjson.c  -l paho-mqtt3c -ljansson -w -lm
 ******************************************************************************/






/*******************************************************************************************************/
																/* Defines */
/*******************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h" //MQTTT client library
#include <jansson.h> //jansson json library
#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include <stdbool.h>
#include <math.h>

#define ADDRESS     "tcp://localhost:1883" //MQTTT host and PORT 
#define CLIENTID    "RaspberryPi" //MQTT Subscriber client 
#define TOPIC       "happy-bubbles/ble/+/ibeacon/+" //MQ HBD topic 
#define QOS         1
#define TIMEOUT     10000L


/*******************************************************************************************************/
															/* Variables Data */
/*******************************************************************************************************/
/* MQTT Client defination */
MQTTClient client;

volatile MQTTClient_deliveryToken deliveredtoken; // MQTTT Token


size_t j; //json
char *text; //json data holder

json_t *root; //json variable for parsing 
json_error_t error; //json error handler

/* Variable holders the number of ibeacons received in one scan cycle */
static int BeaconBufferPtr = 0;


/* 
	Host dectector map 
	get respective host name (string)
*/
char* hostmap[100] ={
	"HBD000",
	"HBD001",
	"HBD002",
	"HBD003",
	"HBD004"
};

/* 
	Host coordinates / Position indoor with respective order of above host map 
 */
int host_coordinates [][2]={
	{0,0},//00
	{1,2},//01
	{2,4},//02
	{3,6},//03
	{4,8},//04
	
};


/*******************************************************************************************************/
															/* Structure Data */
/*******************************************************************************************************/


/* 
	BLEStorageData structure is received data structure from detetors
	extract hostname , ibeacon type , mac address, rssi value, uuid, major and minor numbers, 
	tx power. 
	Create object equals to number of beacons present in system.
*/
typedef struct {
	char host_name[7];
	char beacon_type[8];
	char mac[19];
	int rssi;
	char uuid [33];
	char major [5];
	char minor [5];
	char tx_power [3];
} BLEStorageData;

/* Creatng system to received 10000 beacons as demo */
BLEStorageData bledata[10000];



/* 
	Extract required data from total JSON data, 
	As of now considering hostname, major and minor values as beacon identity, and its received rssi value 
*/
typedef struct {
	char host_name[7];
	int major;
	int minor;
	int rssi;	
} Level1Processing;



/* 
	Detector Table entry
	contains All beacons detected by particular detector and its RSSI
	e.g DetectorTable[1] minor[1]= Beacon 1 value detected by Detector1
*/
typedef struct{
	int minor[1000];
	int rssi[1000];	
}DetectorTable;


/* 
	Take the average of detected RSSI values and create single entry, 
	I will may add kalmann filtering before taking average just to avoid further noise and flutuations
*/
typedef struct{
	int16_t minor[10000];
	float rssi[10000];
	
}DetectorAvgTable;


/* 
	Beacon Table Entries
	contains Beacons Best RSSI values and detectors who detected these values
 */
typedef struct{
	int16_t detectorid[3];
	char* detector[3];
	float rssi[3];
	bool proxyerror[3]
	
}BeaconBestAvg;


/* 
	Trilateration entry table 
	Table maintains beacons entries each row is beacon,
	it contains detector its detected distance coordinates, and flag indicating is 3 best readings presents just to 
	calculate trilateration point 
	values are getting pass to trilaterationcalculation function	which returns the respective trilateration point(x and y coordinates with respective to 
	1st detector )
 */
typedef struct{
	char* detector[3];
	float distance[3];
	int x_coord[3];
	int y_coord[3];
	bool IsTrilat_poss;
}BeaconTriStrcture;




/*******************************************************************************************************/



/*******************************************************************************************************/
												/* Functions */
/*******************************************************************************************************/
/* function to calculate distance from RSSI and TX Power values.
	formula is taken from 
	https://stackoverflow.com/questions/20416218/understanding-ibeacon-distancing 
	need to improve logic in case error is too much 
*/
float calculateDistance(float rssi, int txpower);

// link math library with -lm flag while compile time
float calculateDistance(float rssi, int txpower)
{
	float Txpower;
	Txpower = -59;
	float distance;
	
	if(rssi ==0.0)
	{
		return -1.0;
	}
	float ratio = rssi*1.0/Txpower;
	if(ratio <1.0)
	{
		return(pow(ratio, 10));
	}
	else
	{
		distance = 0.89976 *(pow(ratio,7.7095)) + 0.111; 
		return(distance);
	}
}
/*******************************************************************************************************/



/*******************************************************************************************************/
/* trilateration calculation from three coordinates, find excel based calculation on 
	https://github.com/vaibhavkambli/people-tracking-system/blob/master/algorithms/trilateration/Trilateration%20Calculation.xls* 
 */
void trilaterationcalculation(float* beacon_x, float* beacon_y, int x1,int x2, int x3, int y1, int y2, int y3, float d1, float d2, float d3);


void trilaterationcalculation(float* beacon_x, float* beacon_y, int x1,int x2, int x3, int y1, int y2, int y3, float d1, float d2, float d3) 
{
	/* Translate P1 to origin */
	int X2,Y2,X3,Y3;
	X2= x2-x1;
	Y2= y2-y1;
	X3= x3-x1;
	Y3= y3-y1;
	

	/* Finding calculation values */
	float theta, phi, r1,r2;
	theta =atan2(Y2,X2);
	phi =atan2(Y3,X3);
	r1= sqrt((X2*X2)+(Y2*Y2));
	r2= sqrt((X3*X3)+(Y3*Y3));
	
	/* polar coordinates from the rotating system */
	float pi_minus_theta;
	pi_minus_theta = phi-theta;
	
	
	/* Coordinates of rotated solution */
	float xx, yy, neg_yy;
	xx= (((d1*d1)-(d2*d2)+(r1*r1))/(2*r1));
	yy= sqrt((d1*d1)-(xx*xx));
	neg_yy = -(yy);
	
	/* convert to polar coordinates */
	float rr,tt, tt1, neg_tt;
	rr= sqrt((xx*xx)+(yy*yy));
	tt= atan2(yy,xx);
	neg_tt = -(tt);
	
	/* unrotate back to origin position */
	float TT,neg_TT;
	TT = tt+theta;
	neg_TT = neg_tt +theta;
	
	/* Rectangular coordinates */
	float x_trans, y_trans, neg_x_trans, neg_y_trans;
	x_trans= rr * cos(TT);
	y_trans= rr * sin(TT);
	neg_x_trans= rr * cos(neg_TT);
	neg_y_trans= rr * sin(neg_TT);
	
	/* Untraslate solution */
	float soln_x, soln_y, neg_soln_x ,neg_soln_y;
	soln_x = x_trans+x1; //final soulution1 _x
	soln_y = y_trans+y1; //final soulution1 _y
	neg_soln_x = neg_x_trans+x1; //final soulution2 _x
	neg_soln_y = neg_y_trans+y1; //final soulution2 _y
	
	/* Test for first & second solution */
	float Cal_x1, Cal_x2;
	Cal_x1 = sqrt(((soln_x-x3)*(soln_x-x3)) + ((soln_y-y3)*(soln_y-y3)) );
	Cal_x2 = sqrt(((neg_soln_x-x3)*(neg_soln_x-x3)) + ((neg_soln_y-y3)*(neg_soln_y-y3)) );
	
	/* Re-calculate the distance of solution point with third point to select the best solution 
		majority time solution 1 is prefred solution check the calculated distance is in range of less/greater 3m (assumed 3m now ) 
		return -1 is trilateration failed to get proper point.
	*/
	if((Cal_x1 >=(d3-3)) && (Cal_x1 <=(d3+3)))
	{
		*beacon_x = soln_x;
		*beacon_y = soln_y;
	}
	else 	if((Cal_x2 >=(d3-3)) && (Cal_x2 <=(d3+3)))
	{
		*beacon_x = neg_soln_x;
		*beacon_y = neg_soln_y;
	}
	else
	{
		*beacon_x = -1;
		*beacon_y = -1;
	}
}
/*******************************************************************************************************/



/*******************************************************************************************************/
/* Supporting function to send JSON data in case requiered */
void delivered(void *context, MQTTClient_deliveryToken dt)
{
	//printf("Message with token value %d delivery confirmed\n", dt);
	deliveredtoken = dt;
}
/*******************************************************************************************************/




/*******************************************************************************************************/
/* Get X-coordinates of detector location */
int DetectorXCoordinate(char* Detector);

int DetectorXCoordinate(char* Detector)
{
	int x=0;
	for(x=0;x<5;x++)
	{
		if(Detector == hostmap[x])
		{
			return(host_coordinates[x][0]);
		}
	}
}
/*******************************************************************************************************/



/*******************************************************************************************************/
/* Get X-coordinates of detector location */
int DetectorYCoordinate(char* Detector);

int DetectorYCoordinate(char* Detector)
{
	int x=0;
	for(x=0;x<5;x++)
	{
		if(Detector == hostmap[x])
		{
			return(host_coordinates[x][1]);
		}
	}
}
/*******************************************************************************************************/

 

/*******************************************************************************************************/
/* 
	Timer interrupt handler
	Setting timer to 5 seconds , program start processing data once timer is expired
	disable timer, disable mqtt receiving and process data 
	we may add mapping further 
	enable Timer and MQTT subscribtion at the end once processing is over
*/
/* handler --- handle SIGALRM */

void handler(int signo)
{
	/* looping variables */
	int index=0;
	int subindex=0;
	int intrasubindex=0;
	
	(void) setitimer(ITIMER_REAL, NULL, NULL);	/* turn off timer */
	MQTTClient_unsubscribe(client, TOPIC); /* unsubscribe to MQTT Topic */
	
	/* Timing parameters to reload timer */
	struct itimerval tval;
	timerclear(& tval.it_interval);	/* zero interval means no reset of timer */
	timerclear(& tval.it_value);
	tval.it_value.tv_sec = 5;	/* 10 second timeout */
	tval.it_interval.tv_sec=5;/*reload timing 5sec */
	
	
	//extraction logic here
	//Level-1 processing:- get hostname, major and minor number from total data, convert major, minor to integers  for processing simplicity 
	// creating for 10000 beacons buffer
	Level1Processing level1process[10000];
	
	/* BeaconBufferPtr contains number of entries detected in scan time */
	for(index=0;index<BeaconBufferPtr;index++)
	{
		memcpy(level1process[index].host_name,bledata[index].host_name, 6);
		level1process[index].host_name[6]='\0';
		//printf("hostname[%d]: %s\n",index,level1process[index].host_name);
		
		bledata[index].host_name[0]='\0'; 
		level1process[index].rssi = bledata[index].rssi;
		//printf("RSSI[%d]: %d\n",index,level1process[index].rssi);
		
		level1process[index].major=strtol(bledata[index].major, NULL, 10);
		//printf("major[%d]: %d\n",index,level1process[index].major);
		
		level1process[index].minor=strtol(bledata[index].minor, NULL, 10);
		//printf("minor[%d]: %d\n",index,level1process[index].minor);
	}
	
	
	// Level-2 Processing:sort detected beacons w.r.t detetors i.e. hostname.
	// assuming 100 detectors as of now.	
	
	int detectorentry=0;
	DetectorTable hostdector[100];
	//for detector 0 to detector 100
	//only 5 entries as of now change index carefully(segmentation error may come on wrong entry)
	for(index=0;index<5;index++)//actual 100 detectors
	{
		detectorentry=0;
		// for entry 0 to maximum entry in level1process table.
		for(subindex=0;subindex<BeaconBufferPtr;subindex++)
		{
			if(!strcmp(level1process[subindex].host_name, hostmap[index]))
			{
				hostdector[index].minor[detectorentry]=level1process[subindex].minor;
				//printf("Detector[%d].minor[%d]=%d\n",index,subindex,hostdector[index].minor[detectorentry]);
				hostdector[index].rssi[detectorentry]=level1process[subindex].rssi;
				//printf("Detector[%d].rssi[%d]=%d\n",index,subindex,hostdector[index].rssi[detectorentry]);
				detectorentry++;	
			}
		}
	}
	
	//Level3 Processing: avg values of readings and make table.
	DetectorAvgTable hostavgvalue[100];
	float tempsum=0;
	int tempdivider=0;
	
	for(index=0;index<5;index++) //actual 100detectors
	{
		for(subindex=0;subindex<10;subindex++)//10000 beacons
		{
			tempsum=0;
			tempdivider=0;
			for(intrasubindex=0; intrasubindex<10;intrasubindex++)//1000 //readings buffer
			{
				if(hostdector[index].minor[intrasubindex] == subindex)
				{
					tempdivider++;
					tempsum += hostdector[index].rssi[intrasubindex];
				}
			}
			tempsum=(tempsum/tempdivider);
			hostavgvalue[index].rssi[subindex]=tempsum;
			hostavgvalue[index].minor[subindex]=subindex;
			//printf("Detector[%d].minor[%d] Avg RSSI = %f\n",index,hostavgvalue[index].minor[subindex],hostavgvalue[index].rssi[subindex]);
			
			
		}
	}
	
	//Level4 Processing:- Choose highest 3 values for detector for distance conversion.
	
	BeaconBestAvg beaconbestrssi[10000];
	
	for(index=0;index<10;index++)//10000 beacons
	{
			beaconbestrssi[index].rssi[0]=-500;
			beaconbestrssi[index].rssi[1]=-500;
			beaconbestrssi[index].rssi[2]=-500;
			beaconbestrssi[index].detector[0]=hostmap[0];
			beaconbestrssi[index].detector[1]=hostmap[0];
			beaconbestrssi[index].detector[2]=hostmap[0];
			
			
	}	
	
	
	for(index=0;index<10;index++)//10000 beacons
	{
		for(subindex=0;subindex<5;subindex++)//100
		{
			if(hostavgvalue[subindex].rssi[index] > beaconbestrssi[index].rssi[0])
			{
				beaconbestrssi[index].rssi[0] = hostavgvalue[subindex].rssi[index];
				beaconbestrssi[index].detector[0]= hostmap[subindex];
			}
		}
	}	
	//printf("Larget RSSI %f detector %s\n",beaconbestrssi[4].rssi[0],beaconbestrssi[4].detector[0]);
	
	for(index=0;index<10;index++)//10000 beacons
	{
		for(subindex=0;subindex<5;subindex++)//100
		{
			if((hostavgvalue[subindex].rssi[index] > beaconbestrssi[index].rssi[1]) && (hostavgvalue[subindex].rssi[index] < beaconbestrssi[index].rssi[0]))
			{
				beaconbestrssi[index].rssi[1] = hostavgvalue[subindex].rssi[index];
				beaconbestrssi[index].detector[1]= hostmap[subindex];
			}
		}
	}	
	//printf("Middle RSSI %f detector %s\n",beaconbestrssi[4].rssi[1],beaconbestrssi[4].detector[1]);
	
	for(index=0;index<10;index++)//10000 beacons
	{
		for(subindex=0;subindex<5;subindex++)//100
		{
			if((hostavgvalue[subindex].rssi[index] > beaconbestrssi[index].rssi[2]) && (hostavgvalue[subindex].rssi[index] < beaconbestrssi[index].rssi[1]))
			{
				beaconbestrssi[index].rssi[2] = hostavgvalue[subindex].rssi[index];
				beaconbestrssi[index].detector[2]= hostmap[subindex];
			}
		}
	}	
	//printf("Smallest RSSI %f detector %s\n",beaconbestrssi[4].rssi[2],beaconbestrssi[4].detector[2]);
	
	
	//Get distance estimation from RSSI and get coordinates ready for trilateration algorithm.
	BeaconTriStrcture distancebeacon[10000];
	
	for(index=1;index<10;index++)//10000 beacons
	{
		//printf("\nBeacon[%d] Best Detectors and RSSI\n",index);
		
		distancebeacon[index].IsTrilat_poss =true;
		
		if(beaconbestrssi[index].rssi[0] !=-500.0)
		{
			beaconbestrssi[index].proxyerror[0] =true;
			//printf("(1) Detector= %s\t RSSI= %f flag=%d\n",beaconbestrssi[index].detector[0],beaconbestrssi[index].rssi[0], beaconbestrssi[index].proxyerror[0]);
			
			distancebeacon[index].distance[0] = calculateDistance( beaconbestrssi[index].rssi[0], 0);//tx power 0 passing 
			distancebeacon[index].detector[0] = beaconbestrssi[index].detector[0];
			distancebeacon[index].x_coord[0] = DetectorXCoordinate(	distancebeacon[index].detector[0]);
			distancebeacon[index].y_coord[0] = DetectorYCoordinate(	distancebeacon[index].detector[0]);
			
			printf("Distance Conversion for Beacon[%d] Detector[%s] X-coor[%d] Y-coor[%d]=%f\n ",index,beaconbestrssi[index].detector[0],distancebeacon[index].x_coord[0],distancebeacon[index].y_coord[0],distancebeacon[index].distance[0]);
			
		}
		else
		{
			distancebeacon[index].IsTrilat_poss =false;
			beaconbestrssi[index].proxyerror[0] =false;
			//printf("(1) Detector= %s\t RSSI= %f flag=%d\n",beaconbestrssi[index].detector[0],beaconbestrssi[index].rssi[0], beaconbestrssi[index].proxyerror[0]);
		}
		
		if(beaconbestrssi[index].rssi[1] !=-500.0)
		{
			beaconbestrssi[index].proxyerror[1] =true;
			//printf("(2) Detector= %s\t RSSI= %f flag=%d\n",beaconbestrssi[index].detector[1],beaconbestrssi[index].rssi[1], beaconbestrssi[index].proxyerror[1]);
			
			distancebeacon[index].distance[1] = calculateDistance( beaconbestrssi[index].rssi[1], 0);//tx power 0 passing 
			distancebeacon[index].detector[1] = beaconbestrssi[index].detector[1];
			distancebeacon[index].x_coord[1] = DetectorXCoordinate(	distancebeacon[index].detector[1]);
			distancebeacon[index].y_coord[1] = DetectorYCoordinate(	distancebeacon[index].detector[1]);
			printf("Distance Conversion for Beacon[%d] Detector[%s] X-coor[%d] Y-coor[%d]=%f \n",index,beaconbestrssi[index].detector[1],distancebeacon[index].x_coord[1],distancebeacon[index].y_coord[1],distancebeacon[index].distance[1]);
			
		}
		else
		{
			distancebeacon[index].IsTrilat_poss =false;
			beaconbestrssi[index].proxyerror[1] =false;
			//printf("(1) Detector= %s\t RSSI= %f flag=%d\n",beaconbestrssi[index].detector[0],beaconbestrssi[index].rssi[0], beaconbestrssi[index].proxyerror[0]);
		}
		
		if(beaconbestrssi[index].rssi[2] !=-500.0)
		{
			beaconbestrssi[index].proxyerror[2] =true;
			//printf("(3) Detector= %s\t RSSI= %f flag=%d\n",beaconbestrssi[index].detector[2],beaconbestrssi[index].rssi[2],beaconbestrssi[index].proxyerror[2]);
			
			distancebeacon[index].distance[2] = calculateDistance( beaconbestrssi[index].rssi[2], 0);//tx power 0 passing 
			distancebeacon[index].detector[2] = beaconbestrssi[index].detector[2];
			distancebeacon[index].x_coord[2] = DetectorXCoordinate(	distancebeacon[index].detector[2]);
			distancebeacon[index].y_coord[2] = DetectorYCoordinate(	distancebeacon[index].detector[2]);
			printf("Distance Conversion for Beacon[%d] Detector[%s] X-coor[%d] Y-coor[%d]=%f \n ",index,beaconbestrssi[index].detector[2],distancebeacon[index].x_coord[2],distancebeacon[index].y_coord[2], distancebeacon[index].distance[2]);
			
		}
		else
		{
			distancebeacon[index].IsTrilat_poss =false;
			beaconbestrssi[index].proxyerror[2] =false;
			//printf("(1) Detector= %s\t RSSI= %f flag=%d\n",beaconbestrssi[index].detector[0],beaconbestrssi[index].rssi[0], beaconbestrssi[index].proxyerror[0]);
		}
		
		if(distancebeacon[index].IsTrilat_poss)
		{
			printf("Trilateration Possibilty for Beacon[%d]= %d\n",index,distancebeacon[index].IsTrilat_poss);
		}
		
		
		if(distancebeacon[index].IsTrilat_poss)
		{
			//printf("Calculating Beacon Position with Trilateration\n");
			
			float a,b;
			int x1,x2,x3,y1,y2,y3,d1,d2,d3;
			x1 = distancebeacon[index].x_coord[0];
			x2 = distancebeacon[index].x_coord[1];
			x3 = distancebeacon[index].x_coord[2];
			y1 = distancebeacon[index].y_coord[0];
			y2 = distancebeacon[index].y_coord[1];
			y3 = distancebeacon[index].y_coord[2];
			d1 = distancebeacon[index].distance[0];
			d2 = distancebeacon[index].distance[1];
			d3 = distancebeacon[index].distance[2];
			trilaterationcalculation(&a,&b,x1,x2,x3,y1,y2,y3,d1,d2,d3);
			printf("Beacon[%d]= Position-x[%f] Position-y[%f]\n",index,a,b);
			
		}
	
		

		
		
	}	
	
	BeaconBufferPtr=0;
	
	///////////////////////
	
	//timer values 
	(void) signal(SIGALRM, handler);
	(void) setitimer(ITIMER_REAL, & tval, NULL);
	//printf("Subscribing back to MQTT Topic\n");
	MQTTClient_subscribe(client, TOPIC, QOS);
	//exit(1);
}


/*******************************************************************************************************/



int bufferindex=0;
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
	int i; //mqtt
	char* payloadptr; //mqtt
	char buffer[500]; //mqtt
	
	//printf("\nMessage arrived");
	//printf("     topic: %s\n", topicName);
	//printf("   message: ");
	
	payloadptr = message->payload;
	//manipulte the data so that jasson json parser can get the data 
	// it requiered json data in [ ]brackets so adding brackets to data first and last position 
	// data will look like [{"Data here"}]
	buffer[0]='[';
	for(i=1; i<message->payloadlen; i++)
	{
		
		buffer[i]=*payloadptr++;
		//putchar(*payloadptr++);
		
	}
	buffer[i]='}';
	buffer[i+1]=']';
	//putchar('\n');
	buffer[i+2]='\0';
	//printf("String= %s \n ",buffer);
	text= &buffer;
	/*json starts*/
	//printf("Received JSON string = \n%s\n\n",text); //json
	
	root = json_loads(text, 0, &error);
	
	if(!root)
	{
		printf("Debug:-Error Loading JSON data");
		return 1;
	}
	
	if(!json_is_array(root))
	{
		fprintf(stderr, "error: root is not an array\n");
		json_decref(root);
		return 1;
	}
	
	for(i = 0; i < json_array_size(root); i++)
	{
		json_t *data, *sha, *commit, *message;
		json_t *hostname,*beacon_type, *mac, *rssi, *uuid, *major, *minor, *tx_power;
		const char *beacon_type_txt;
		
		data = json_array_get(root, i);
		if(!json_is_object(data))
		{
			fprintf(stderr, "error: commit data %d is not an object\n", (int)(i + 1));
		json_decref(root);
		return 1;
		}
		
		hostname = json_object_get(data, "hostname");
		if(!json_is_string(hostname))
		{
			fprintf(stderr, "error: commit %d: hostname is not a string\n", (int)(i + 1));
			return 1;
		}
		
		beacon_type = json_object_get(data, "beacon_type");
		if(!json_is_string(beacon_type))
		{
			fprintf(stderr, "error: commit %d: beacon_type is not a string\n", (int)(i + 1));
			json_decref(root);
			return 1;
		}
		
		mac = json_object_get(data, "mac");
		if(!json_is_string(mac))
		{
			fprintf(stderr, "error: commit %d: mac is not a string\n", (int)(i + 1));
			json_decref(root);
			return 1;
		}
		
		rssi = json_object_get(data, "rssi");
		if(!json_is_integer(rssi))
		{
			fprintf(stderr, "error: commit %d: rssi is not a string\n", (int)(i + 1));
			json_decref(root);
			return 1;
		}
		
		uuid = json_object_get(data, "uuid");
		if(!json_is_string(uuid))
		{
			fprintf(stderr, "error: commit %d: uuid is not a string\n", (int)(i + 1));
			json_decref(root);
			return 1;
		}
		
		major = json_object_get(data, "major");
		if(!json_is_string(major))
		{
			fprintf(stderr, "error: commit %d: major is not a string\n", (int)(i + 1));
			json_decref(root);
			return 1;
		}
		
		minor = json_object_get(data, "minor");
		if(!json_is_string(minor))
		{
			fprintf(stderr, "error: commit %d: minor is not a string\n", (int)(i + 1));
			json_decref(root);
			return 1;
		}
		
		tx_power = json_object_get(data, "tx_power");
		if(!json_is_string(tx_power))
		{
			fprintf(stderr, "error: commit %d: tx_power is not a string\n", (int)(i + 1));
			json_decref(root);
			return 1;
		}
		
		printf("------------------------\n");
		memcpy(bledata[BeaconBufferPtr].host_name, json_string_value(hostname), 6);
		bledata[BeaconBufferPtr].host_name[6]='\0';
		printf("hostname[%d]: %s\n",BeaconBufferPtr,bledata[BeaconBufferPtr].host_name);
		
		memcpy(bledata[BeaconBufferPtr].beacon_type, json_string_value(beacon_type), 7);
		bledata[BeaconBufferPtr].beacon_type[7]='\0';
		printf("beacon_type[%d]: %s\n",BeaconBufferPtr,bledata[BeaconBufferPtr].beacon_type);
		
		memcpy(bledata[BeaconBufferPtr].mac, json_string_value(mac), 18);
		bledata[BeaconBufferPtr].mac[18]='\0';
		printf("mac[%d]: %s\n",BeaconBufferPtr,bledata[BeaconBufferPtr].mac);
		
		memcpy(bledata[BeaconBufferPtr].uuid, json_string_value(uuid), 32);
		bledata[BeaconBufferPtr].uuid[32]='\0';
		printf("uuid[%d]: %s\n",BeaconBufferPtr,bledata[BeaconBufferPtr].uuid);
		
		bledata[BeaconBufferPtr].rssi=json_integer_value(rssi);
		printf("rssi[%d]: %d\n",BeaconBufferPtr,bledata[BeaconBufferPtr].rssi);
		
		memcpy(bledata[BeaconBufferPtr].major, json_string_value(major), 4);
		bledata[BeaconBufferPtr].major[4]='\0';
		printf("major[%d]: %s\n",BeaconBufferPtr,bledata[BeaconBufferPtr].major);
		
		memcpy(bledata[BeaconBufferPtr].minor, json_string_value(minor), 4);
		bledata[BeaconBufferPtr].minor[4]='\0';
		printf("minor[%d]: %s\n",BeaconBufferPtr,bledata[BeaconBufferPtr].minor);
		
		memcpy(bledata[BeaconBufferPtr].tx_power, json_string_value(tx_power), 2);
		bledata[BeaconBufferPtr].tx_power[2]='\0';
		printf("tx_power[%d]: %s\n",BeaconBufferPtr,bledata[BeaconBufferPtr].tx_power);
		printf("------------------------\n");
		
		BeaconBufferPtr++;
		
		
		
	}
	
	json_decref(root);
	
	/*json ends */
	MQTTClient_freeMessage(&message);
	MQTTClient_free(topicName);
	return 1;
}

void connlost(void *context, char *cause)
{
	printf("\nConnection lost\n");
	printf("     cause: %s\n", cause);
}

int main(int argc, char* argv[])
{
	//timer logic
	struct itimerval tval;
	timerclear(& tval.it_interval);	/* zero interval means no reset of timer */
	timerclear(& tval.it_value);
	tval.it_value.tv_sec = 5;	/* 10 second timeout */
	tval.it_interval.tv_sec=5;/*reload timing 5sec */
	//timer values 
	(void) signal(SIGALRM, handler);
	(void) setitimer(ITIMER_REAL, & tval, NULL);
	////
	
	
	MQTTClient client;
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	int rc;
	int ch;
	
	MQTTClient_create(&client, ADDRESS, CLIENTID,
							MQTTCLIENT_PERSISTENCE_NONE, NULL);
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	
	MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
	
	if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
	{
		printf("Failed to connect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}
	printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
	"Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
	MQTTClient_subscribe(client, TOPIC, QOS);
	
	do 
	{
		ch = getchar();
	} while(ch!='Q' && ch != 'q');
	
	MQTTClient_unsubscribe(client, TOPIC);
	MQTTClient_disconnect(client, 10000);
	MQTTClient_destroy(&client);
	return rc;
}

