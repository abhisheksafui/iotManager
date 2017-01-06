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
#include<string.h>
#include<event.h> 
#include<stdlib.h> 
#include<node.h>
#include<event.h>
#include<jansson.h>
#include<workqueue.h>
#include<mqttJsonIf.h>
#include<mqttfunc.h>
#include<errno.h>
#include<log.h>
#include<json_utils.h>

uint8_t  parse_msg_type(json_t *resp)
{
  uint8_t rc = IOT_MSG_TYPE_UNKNOWN;
  
  json_t  *type = json_object_get(resp,"MSGTYPE");
  if(type == NULL)
  {
    ERROR("Message from iotnode does not contain MSGTYPE.. skipping");
    goto done;
  }
  if(!JSON_CHK_INTEGER(type))
  {
    ERROR("Error in getting MSGTYPE, integer expected but not so");
    goto done;
  }
  rc = json_integer_value(type);
  switch(rc)
  { 
    case IOT_MSG_TYPE_CAPS:
    case IOT_MSG_TYPE_HELLO:
    case IOT_MSG_TYPE_GET_RESP: 
    case IOT_MSG_TYPE_SET_RESP:
    case IOT_MSG_TYPE_NOTIF_EVENT:

      break;
    default:
      rc = IOT_MSG_TYPE_UNKNOWN;
      ERROR("Unexpected MSGTYPE..skipping");
  }
done: 
      return rc;;
}

/**
 * @brief Parses IOT node name from CAPS message
 *
 * @param node pointer to the node that sent the CAPS
 * @param IN out pointer for receiving start of msg and returns pointer to 
 * next msg string
 */
void parse_and_fill_node_name(struct event_list_node *node,
    json_t *resp)
{
  const char *str_p;
  json_t  *name = json_object_get(resp,"NAME");
  if(name == NULL)
  {
    ERROR("Capability Message from iotnode does not contain NAME.. skipping");
    goto done;
  }
  if(!JSON_CHK_STRING(name))
  {
    ERROR("Error in getting STRING, string expected but not so");
    goto done;
  }

  str_p = json_string_value(name);
  DEBUG("Client %d said her name is %s",node->event_fd,
      str_p);
  strncpy(EVENT_TO_IOT_DATA(node).client_name,str_p,strlen(str_p)+1);
done:
  return;
  
}

int8_t  parse_and_fill_node_caps(struct event_list_node *node,json_t *resp)
{
  int rc = RC_OK;
  const char *str_p =NULL;
  int type;
  struct capability_node_s *cap_node = NULL;
  size_t index;
  json_t *value;
  json_t  *caps_array = json_object_get(resp,"CAPS_ARRAY");

  if(caps_array == NULL)
  {
    ERROR("Capability Message from iotnode does not contain CAPS_ARRAY.. skipping");
    rc =RC_ERROR;
    goto done;
  }
  if(!JSON_CHK_ARRAY(caps_array))
  {
    ERROR("Error in getting CAPS_ARRAY, ARRAY expected but not so");
    rc =RC_ERROR;
    goto done;
  }

  json_t *jobj; 
  json_array_foreach(caps_array, index, value) {
    
    jobj = json_object_get(value, "TYPE");
    if(!jobj)
    {
      ERROR("CAPS_ARRAY  from iotnode does not contain TYPE.. skipping");
      rc =RC_ERROR;
      goto done;
    }
    if(!JSON_CHK_INTEGER(jobj))
    {
      ERROR("Error in getting TYPE,INTEGR expected but not so");
      rc =RC_ERROR;
      goto done;
    }

    type = json_integer_value(jobj);
    switch(type)
    {
      case CAP_SWITCH:                                                                   
      case CAP_LED_MATRIX_DISPLAY:
      case CAP_POT:
      case CAP_RTC:
      case CAP_FAN_SPEED:                                     
      case CAP_TEMP_SENSOR: 
      case CAP_BUZZER:

      break; 
      default: 
        ERROR("Unexpected capability type..skipping");
        goto done;      
    }

    jobj = json_object_get(value, "CAP_ID");
    if(!jobj)
    {
      ERROR("CAPS_ARRAY  from iotnode does not contain CAP_ID.. skipping");
      rc =RC_ERROR;
      goto done;
    }
    if(!JSON_CHK_STRING(jobj))
    {
      ERROR("Error in getting CAP_ID, STRING expected but not so");
      rc =RC_ERROR;
      goto done;
    }
    str_p = json_string_value(jobj);
    cap_node = (struct capability_node_s *)
              calloc(1,sizeof(struct capability_node_s));
    CHECK_MEM(cap_node != NULL);
    cap_node->type=type;
    strncpy(cap_node->cap_id,str_p,strlen(str_p)+1);
    
    add_node_capability(node,cap_node);
  }
done:
  //exit(1);
  return rc;
}

