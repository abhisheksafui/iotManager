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
#ifndef SCHEDULE_H
#define SCHEDULE_H

#include<global.h> 


typedef enum {
  MQTT_MSG_TYPE_GET=1,                                                          
  MQTT_MSG_TYPE_GET_RESP,                                                       
  MQTT_MSG_TYPE_SET,                                                            
  MQTT_MSG_TYPE_SET_RESP,                                                       
  MQTT_MSG_TYPE_GET_TOPO,                                                       
  MQTT_MSG_TYPE_GET_TOPO_RESP,                                                  
  MQTT_MSG_TYPE_NOTIF_BROADCAST,                                                
  MQTT_MSG_TYPE_ERROR_RESP,                                                     
  MQTT_MSG_TYPE_GW_NODE_COMMING_UP,                                             
  MQTT_MSG_TYPE_GW_NODE_GOING_DOWN,                                             
  MQTT_MSG_TYPE_GW_COMING_UP,

  /* Timer related messages  */ 
  MQTT_MSG_TYPE_TIMER_SET_REQ = 100,
  MQTT_MSG_TYPE_TIMER_SET_RESP,
  MQTT_MSG_TYPE_TIMER_GET_REQ,
  MQTT_MSG_TYPE_TIMER_GET_RESP,
  MQTT_MSG_TYPE_TIMER_DEL_REQ,
  MQTT_MSG_TYPE_TIMER_DEL_RESP

}MQTT_MSG_TYPE;

typedef enum {
  /*
   * SCHEDULE an MQTT message to switch off an IOTSWITCH of some IOTNODE of 
   * some IOTGATEWAY. The associated time will be an interval of some seconds 
   * like 600 secs. So you need to create a time_t and match against it every 
   * second  
   */
  SWITCH_OFF_TIMER,

  ON_TIMER, 

  OFF_TIMER, 

}TIMER_TYPE;


struct schedule_s{
  char *min; 
  char *hr;
  char *dow;
  char *dom;
  char *month; 
};

typedef enum {
  INACTIVE,
  ACTIVE
}OPER_STATE;

typedef  enum {
  ONCE,
  ALWAYS
}PERSISTENCE_TYPE;

struct task_s {

  struct list_head node;
  int id; 
  uint8_t oper_state; 
  TIMER_TYPE type;
  int persist; 
  char iot_node_name[MAX_IOTNODE_NAME_LEN];
  char capability_name[MAX_CAPABILITY_NAME_LEN];  
  char gateway_name[MAX_GATEWAY_NAME_LEN];
  char req_sender_name[MAX_MQTTCLIENT_NAME_LEN]; 

  union {
    /*
     *  Used in ON/OFF timer
     */
    struct schedule_s sched_time; 
    /*
     *  Used in SWITCH_OFF  timer
     */
    time_t timeval;
  }timer_data;

  uint32_t interval; 
  struct event_subscription_s *subscription;
  /*
   * You can register the function you want to execute
   * with parameters 
   */
  void (*cmd)(void *arg);  
  void *cmd_arg; 
};

struct switch_event_s {
  uint8_t msg_type;
  char iot_node_name[MAX_IOTNODE_NAME_LEN];
  char capability_name[MAX_CAPABILITY_NAME_LEN];  
  char gateway_name[MAX_GATEWAY_NAME_LEN];
  //char req_sender_name[MAX_MQTTCLIENT_NAME_LEN]; 
  char *value; 
};

typedef enum {
   SWITCH_ON_EVENT
}SUBSC_TYPE;

struct event_subscription_s{
  struct list_head node;
  SUBSC_TYPE type;
  /* Match on the following event details and perform the following action */
  void *event;
  void *action_data;
  void (*action)(void *arg);
};

#endif

