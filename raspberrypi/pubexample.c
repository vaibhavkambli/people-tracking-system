/*******************************************************************************
 * Copyright (c) 2012, 2017 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution. 
 *
 * The Eclipse Public License is available at 
 *   http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at 
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial contribution
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "ExampleClientPub"
#define TOPIC       "happy-bubbles/ble/vaibhav/ibeacon/vaibhav" //MQ HBD topic 
#define PAYLOAD      "{\"hostname\": \"HBD001\",\r\n\"beacon_type\": \"ibeacon\",\r\n\"mac\": \"10:20:30:40:50:60\",\r\n\"rssi\": -79,\r\n\"uuid\": \"1a1a1a1a1a1a1a1a1\",\r\n\"major\": \"1234\",\r\n\"minor\": \"0004\",\r\n\"tx_power\": \"bd\"}"
#define QOS         1
#define PAYLOAD2      "{\"hostname\": \"HBD002\",\r\n\"beacon_type\": \"ibeacon\",\r\n\"mac\": \"10:20:30:40:50:60\",\r\n\"rssi\": -65,\r\n\"uuid\": \"1a1a1a1a1a1a1a1a1\",\r\n\"major\": \"1234\",\r\n\"minor\": \"0004\",\r\n\"tx_power\": \"bd\"}"
#define TIMEOUT     10000L
#define PAYLOAD3      "{\"hostname\": \"HBD003\",\r\n\"beacon_type\": \"ibeacon\",\r\n\"mac\": \"10:20:30:40:50:60\",\r\n\"rssi\": -71,\r\n\"uuid\": \"1a1a1a1a1a1a1a1a1\",\r\n\"major\": \"1234\",\r\n\"minor\": \"0004\",\r\n\"tx_power\": \"bd\"}"
#define PAYLOAD4      "{\"hostname\": \"HBD004\",\r\n\"beacon_type\": \"ibeacon\",\r\n\"mac\": \"10:20:30:40:50:60\",\r\n\"rssi\": -69,\r\n\"uuid\": \"1a1a1a1a1a1a1a1a1\",\r\n\"major\": \"1234\",\r\n\"minor\": \"0004\",\r\n\"tx_power\": \"bd\"}"
int main(int argc, char* argv[])
{
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;

    MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
  
	 int i=0;
	do
	{
		pubmsg.payload = PAYLOAD;
		pubmsg.payloadlen = strlen(PAYLOAD);
		pubmsg.qos = QOS;
		pubmsg.retained = 0;
		MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
		printf("Waiting for up to %d seconds for publication of %s\n"
		"on topic %s for client with ClientID: %s\n",
		(int)(TIMEOUT/1000), PAYLOAD, TOPIC, CLIENTID);
		rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
		printf("Message with delivery token %d delivered\n", token);
		
		
		pubmsg.payload = PAYLOAD2;
		pubmsg.payloadlen = strlen(PAYLOAD2);
		MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
		printf("Waiting for up to %d seconds for publication of %s\n"
		"on topic %s for client with ClientID: %s\n",
		(int)(TIMEOUT/1000), PAYLOAD2, TOPIC, CLIENTID);
		rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
		printf("Message with delivery token %d delivered\n", token);
		
		pubmsg.payload = PAYLOAD3;
		pubmsg.payloadlen = strlen(PAYLOAD3);
		MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
		printf("Waiting for up to %d seconds for publication of %s\n"
		"on topic %s for client with ClientID: %s\n",
		(int)(TIMEOUT/1000), PAYLOAD3, TOPIC, CLIENTID);
		rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
		printf("Message with delivery token %d delivered\n", token);
		
		pubmsg.payload = PAYLOAD4;
		pubmsg.payloadlen = strlen(PAYLOAD4);
		MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
		printf("Waiting for up to %d seconds for publication of %s\n"
		"on topic %s for client with ClientID: %s\n",
		(int)(TIMEOUT/1000), PAYLOAD4, TOPIC, CLIENTID);
		rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
		printf("Message with delivery token %d delivered\n", token);
		
		i++;
	}
	while(i<=100);
    
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}