void process_iot_msg(struct event_list_node *node,char *buff, uint16_t len)
{
  uint8_t msg_type;
  json_t *root,*array,*object,*jobj;
  struct request_node_s *req = NULL;
  json_error_t error;
  uint8_t  ret=0;
  int index;
  json_t *value;
  json_t *req_array;

  json_t *node_resp = NULL;
  DEBUG("CLIENT %d: %s",node->event_fd, buff);  
   
  node_resp = json_loadb(buff,len,(size_t)JSON_DISABLE_EOF_CHECK,&error); 
  if(!node_resp)
  {
    ERROR("json_loads error: %d:%s",error.line,error.text);
    goto done_exit;
  } 
  ret = JSON_CHK_OBJECT(node_resp);
  if(!ret){
    ERROR("json is not object");
    goto EXIT;
  }
   
  msg_type = parse_msg_type(node_resp);

  switch(msg_type)
  {
    case IOT_MSG_TYPE_HELLO:

      RESET_HELLO_COUNT(node);
      DEBUG("IOT_NODE %s sent hello, current hello_count %d",
          EVENT_TO_IOT_DATA(node).client_name,
          EVENT_TO_IOT_DATA(node).hello_count);

      if(list_empty(&EVENT_TO_IOT_DATA(node).request_list))
      {
        ASSERT_DEBUG(0,"Unexpected Hello request");
        break;
      }
      /**
       * Do some checks that the first request in the list is HELLO. 
       * If so delete it 
       */
      req = list_entry(EVENT_TO_IOT_DATA(node).request_list.next,
                         typeof(*req), node);
      ASSERT_DEBUG(req->req_type == HELLO_REQUEST,"HELLO REQUEST expected");
      ASSERT_DEBUG(req->req_status == REQUEST_SENT,"REQUEST_SENT expected");
      ASSERT_DEBUG(EVENT_TO_IOT_DATA(node).work_state == IOT_NODE_BUSY,
                                      "Client should ihave been busy");
      DEBUG("ALL Checks passed now deleting HELLO request");
      delete_request(req);
      EVENT_TO_IOT_DATA(node).work_state = IOT_NODE_FREE;
      break;

    case IOT_MSG_TYPE_CAPS:


      parse_and_fill_node_name(node,node_resp);
      parse_and_fill_node_caps(node,node_resp);

      print_node_caps_list(node);
      /*sprintf(buff,"/%s/%s",CLIENTID,EVENT_TO_IOT_DATA(node).client_name);
      if(MQTTClient_subscribe(client, buff , 0)!= MQTTCLIENT_SUCCESS)                                            
      {                                                                             
        DEBUG("ERROR in MQTTClient_subscribe %s", buff);                                     
      } 
      break;
      */

      /*
       * Advertise the Availability of the node to all listeners on /topic
       */ 
      root = json_object();
      json_object_set_new(root,"msgtype",
                          json_integer(MQTT_MSG_TYPE_GW_NODE_COMMING_UP));
      
      json_object_set_new(root,"gw_name",
          json_string(gatewayName));
     
      json_object_set_new(root,"nodename",
            json_string(EVENT_TO_IOT_DATA(node).client_name));
      
      append_iot_node_caps(root, node);
      char *msg = json_dumps(root,0);

      /*
       * Send the message to either NOTIFICATION BROADCAST TOPIC or to /topics
       * For now sending to /topics to both local and remote 
       * */ 
      publishMqttMessage(msg,notif_broadcast_topic,0,                     
                            &remote_client);
      publishMqttMessage(msg,notif_broadcast_topic,0,                     
                            &local_client);

      free(msg);  
      EVENT_TO_IOT_DATA(node).work_state = IOT_NODE_FREE;
      json_decref(root);

      break;

    case IOT_MSG_TYPE_GET_RESP:
      /**
       * Check that the first request in the list matches
       */
      req = list_entry(EVENT_TO_IOT_DATA(node).request_list.next,
               typeof(*req), node );
      ASSERT_DEBUG(EVENT_TO_IOT_DATA(node).work_state==IOT_NODE_BUSY, 
          "GOT GET_RESPONSE but node state is not busy!!! Strange.");

      ASSERT_DEBUG(req->req_status == REQUEST_SENT, 
        "GOT GET_RESPONSE but first request of IOT node is not REQUEST_SENT.");
      
      ASSERT_DEBUG(req->req_type == GET_REQUEST, 
          "GOT GET_RESPONSE but first request of IOT node not of type"
          "GET_REQUEST.");
      
      req_array = json_object_get(node_resp,"REQ_ARRAY");

      if(req_array == NULL)
      {
        ERROR("GET Message from iotnode does not contain REQ_ARRAY.. skipping");
        goto EXIT;
      }
      if(!JSON_CHK_ARRAY(req_array))
      {
        ERROR("Error in getting REQ_ARRAY, ARRAY expected but not so");
        goto EXIT;
      }

      array = json_array();

      json_array_foreach(req_array,index,value)
      {
        object = json_object();
        jobj = json_object_get(value, "CAP_ID");
        if(jobj == NULL)
        {
          ERROR("GET Response doesnot contain CAP_ID");
          json_decref(array);
          json_decref(object);

          goto EXIT;
        }
        if(!JSON_CHK_STRING(jobj))
        {
          ERROR("STRING expected for CAP_ID, but not so)");
          json_decref(array);
          json_decref(object);
          goto EXIT;
        }
        
        json_object_set_new(object,"id",json_string(json_string_value(jobj)));

        jobj = json_object_get(value, "VALUE");
        if(jobj == NULL)
        {
          ERROR("GET Response doesnot contain VALUE");
          json_decref(array);
          json_decref(object);
          goto EXIT;
        }
        if(!JSON_CHK_STRING(jobj))
        {
          ERROR("STRING expected for VALUE, but not so)");
          json_decref(array);
          json_decref(object);
          goto EXIT;
        }
        json_object_set_new(object,"value",json_string(json_string_value(jobj)));

        jobj = json_object_get(value, "UNAME");
        if(jobj == NULL)
        {
          DEBUG("GET Response doesnot contain VALUE");
        }
        else if(!JSON_CHK_STRING(jobj))
        {
          ERROR("STRING expected for VALUE, but not so)");
          json_decref(array);
          json_decref(object);
          goto EXIT;
        }
        json_object_set_new(object,"uname",json_string(json_string_value(jobj)));

        jobj = json_object_get(value, "TIMESTAMP");
        if(jobj == NULL)
        {
          DEBUG("GET Response doesnot contain TIMESTAMP");
        }
        else if(!JSON_CHK_INTEGER(jobj))
        {
          ERROR("INTEGER expected for VALUE, but not so)");
          json_decref(array);
          json_decref(object);
          goto EXIT;
        }
        json_object_set_new(object,"timestamp",json_integer(json_integer_value(jobj)));
        json_array_append_new(array,object);
      }

      root = json_object();
      json_object_set_new(root,"msgtype",json_integer(MQTT_MSG_TYPE_GET_RESP));
      json_object_set_new(root,"nodeid",
          json_integer(EVENT_TO_IOT_DATA(node).client_id));
      json_object_set_new(root,"nodename",
            json_string(EVENT_TO_IOT_DATA(node).client_name));
      json_object_set_new(root,"items",array);
      json_object_set_new(root,"gwname",json_string(gatewayName));
      msg = json_dumps(root,0);
      char topic[BUFFSIZ]; 
      /**
       * Now send the MQTT message and free the msg ptr and delete the req        
       */
        snprintf(topic,sizeof(topic),"%s/%s",networkId,
          ((struct mqtt_request_s *)req->req_data)->req_sender_name);    

      DEBUG("\nGoing to send msg on topic %s->\n%s\n",topic,msg);
      publishMqttMessage(msg,topic,0,((struct mqtt_request_s *)                      
                            req->req_data)->brokerConnection);

      free(msg);  
      delete_request(req);
      EVENT_TO_IOT_DATA(node).work_state = IOT_NODE_FREE;
      json_decref(root);
      break;

    case IOT_MSG_TYPE_SET_RESP:
    case IOT_MSG_TYPE_NOTIF_EVENT:

      /**
       * Check that the first request in the list matches
       */
      if(msg_type == IOT_MSG_TYPE_SET_RESP)
      {

        req = list_entry(EVENT_TO_IOT_DATA(node).request_list.next,
               typeof(*req), node);
        ASSERT_DEBUG(EVENT_TO_IOT_DATA(node).work_state==IOT_NODE_BUSY, 
          "GOT GET_RESPONSE but node state is not busy!!! Strange.");
        ASSERT_DEBUG(req->req_status == REQUEST_SENT, 
        "GOT GET_RESPONSE but first request of IOT node is not REQUEST_SENT.");
        ASSERT_DEBUG(req->req_type == SET_REQUEST, 
          "GOT GET_RESPONSE but first request of IOT node not of type"
          "GET_REQUEST.");
      }

      req_array = json_object_get(node_resp,"REQ_ARRAY");
      if(req_array == NULL)
      {
        ERROR("GET Message from iotnode does not contain REQ_ARRAY.. skipping");
        goto EXIT;
      }
      if(!JSON_CHK_ARRAY(req_array))
      {
        ERROR("Error in getting REQ_ARRAY, ARRAY expected but not so");
        goto EXIT;
      }


      array = json_array();

      json_array_foreach(req_array,index,value)
      {
        object = json_object();
        jobj = json_object_get(value, "CAP_ID");
        if(jobj == NULL)
        {
          ERROR("SET Response doesnot contain CAP_ID");
          json_decref(array);
          json_decref(object);

          goto EXIT;
        }
        if(!JSON_CHK_STRING(jobj))
        {
          ERROR("STRING expected for CAP_ID, but not so)");
          json_decref(array);
          json_decref(object);
          goto EXIT;
        }
        
        json_object_set_new(object,"id",json_string(json_string_value(jobj)));

        jobj = json_object_get(value, "VALUE");
        if(jobj == NULL)
        {
          ERROR("GET Response doesnot contain VALUE");
          json_decref(array);
          json_decref(object);
          goto EXIT;
        }
        if(!JSON_CHK_STRING(jobj))
        {
          ERROR("STRING expected for VALUE, but not so)");
          json_decref(array);
          json_decref(object);
          goto EXIT;
        }
        json_object_set_new(object,"value",json_string(json_string_value(jobj)));

        jobj = json_object_get(value, "UNAME");
        if(jobj == NULL)
        {
          DEBUG("GET Response doesnot contain VALUE");
        }
        else if(!JSON_CHK_STRING(jobj))
        {
          ERROR("STRING expected for VALUE, but not so)");
          json_decref(array);
          json_decref(object);
          goto EXIT;
        }
        json_object_set_new(object,"uname",json_string(json_string_value(jobj)));

        jobj = json_object_get(value, "TIMESTAMP");
        if(jobj == NULL)
        {
          DEBUG("GET Response doesnot contain TIMESTAMP");
        }
        else if(!JSON_CHK_INTEGER(jobj))
        {
          ERROR("STRING expected for VALUE, but not so)");
          json_decref(array);
          json_decref(object);
          goto EXIT;
        }
        json_object_set_new(object,"timestamp",json_integer(json_integer_value(jobj)));

        json_array_append_new(array,object);
      }
      
      root = json_object();
      if(msg_type == IOT_MSG_TYPE_SET_RESP)
        json_object_set_new(root,"msgtype",json_integer(MQTT_MSG_TYPE_SET_RESP));
      else if(msg_type == IOT_MSG_TYPE_NOTIF_EVENT) 
        json_object_set_new(root,"msgtype",json_integer(MQTT_MSG_TYPE_NOTIF_BROADCAST));


      json_object_set_new(root,"nodeid",
          json_integer(EVENT_TO_IOT_DATA(node).client_id));
      json_object_set_new(root,"nodename",
            json_string(EVENT_TO_IOT_DATA(node).client_name));
      json_object_set_new(root,"items",array);
      json_object_set_new(root,"gwname",json_string(gatewayName));

      msg = json_dumps(root,0);
      /**
       * Now send the MQTT message and free the msg ptr and delete the req 
       * --IMPORTANT CHANGE--
       *  Now any set reaponse will be sent to broadcast topic 
       *  BOTH for local as well as for remote broker       
       */
      /*snprintf(topic,sizeof(topic),"%s",
          ((struct mqtt_request_s *)req->req_data)->req_sender_name);*/    

      DEBUG("\nGoing to send msg on topic %s->\n%s\n",notif_broadcast_topic,
                                                        msg);
      publishMqttMessage(msg,notif_broadcast_topic,2, &remote_client);

      publishMqttMessage(msg,notif_broadcast_topic,2, &local_client);

      free(msg);
      if(msg_type == IOT_MSG_TYPE_SET_RESP) 
      { 
        /* 
         * request was searched only to match set response to a set request.
         * Notif events are random and does not have any matching request. 
         */
        delete_request(req);
        EVENT_TO_IOT_DATA(node).work_state = IOT_NODE_FREE;
      }
      
      json_decref(root);
      break;

    case IOT_MSG_TYPE_UNKNOWN:
    default:
      DEBUG("IOT_NODE %d sent UNKNOWN message", node->event_fd);
  }

return; 

EXIT:
  json_decref(node_resp);
done_exit:
return; 
}

