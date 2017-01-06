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
#include<sys/types.h>
#include<time.h>
#include<sys/time.h>
#include<sys/timerfd.h>
#include<event.h>
#include<stdlib.h>
#include<workqueue.h>
#include<string.h>

/**
 * Disarms the timer 
 *
 * @param fd
 */
static void disarm_timer(int fd)
{
    struct itimerspec tisp;      
    memset(&tisp,0,sizeof(struct itimerspec));

    if(timerfd_settime(fd,0,&tisp,NULL) < 0)          
    {           
      DEBUG("errro in mqtt timerfd_settime");
    }                                                                        
    DEBUG("timer %d is disalarmed ",global_work_scan_timer_fd);               
}


/*
 * deletes request from shared work queue in case of any error in processing 
 * the work
 */
void deleteReqBeforeEnqueue(struct request_node_s *req)
{
  if(req == NULL)
  {
    ERROR("delete_request received with request = NULL, returning");
    return;
  } 
  list_del(&req->node);
  switch(req->req_type)
  {
    case SET_REQUEST:
    case GET_REQUEST:
      /**
       * Cases that are mqtt requests 
       * Empty the params list 
       */
      freeRequestParamList(req);
      /**
       * Now that params are deleted free the req_data 
       */
      break; 
    case HELLO_REQUEST:
      break;

    default:
      break;
  } 
  free(req);
}



/*
 * Function to free the request data(along with any specific parameter list
 * inside the request data)
 */
void freeRequestParamList(struct request_node_s *req)
{
  struct request_param_s * req_param = NULL;
  struct mqtt_request_s * req_data = NULL; 

  req_data = (struct mqtt_request_s *)(req->req_data); 

  while(!list_empty(&req_data->params))
  {
    req_param = list_entry((req_data->params).next,typeof(*req_param),node);
    list_del(&req_param->node);
    if(req_param)
    {
      DEBUG("Param has arg. Freeing arg");
      free(req_param->args);
    }
    free(req_param);
  }
  free(req_data);
}

void delete_request(struct request_node_s *req)
{
 
  if(req == NULL)
  {
    ERROR("delete_request received with request = NULL, returning");
    return;
  } 
  list_del(&req->node);
  switch(req->req_type)
  {
    case SET_REQUEST:
    case GET_REQUEST:
      /**
       * Cases that are mqtt requests 
       * Empty the params list 
       */
      freeRequestParamList(req);
      /**
       * Now that params are deleted free the req_data 
       */
      break; 
    case HELLO_REQUEST:
      break;

    default:
      break;
  } 

  /**
   * Now free req.
   * mqtt_msg_count counts the current number of messages summed over all the 
   * IOTNODES that are waiting in the nodes workqueue.
   */
  free(req);
  mqtt_msg_count--;

  /**
   * Disarm global_work_scan_timer if no more requests left 
   */
  if(mqtt_msg_count == 0) 
  {
    disarm_timer(global_work_scan_timer_fd);
  }          

}
