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

#define MAX_MQTT_USERNAME_LEN 20 
#define MAX_MQTT_PASSWORD_LEN 20

#define MQTT_PUB_TIMEOUT      2000L
#define MAX_CLIENT_NAME_LEN   100 
#define MAX_GATEWAY_NAME_LEN  100 
#define MAX_NETWORK_ID_LEN   10 
#define MAX_NUMBER_OF_CLIENTS 10
#define SERVER_LISTEN_PORT    10001
#define TOPIC               "/topics"
#define RC_OK 1
#define RC_ERROR 0
#define BUFFSIZ              256
#define BILLION 1E9
#define update_max_fd(a,b) ({if(a<=b){ a = b+1;} })
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

/**
 * GLOBAL variables 
 */

extern struct event_list_s events_list;
extern int max_fd;
extern fd_set read_fd_set;
extern pthread_t mqttClientConnectionTid;
extern int global_work_scan_timer_fd; 
extern char notif_broadcast_topic[MAX_NETWORK_ID_LEN + 
                            MAX_GATEWAY_NAME_LEN + 20] ; 

extern int pipe_fd[2];
extern struct list_head work_list;
extern pthread_mutex_t work_list_mutex; 
extern uint8_t mqtt_msg_count;

extern mqttConnection_t local_client;
extern mqttConnection_t remote_client;

void print_pthread_id(pthread_t id);
extern char topology_topic[MAX_NETWORK_ID_LEN + MAX_GATEWAY_NAME_LEN + 20]; 
extern char networkId[MAX_NETWORK_ID_LEN];
extern char gatewayName[MAX_GATEWAY_NAME_LEN];
#endif
