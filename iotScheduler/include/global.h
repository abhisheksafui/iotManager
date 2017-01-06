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
#ifndef GLOBAL_H
#define GLOBAL_H

//#include<MQTTClient.h>
#include<pthread.h>
#include<inttypes.h>
#include<mqttdefs.h>


#define MAX_FILE_PATH_LEN 256

#define MAX_NETWORK_ID_LEN   10 

#define MAX_MQTT_USERNAME_LEN 20 
#define MAX_MQTT_PASSWORD_LEN 20
#define MAX_IOTNODE_NAME_LEN 64
#define MAX_CAPABILITY_NAME_LEN 32
#define MAX_GATEWAY_NAME_LEN 100
#define MAX_MQTTCLIENT_NAME_LEN 100

#define SERVER_LISTEN_PORT    10001
#define RC_OK 1
#define RC_ERROR 0
#define update_max_fd(a,b) ({if(a<=b){ a = b+1;} })

#define GET_SHARED_DATA_LOCK() pthread_mutex_lock(&task_list_mutex);
#define RELEASE_SHARED_DATA_LOCK() pthread_mutex_unlock(&task_list_mutex);
  


#define compute_max_fd(a)  ({\
    a=0;\
    struct event_list_node *iter;\
    list_for_each_entry(iter,&(events_list.head),head)\
    {\
      if(iter->event_fd >= a)\
      {\
        a = iter->event_fd+1;\
      }\
    }\
  })

#define JSON_CHK_OBJECT(j) ({\
    uint8_t ret = json_is_object(j);\
    if(!ret)\
    {DEBUG("Json value is not object"); }\
    ret;\
    })
#define JSON_CHK_INTEGER(j) ({\
    uint8_t ret = json_is_integer(j);\
    if(!ret)\
    {DEBUG("Json value is not integer"); }\
    ret;\
    })
#define JSON_CHK_STRING(j) ({\
    uint8_t ret = json_is_string(j);\
    if(!ret)\
    {DEBUG("Json value is not string"); }\
    ret;\
    })
#define JSON_CHK_ARRAY(j) ({\
    uint8_t ret = json_is_array(j);\
    if(!ret)\
    {\
    DEBUG("Json value is not array");\
    }\
    ret;\
    })
/**
 * GLOBAL variables 
 */
extern char networkId[MAX_NETWORK_ID_LEN];
char gatewayName[MAX_GATEWAY_NAME_LEN];
extern uint16_t gatewayPort; 

extern struct list_head task_list;
extern pthread_mutex_t task_list_mutex; 
extern mqttConnection_t local_client; 
extern mqttConnection_t remote_client; 
extern uint32_t task_count;
extern char gateway_topic[MAX_GATEWAY_NAME_LEN]; 
extern char  save_dir[MAX_FILE_PATH_LEN]; 
extern char logfile_name[MAX_FILE_PATH_LEN];
extern struct list_head event_subsc_list;
#endif
