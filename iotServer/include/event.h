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
#ifndef EVENT_H
#define EVENT_H

#include <unistd.h>
#include <list.h>
#include <sys/select.h>
#include <inttypes.h>
#include <global.h>
#include <mqttdefs.h>
#include<log.h>


#define HELLO_COUNT_DEFAULT 3
#define MAX_IOT_NODE_NAME_LEN 50
#define MAX_CAP_ID_LEN 15
#define RESET_HELLO_COUNT(node)  ({node->event_data.iot_data.hello_count = HELLO_COUNT_DEFAULT;})
#define CLIENT_DEAD(a)  ({ (((a)->event_data).iot_data.hello_count == 0)? 1: 0;})

#define NODE2IOT_DATA(node) ((node->event_data).iot_data)
#define EVENT_TO_TIMER_DATA(node) ((node->event_data).timer_data)
#define EVENT_TO_IOT_DATA(node) ((node->event_data).iot_data)
#define TIMER_PTR_TO_IOT_PTR(node) (EVENT_TO_TIMER_DATA(node).node_ptr)
#define IOT_PTR_TO_TIMER_PTR(node) (EVENT_TO_IOT_DATA(node).timer_ptr)
#define TIMER_TO_IOT_DATA(node) (EVENT_TO_IOT_DATA(TIMER_PTR_TO_IOT_PTR(node)))

#define MARK_FOR_DELETION(node) do{\
                                if(node)\
                                {\
                                  node->del_mark = 1;\
                                }\
                                }while(0);

#define ENQUEUE_IOT_REQUEST(iot_node,request) ({\
    list_add_tail(&request->node,&(EVENT_TO_IOT_DATA(iot_node).request_list));\
    mqtt_msg_count++; \
    if(mqtt_msg_count == 1)\
    {\
      struct itimerspec tisp;\
      memset(&tisp,0,sizeof(tisp));\
      tisp.it_interval.tv_sec = 1;\
      tisp.it_value.tv_nsec = 50;\
      if(timerfd_settime(global_work_scan_timer_fd,0,&tisp,NULL) < 0)\
      {\
         ERROR("errro in mqtt timerfd_settime");\
      }\
      else\
        DEBUG("timer %d set up to expire in 1 seconds",global_work_scan_timer_fd);\
    }\
})

#define IOT_NODE_BUSY  1
#define IOT_NODE_FREE  2

typedef enum
{
  IOT_MSG_TYPE_UNKNOWN,
  IOT_MSG_TYPE_CAPS,
  IOT_MSG_TYPE_HELLO,
  IOT_MSG_TYPE_GET_REQ,
  IOT_MSG_TYPE_GET_RESP,
  IOT_MSG_TYPE_SET_REQ,
  IOT_MSG_TYPE_SET_RESP,
  IOT_MSG_TYPE_NOTIF_EVENT

}IOT_MSG_TYPES;

typedef enum 
{

  LISTEN_SOCKET,
  IOT_EVENT,
  TIMER_EVENT,
  MQTT_MSG_EVENT,
  WORK_SCAN_TIMER_EVENT

}EVENT_TYPE;

typedef enum 
{
  CAP_SWITCH,
  CAP_LED_MATRIX_DISPLAY,
  CAP_POT,
  CAP_RTC,
  CAP_FAN_SPEED,
  CAP_TEMP_SENSOR,
  CAP_BUZZER

}CAPABILITY;

typedef enum
{
  GET_REQUEST=1,
  GET_REQUEST_RESP,
  SET_REQUEST,
  SET_REQUEST_RESP,
  HELLO_REQUEST,
  HELLO_RESPONSE

}REQUEST_TYPE;

typedef enum
{
  REQUEST_SENT,          //IOT NODE BUSY
  REQUEST_PENDING        //ALL other requests in the queue

}REQUEST_STATUS;

struct request_param_s
{
  struct list_head node;
  char id[MAX_CAP_ID_LEN];
  void *args;  
};

struct mqtt_request_s 
{
  char req_sender_name[MAX_CLIENT_NAME_LEN];
  char iotnode_name[MAX_IOT_NODE_NAME_LEN];
  uint8_t iotnode_id;
  struct list_head params;
  mqttConnection_t *brokerConnection; 
};

struct request_node_s 
{
  REQUEST_TYPE req_type;
  REQUEST_STATUS req_status;
  struct timespec timestamp;
  void *req_data; 
  struct list_head node;  
};


struct capability_node_s {

  CAPABILITY type;
  char cap_id[MAX_CAP_ID_LEN];
  struct list_head node;
   

};

struct iot_data_s 
{

  int client_id;
  uint8_t hello_count;
  /**
   * work_state can be either BUSY(when IOT node is already processing a request
   * or FREE(when IOT node is not processing any request.)
   */
  uint8_t work_state; 
  char client_name[MAX_IOT_NODE_NAME_LEN];
  uint8_t caps_num;   //bit flags for each capability; 
  struct list_head caps_list;
  struct list_head request_list;
  /*
   * pointer to hello rimer for this iot node
   */
  struct event_list_node *timer_ptr;
  

};

struct timer_data_s 
{

  /**
   * Store the pointer to the node for which this timer has been created;
   */
  struct event_list_node *node_ptr;

};
union event_data_s
{
  struct timer_data_s timer_data;
  struct  iot_data_s iot_data;   
};

struct event_list_node 
{
  struct list_head head;
  int event_fd;
  EVENT_TYPE event_type;
  uint8_t del_mark;               //used by gc  
  union event_data_s event_data;
};

struct event_list_s
{
  struct list_head head;
  int event_count;
};

struct event_list_node *add_event(struct event_list_s *list,fd_set *fdsetptr,int fd, EVENT_TYPE event_type);
int del_event(struct event_list_s *list, struct event_list_node *node,fd_set *fdset); 
void del_node_caps(struct event_list_node *node);
void del_node_requests(struct event_list_node *node); 
void add_node_capability(struct event_list_node *node, 
                              struct capability_node_s *cap_node);

void print_node_caps_list(struct event_list_node *node);

static inline void gc_events_list(void)
{
  struct event_list_node *iter,*tmp;
  list_for_each_entry_safe(iter,tmp,&(events_list.head),head)
  {
    if(iter->del_mark == 1)
    {
      DEBUG("GC deleting event_node %d.",iter->event_fd);
      /**
       * Should take some lock here before deleting as our process is multi 
       * threaded. TBD
       */
      del_event(&events_list,iter,&read_fd_set);
    }
  }
}


#endif 
