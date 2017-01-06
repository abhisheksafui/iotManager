/* Copyright 2016 Abhishek Safui
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef MQTT_DEFS_H
#define MQTT_DEFS_H
#include<mosquitto.h>
#include<list.h>
#include<inttypes.h>
#include<pthread.h>

#define MAX_TOPICS_PER_CLIENT 5 

typedef struct subscriptionTopic_s {

  char *topic;
  int qos;
  struct list_head node;

}subscriptionTopic_t;

typedef struct mqttConnection_s
{
  struct mosquitto *client;
  char *serverUri;
  uint16_t serverPort;
//  char *clientId;
  uint8_t connectionStatus; 
  
  char *subscribedTopics[MAX_TOPICS_PER_CLIENT]; //For first implementation 
                                                 //Keeping array. Will move to list
  uint8_t subscribedTopicCount;
  int subscriptionQos[MAX_TOPICS_PER_CLIENT]; 
  pthread_t connectionThreadPid;  

  char *username;
  char *password; 

} mqttConnection_t;

#endif
