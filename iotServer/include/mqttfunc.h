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
#ifndef MQTTFUNC_H
#define MQTTFUNC_H
#include<mqttdefs.h>

bool mqttSetCallbacks(struct mosquitto *m); 
void mqttClientSubscribe(mqttConnection_t *cl,char *topic,uint8_t qos);
int publishMqttMessage(char *message, const char *topic,uint8_t qos,
                       mqttConnection_t *cl);
void subscribe_topics(mqttConnection_t *cl, int qos);

struct mosquitto * mqttClientInit(mqttConnection_t *cl,bool cleansession);

void mqttStartClientThread(mqttConnection_t *cl);
/*
 * Callback functions registered
 */
void onMessage(struct mosquitto *m,void *context,
                   const struct mosquitto_message *message);
void onConnect(struct mosquitto *m, void* context, 
                    int ret);                  
void onSubscribe(struct mosquitto *m,
                  void *context,
                  int mid,int qos_count,
                  const int *granted_qos); 
void onPublish(struct mosquitto *m,void *context,int mid);
void mqttClientConnect(mqttConnection_t *cl);

void process_mqtt_msg(mqttConnection_t *cl,char *topicName, 
                     const struct mosquitto_message *message);
int append_iot_node_caps(json_t *iot_json_object, struct event_list_node *node);
int append_iot_nodes(json_t *reply_msg);

#endif
