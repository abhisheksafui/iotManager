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
#include <event.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <global.h>
#include <time.h>
#include<sys/timerfd.h>
#include<workqueue.h>
#include<log.h>

void add_node_capability(struct event_list_node *node, 
                              struct capability_node_s *cap_node)
{
  if(node == NULL)
    DEBUG("ERROR Null IOT_NODE provided, cannot add capability");
  if(cap_node == NULL)
    DEBUG("ERROR NULL cap_node provided, cannot add capability");
  
  list_add(&(cap_node->node),&(EVENT_TO_IOT_DATA(node).caps_list));
  EVENT_TO_IOT_DATA(node).caps_num++;
  DEBUG("Added cap_node %p to IOT_NODE %p. cap_num = %d",cap_node,node,
      EVENT_TO_IOT_DATA(node).caps_num);
}

/**
 * @brief Loops through the requests of an IOT node and deletes each of them
 *        safely. 
 *        Called when an IOT EVENT is deleted. 
 *
 *
 * @param node
 */
void del_node_requests(struct event_list_node *node) 
{
  struct request_node_s *req_iter=NULL,*req_tmp=NULL;
  list_for_each_entry_safe(req_iter,req_tmp,
              &EVENT_TO_IOT_DATA(node).request_list,node)
  {
       delete_request(req_iter);
  }
}

void print_node_caps_list(struct event_list_node *node)
{
  struct capability_node_s *iter=NULL;

  ASSERT_DEBUG(node->event_type == IOT_EVENT,"Expected IOT_EVENT type node");
  STD_LOG("\n START PRINTING CAPABILITY LIST for nodeid %d nodename %s \n",
      EVENT_TO_IOT_DATA(node).client_id,
      EVENT_TO_IOT_DATA(node).client_name);
  list_for_each_entry(iter,&(EVENT_TO_IOT_DATA(node).caps_list),node)
  {
    STD_LOG("\nCapability type = %d",iter->type);
    STD_LOG("\nCapability name = %s",iter->cap_id);
  }
  STD_LOG("\n START PRINTING CAPABILITY LIST for nodeid %d nodename %s \n",
      EVENT_TO_IOT_DATA(node).client_id,
      EVENT_TO_IOT_DATA(node).client_name);
}
/**
 * @brief delete node capabilities while deleting IOT_EVENT node 
 *
 * @param node
 */
void del_node_caps(struct event_list_node *node)
{
  struct capability_node_s *iter=NULL,*tmp=NULL;
  CHECK(node->event_type==IOT_EVENT,"Capabilities are meant for IOT_EVENT only");
  
  print_node_caps_list(node);

  while(!list_empty(&(EVENT_TO_IOT_DATA(node).caps_list)))
  {
    list_for_each_entry_safe(iter,tmp,&(EVENT_TO_IOT_DATA(node).caps_list),node)
    {
      DEBUG("Deleting Node capabiity of type %d and cap_id %s",
                  iter->type,
                  iter->cap_id);
      list_del(&(iter->node));
      free(iter);
    }

  }
  return;
error:
  DEBUG("Error occured in del_node_caps");
  //exit(1);
}
/**
 * @brief Adds an event to event list
 *
 * @param list
 * @param fdsetptr
 * @param fd
 * @param event_type
 *
 * @return 
 */
struct event_list_node * add_event(struct event_list_s *list,fd_set *fdsetptr,int fd, EVENT_TYPE event_type)
{
  STD_LOG("\nADDING EVENT\n");
  struct event_list_node *node = NULL;
  struct itimerspec tisp;
  node  =(struct event_list_node *) malloc(sizeof(struct event_list_node));
 
  if(node == NULL)
  {
    goto return_label;
  }
  node->event_fd = fd;
  node->event_type = event_type; 
  node->del_mark = 0; 
  list->event_count++;
  DEBUG("Addeding node %p",node);
  list_add(&(node->head),&(list->head)); 
  
  DEBUG("Added node %p",node);
  
  FD_SET(fd,fdsetptr);
  
  /**
   * Node added to list. Now do task based on EVENT TYPE 
   */
  if(event_type == IOT_EVENT)
  {
    /**
     * IOT_event needs to set up a timer event associated with it  
     */
    //snprintf(node->event_data.iotdata.client_id,MAX_CLIENT_ID_LEN,
    node->event_data.iot_data.client_id = fd;
    node->event_data.iot_data.hello_count = 3;
    EVENT_TO_IOT_DATA(node).caps_num = 0;
    EVENT_TO_IOT_DATA(node).work_state = IOT_NODE_FREE;
    INIT_LIST_HEAD(&(EVENT_TO_IOT_DATA(node).caps_list));
    INIT_LIST_HEAD(&(EVENT_TO_IOT_DATA(node).request_list));

    //print_node_caps_list(node);
    DEBUG("IOT_EVENT added client_id = %d", fd);
  }
  else if(event_type == TIMER_EVENT)
  {
    memset(&tisp,0,sizeof(tisp)); 
    tisp.it_interval.tv_sec = 20;
    tisp.it_value.tv_sec = 20;
     
    if(timerfd_settime(fd,0,&tisp,NULL) < 0)
    {
       del_event(list,node,fdsetptr);
       DEBUG("errro in timerfd_settime");
       free(node);
       node = NULL;
       goto return_label;
    }
    DEBUG("timer %d set up to expire in 20 secs",fd);
  }
  else if(event_type == MQTT_MSG_EVENT)
  {
    /**
     * No special addition for MQTT_MSG_EVENT 
     */
  }
  else if(event_type == WORK_SCAN_TIMER_EVENT)
  {
    DEBUG("Adding WORK SCAN TIMER. No special processing");
  } 
  /**
   * Updating max_fd
   */
  DEBUG("BEFORE updation max_fd is now %d fd is %d",max_fd,fd);
  update_max_fd(max_fd,fd);
  DEBUG("AFTER UPDATION max_fd is now %d fd is %d",max_fd,fd);
return_label:
  return node;
}

int del_event(struct event_list_s *list, struct event_list_node *node,
            fd_set *fdset) 
{
  int ret_code = RC_OK;

  list_del(&(node->head));
  list->event_count--;
  FD_CLR(node->event_fd, fdset); 
  close(node->event_fd);

  /**
   * If we are deleting IOT_EVENT then we should delete any capability node
   * inside this IOT_EVENT node.
   * Also need to delete request list inside iotnode
   */
  if(node->event_type == IOT_EVENT)
  {
      del_node_caps(node);
      del_node_requests(node); 
  }
  free(node); 
  compute_max_fd(max_fd);

  return ret_code;
} 