void sendNodeMessage(struct event_list_node *event)
{
  /**
   * Check that the first node in the request node is pending and also the state
   * of this client 
   */
  struct request_node_s *req = NULL;
  struct request_param_s *param_iter = NULL;  
  int size; 
  char *buff;
  char ret_code = RC_OK;
  ASSERT_DEBUG(EVENT_TO_IOT_DATA(event).work_state == IOT_NODE_FREE,
        "Error IOT node not free");
  ASSERT_DEBUG(!list_empty(&EVENT_TO_IOT_DATA(event).request_list),
        "Error IOT node has empty request list, should not be here.");
  req = list_entry(EVENT_TO_IOT_DATA(event).request_list.next,
               typeof(*req), node );
  ASSERT_DEBUG(req!=NULL,"Error request is null"); 

  ASSERT_DEBUG(req->req_status == REQUEST_PENDING,
      "ERROR If the node is free that means no request should be in sent state"); 
  
  json_t *msg,*jobj,*jarray;  
  
  msg = json_object();
  switch(req->req_type)
  {
    case HELLO_REQUEST: 
      json_object_set_new(msg,"MSGTYPE",json_integer(IOT_MSG_TYPE_HELLO));
    break;
    case GET_REQUEST: 

      json_object_set_new(msg,"MSGTYPE",json_integer(IOT_MSG_TYPE_GET_REQ));
      jarray = json_array();
      list_for_each_entry(param_iter,
          &((struct mqtt_request_s *)req->req_data)->params,node)
      {
        jobj = json_object();
        json_object_set_new(jobj,"CAP_ID",json_string(param_iter->id)); 
        json_array_append_new(jarray, jobj);
      }
      json_object_set_new(msg,"REQ_ARRAY",jarray);
      
    break; 

    case SET_REQUEST: 
      json_object_set_new(msg,"MSGTYPE",json_integer(IOT_MSG_TYPE_SET_REQ));
      json_object_set_new(msg,"REQ_SENDER_NAME",
          json_string(((struct mqtt_request_s *)(req->req_data))->req_sender_name));
      jarray = json_array();
      list_for_each_entry(param_iter,
          &((struct mqtt_request_s *)req->req_data)->params,node)
      {
        jobj = json_object();
        json_object_set_new(jobj,"CAP_ID",json_string(param_iter->id)); 
        if(param_iter->args != NULL)
        {
          json_object_set_new(jobj,"CAP_ID",json_string(param_iter->id));
          json_object_set_new(jobj,"VALUE",json_string(param_iter->args)); 
        }
        else
        {
          ERROR("Expected  param->args in set request, but missing");
        }
        json_array_append_new(jarray, jobj);
      }
      json_object_set_new(msg,"REQ_ARRAY",jarray);
    break;

    default:

    DEBUG("Error should not be here");
  }

  DEBUG("MSG that is ready to be sent to IOT NODE is %s %u", 
      json_dumps(msg,1),
      strlen(json_dumps(msg,1)));

  size = strlen(json_dumps(msg,1));
  while(size > 0 && 
          (ret_code = write(event->event_fd,
                              json_dumps(msg,1),
                              size)) != size)
  {
     if(ret_code < 0 && errno==EINTR)
     {
        ERROR("Error write interrupted");
        continue; 
     }
     if(ret_code < 0) 
     {
        ERROR("Some error occured in write to iotnode");
        break;
     }
     size -= ret_code; 
     buff += ret_code; 
  } 
  DEBUG("Message sent to IOT NODE now marking it busy and request "
      "status to REQUEST_SENT from REQUEST_PENDING");

  EVENT_TO_IOT_DATA(event).work_state = IOT_NODE_BUSY;
  req->req_status = REQUEST_SENT;
  clock_gettime(CLOCK_MONOTONIC,&req->timestamp);

  json_decref(msg);
  return;
}

void try_do_some_work(void)
{
  struct event_list_node *iter = NULL; 
  /**
   * Scan through the iot nodes  
   */
  list_for_each_entry(iter,&events_list.head,head)
  {
    if(iter->event_type !=  IOT_EVENT)
    {
      continue;
    } 

    DEBUG("IOT NODE named %s has work state = %d and request_list empty"
          "is %d.",EVENT_TO_IOT_DATA(iter).client_name,
         EVENT_TO_IOT_DATA(iter).work_state,
         list_empty(&(EVENT_TO_IOT_DATA(iter).request_list)));

    if(EVENT_TO_IOT_DATA(iter).work_state == IOT_NODE_BUSY)
    {
      /**
       * Skip this node if the node is busy processing. 
       * Check the timestamp and delete if 5 seconds have expired since 
       * it went BUSY.
       */
      struct timespec tistmp; 
      double diff;
      struct request_node_s *req = 
            list_entry(EVENT_TO_IOT_DATA(iter).request_list.next,
                         typeof(*req), node);
      clock_gettime(CLOCK_MONOTONIC, &tistmp); 
      diff = BILLION * (tistmp.tv_sec - req->timestamp.tv_sec) +  
                    (tistmp.tv_nsec - req->timestamp.tv_nsec);
      DEBUG("req->timestamp.tv_sec = %d. Node busy since %lf  seconds",
					req->timestamp.tv_sec, diff);

      if(diff >= 20*BILLION)
      {
        DEBUG("Node might be in trouble. Deleting this request"); 
        delete_request(req);
        EVENT_TO_IOT_DATA(iter).work_state = IOT_NODE_FREE;
      }
      DEBUG("Skipping iot node %s as its BUSY",EVENT_TO_IOT_DATA(iter).client_name);
      continue;

    }
    if(list_empty(&(EVENT_TO_IOT_DATA(iter).request_list)))
    {
      DEBUG("Skipping iot node %s as its request list is empty",
          EVENT_TO_IOT_DATA(iter).client_name);
      continue;
    }
    else
    {
      /**
       * Send mwssage to the IOT NODE  
       */
      DEBUG("IOT node is neither busy nor is its list empty so need to send "
          "some message to it");
      sendNodeMessage(iter);
    }

  }
}
