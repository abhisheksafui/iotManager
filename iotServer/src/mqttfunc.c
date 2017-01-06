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
#include<event.h>
#include<global.h>
#include<string.h>
#include<jansson.h>
#include<mqttJsonIf.h>
#include<mqttdefs.h>
#include<mqttfunc.h>
#include<log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include<errno.h>
#include<json_utils.h>

void subscribe_topics(mqttConnection_t *cl, int qos)
{
  char buffer[256]; 
  mqttClientSubscribe(cl,topology_topic,qos); 
  snprintf(buffer,sizeof(buffer),"%s/%s",networkId,gatewayName);
  mqttClientSubscribe(cl,buffer,0);
}

struct mosquitto * mqttClientInit(mqttConnection_t *cl,
    bool cleanSession)
{

  char buffer[256]; 
  snprintf(buffer,sizeof(buffer),"%s/%s",networkId,gatewayName);
  cl->client = mosquitto_new(buffer,
                                      cleanSession,(void *)cl);
  if(cl->client == NULL)
  {
    ERROR("mosquitto_new error");

    return NULL;
  }
  if(cl->username && cl->password)
  {
    if(mosquitto_username_pw_set(cl->client,cl->username,cl->password)!=
        MOSQ_ERR_SUCCESS )
    {
      ERROR("Failed to set username password for mosquito client. reason");
    }
  }

  return cl->client; 
}
/* Register the callbacks that the mosquitto connection will use. */            
bool mqttSetCallbacks(struct mosquitto *m) {                                
  mosquitto_connect_callback_set(m, onConnect);                              
  mosquitto_publish_callback_set(m, onPublish);                              
  mosquitto_subscribe_callback_set(m, onSubscribe);                          
  mosquitto_message_callback_set(m, onMessage);                              
  return true;                                                                
}

void onSubscribe(struct mosquitto *m,
    void *context,
    int mid,int qos_count,
    const int *granted_qos)                
{                                                                               
  mqttConnection_t *cl = (mqttConnection_t *)context;                                        
  DEBUG("Subscribe succeeded for client %p\n",cl->client);                                              
}                                                                               

void onConnect(struct mosquitto *m, void* context, 
    int ret)                  
{                                                                               
  mqttConnection_t *cl = (mqttConnection_t *)context;                                        
  int rc;
  uint8_t count = 0; 
  switch(ret)
  {
    case 0: /*Success*/ 
      DEBUG("Successful connection for client %p\n",cl->client);                                            
      cl->connectionStatus = 1;     
      DEBUG("Subscribing to topics");                                                                     
      while(count < cl->subscribedTopicCount)
      {  
        if ((rc = mosquitto_subscribe(cl->client,NULL,cl->subscribedTopics[count],
                    cl->subscriptionQos[count])) != MOSQ_ERR_SUCCESS)
        {                                                                             
          STD_LOG("Failed to start subscribe, return code %d\n", rc);                  
          exit(-1);                                                                   
        }
        else
        {
          DEBUG("Started async subscribe for client %p to topic %s",
                    cl->client,
                    cl->subscribedTopics[count]);
        }
        count++;
      }
      broadcast_gateway_advertise(cl);

      break;
    default:
      DEBUG("Connection Unsucessful for client %p reason: %d",cl->client,
          ret);
      DEBUG("Trying to reconnect..");
      cl->connectionStatus = 0;                                                                 
      mqttClientConnect(cl);
      break;
  }  
}                                                                               

/**
 * @brief 
 *
 * @param context
 * @param topicName
 * @param topicLen
 * @param message
 *
 * @return 
 */
void onMessage(struct mosquitto *m,void *context,
    const struct mosquitto_message *message)
{                                                                               
  int i;                                                                      
  char* payloadptr;      
  mqttConnection_t *cl = context;    

  DEBUG("MQTT Message arrived. My PID %d" ,
      getpid());

  DEBUG("   topic: %s, qos: %d, retain: %d", message->topic,
      message->qos,
      message->retain);                                      
  DEBUG("   message: ");                                                     

  payloadptr = message->payload;                                              
  for(i=0; i<message->payloadlen; i++)                                        
  {                                                                           
    putchar(*payloadptr++);                                                 
  }                                                                           
  putchar('\n'); 
  /* First scan through subscription events*/
  
  process_mqtt_msg(cl,message->topic,message);   

}
void onPublish(struct mosquitto *m,void *context,int mid)
{
  DEBUG("Publish successfull for message id %d",mid);

}
void mqttClientSubscribe(mqttConnection_t *cl,char *topic,uint8_t qos)
{
  int ret_code;
 /*
  * Add the topic to the clients list of topics
  */ 
  cl->subscribedTopics[cl->subscribedTopicCount] = (char *)malloc(strlen(topic)+1);
  snprintf((cl->subscribedTopics)[cl->subscribedTopicCount], strlen(topic)+1,"%s",
                                     topic);
  (cl->subscriptionQos)[cl->subscribedTopicCount] = qos;
  cl->subscribedTopicCount++; 

  if(cl->connectionStatus == 1)
  {
    if ((ret_code = mosquitto_subscribe(cl->client,NULL,topic,
              qos)) != MOSQ_ERR_SUCCESS)
    {                                                                             
      STD_LOG("Failed to start subscribe, return code %d\n", ret_code);                  
      exit(-1);                                                                   
    }
    else
    {
      DEBUG("Started async subscribe for client %p to topic %s",
                cl->client,
                topic);
    }
  }
  else
  {
    DEBUG("Client is not connected so skipping subscription");
  }
} 

void mqttClientConnect(mqttConnection_t *cl)
{
  int ret_code;
  DEBUG("Trying to connect to  %p",cl->client);
  if ((ret_code = mosquitto_connect_async(cl->client,
                                          cl->serverUri,
                                          cl->serverPort,
                                          200))!= MOSQ_ERR_SUCCESS)    
  { 
    ERROR("Error in mosquitto_connect_async");
    cl->connectionStatus = 0;
  }
  else
  {
    DEBUG("Mqtt Client async connection started"); 
  }

}
#if 0
void *mqttConnectionRetry(void* arg)
{
  int ret_code;
  mqttConnection_t *cl = (mqttConnection_t *)arg;  
  while(cl->connectionStatus != 1)
  {
    DEBUG("Trying to connect to  %p",cl->client);
    if ((ret_code = MQTTAsync_connect(cl->client, &cl->connOpts)) != MQTTCLIENT_SUCCESS)    
    { 
      ERROR("Error in MQTTClinet_connect");
      cl->connectionStatus = 0;
    }
    else
    {
      DEBUG("Mqtt Client connected"); 
      cl->connectionStatus = 1;
    }
    sleep(10); 
  }
  /*
   * Need to Subscribe to all the topics this client had already/requested
   * to  subscribe
   */
  ret_code = MQTTClient_subscribeMany(cl->client,cl->subscribedTopicCount,
                                      cl->subscribedTopics,cl->subscriptionQos);
  if(ret_code != MQTTCLIENT_SUCCESS)
   DEBUG("Error in MQTTClient_subscribeMany: error code %d",ret_code);
  else
   DEBUG("Subscription Complete");

  return NULL; 
}

void start_mqtt_client_connect_thread(mqttConnection_t *cl)
{
  int ret = 0; 
  DEBUG("Starting Thread to retry connection to MQTT Broker");
  ret = pthread_create(&(cl->connectionThreadPid),
                         NULL, mqttConnectionRetry,cl);
  if(ret!=0)
  {
    ERROR("Error Creating thread");
    exit(1);
  }
                                                        
}
#endif
void mqttStartClientThread(mqttConnection_t *cl)
{

  int ret_code;
  ret_code = mosquitto_loop_start(cl->client); /* Start the thread for processing local client */
  if(ret_code != MOSQ_ERR_SUCCESS)
  {
    DEBUG("Error in starting broker client thread, reason: %d",ret_code);
    exit(-1);
  } 
  else
  {
    DEBUG("Created thread for processing broker client %p",cl->client);
  }
  mosquitto_reconnect_delay_set(cl->client,
                                            2,
                                            10,
                                            false);

}

int publishMqttMessage(char *message, const char *topic,uint8_t qos,
                       mqttConnection_t *cl)
{
  int ret = RC_OK;
  /*
   * I may skip publish if connetion status is not down
   * For now not skipping. 
   */
  if((ret = mosquitto_publish(cl->client, NULL, topic,
                          strlen(message),
                          message,0,false)) != MOSQ_ERR_SUCCESS)
  {
    ERROR("Error in publishing mqtt message, reason %d",ret);
    ret = RC_ERROR;
    goto EXIT;
  }

EXIT:
  return ret; 
}

static void appendRequestToWorkList(struct request_node_s *request,
                                    struct list_head *work_list,
                                    pthread_mutex_t *wrklst_mutex,
                                    int signal_fd)
{
  int rc;
  /**
   * Get the mutex lock and add the request to work_list and 
   * release the mutex lock
   */
  pthread_mutex_lock(wrklst_mutex);
  DEBUG("Got the lock to add request node %p to worklist %p",request,
                                                 work_list);
  list_add_tail(&request->node,work_list); 

  DEBUG("Added now releasing the lock");
  pthread_mutex_unlock(wrklst_mutex);
  /**
   * Now wite a byte to pipe to signal main thread of new work
   */
  rc = write(signal_fd,"WORK LOAD",strlen("WORK LOAD")+1);
  if(rc < 0)
    ERROR("Error writing work list signal to fd = %d",signal_fd);
}

int append_iot_node_caps(json_t *iot_json_object, struct event_list_node *node)
{
  json_t *caps_array = NULL;
  json_t *caps_obj = NULL;
  struct capability_node_s *iter=NULL; 
  int rc = RC_OK;

  caps_array = json_array();
  if(!caps_array)
  { 
    ERROR("Couldn't allocate new json array for caps_array");
    rc = RC_ERROR;
    goto ERROR;
  }

  list_for_each_entry(iter,&(EVENT_TO_IOT_DATA(node).caps_list),node)
  {
    caps_obj = json_object();
    if(!caps_obj)
    {
      rc = RC_ERROR;
      goto ERROR;
    }

    json_object_set_new(caps_obj,"capId",json_string(iter->cap_id));
    json_object_set_new(caps_obj,"capType",json_integer(iter->type));
    json_array_append_new(caps_array,caps_obj); 
  }

  json_object_set_new(iot_json_object,"caps",caps_array);

  return rc;

ERROR:
  json_decref(caps_array);
  return rc;
}



/**
* @brief 
*
* @param tmp
* @param len
*/
int append_iot_nodes(json_t *reply_msg)
{
  struct event_list_node *iter = NULL;
  json_t *iot_node_array;
  json_t *iot_node; 
  int rc = RC_OK;

  iot_node_array= json_array();
  if(!iot_node_array)
  { 
    ERROR("Couldn't allocate new json array for iot_node");
    rc = RC_ERROR;
    goto ERROR;
  }
  /**
   * Go through all the IOT_EVENT nodes and append their names 
   */
  list_for_each_entry(iter,&events_list.head,head)
  {
    if(iter->event_type == IOT_EVENT)
    { 
      iot_node = json_object();
      if(!iot_node) 
      {
        ERROR("Couldn't allocate new json_object");
        rc = RC_ERROR;
        goto ERROR;
      }
      json_object_set_new(iot_node,"iotNodeName",
                          json_string(NODE2IOT_DATA(iter).client_name));
      rc = append_iot_node_caps(iot_node,iter);

      if(rc == RC_ERROR)
      {
        json_decref(iot_node);
        goto ERROR;
      }

      json_array_append_new(iot_node_array, iot_node);
    }
  }

  json_object_set_new(reply_msg,"iotNodes",iot_node_array);

  return rc;
ERROR:
  json_decref(iot_node_array);
  return rc;
}

/**
* @brief 
*
* @param topicName
* @param message
*/
void process_mqtt_msg(mqttConnection_t *cl,char *topicName, 
                      const struct mosquitto_message *message)
{

  json_t *root;
  json_error_t error;
  const char *senderid;
  json_t *json_obj;
  json_t *reply_msg; 
  struct request_node_s *request;
  struct request_param_s *param;  
  uint8_t msg_type,ret;
  size_t index;
  json_t *value;
  char temp[BUFFSIZ];   

  /**
  * Compare message topic and the do action;
  * If topic is /topics then check that message is GET, If so then create
  * a message of the form
  * MSGTYPE:TOPICS\n
  * ADV\n
  * myclientid/IOT_NODEID,..,<msgid> and send
  */
  DEBUG("GOT Message in topic %s", topicName);
  root = json_loadb(message->payload, message->payloadlen,0, &error);
  
  if(!root)
  {
    ERROR("json_loads error: %d:%s",error.line,error.text);
  } 

  ret = JSON_CHK_OBJECT(root);
  if(!ret)
    goto error;

  if(!strncmp(topicName,topology_topic,strlen(topology_topic)))
  {
    json_obj = json_object_get(root,"msgtype");
    if(!json_obj)
      goto error;

    ret = JSON_CHK_INTEGER(json_obj);
    if(!ret)
      goto error;

    msg_type = (uint8_t)json_integer_value(json_obj);

    json_obj  = json_object_get(root,"senderid");
    if(!json_obj)
      goto error;
    ret = JSON_CHK_STRING(json_obj);
    if(!ret)
      goto error;

    senderid = (const char *) json_string_value(json_obj);

      
    if(msg_type ==  MQTT_MSG_TYPE_GET_TOPO)
    {
      /**
       * Message type GET_TOPO .
       * { "msgtype": 5 , "senderid" : "APPNAME" }
       */
      reply_msg = json_object();
      if(reply_msg == NULL)
      {
        ERROR("Could not get new json object");
      }
      
      json_object_set_new(reply_msg,"msgtype",
                              json_integer(MQTT_MSG_TYPE_GET_TOPO_RESP));
      json_object_set_new(reply_msg,"gw_name",
                              json_string(gatewayName));

      ret = append_iot_nodes(reply_msg); 
      if(ret == RC_ERROR)
      {
        json_decref(reply_msg);
        goto error;
      } 
      char *msg; 
      msg = json_dumps(reply_msg,JSON_INDENT(10));
  
      snprintf(temp,BUFFSIZ,"%s/%s",networkId,senderid);
      DEBUG("ABout to send MQTT message to %s, msg is %s",temp,msg);
      if(RC_OK != publishMqttMessage(msg,temp,0,cl))
      {
        ERROR("Error in MQTT Publish api call");
        //send error reply to senderid
        goto error; 
      }
      free(msg);
      json_decref(reply_msg);
    }
    json_decref(root);

  }
  else
  {
    json_obj = json_object_get(root,"msgtype");

    ret = JSON_CHK_INTEGER(json_obj);
    if(!ret)
      goto error;
    msg_type = (uint8_t)json_integer_value(json_obj);

    /**
     * Allocate the request structure that will be added to shared work_list 
     * with main thread. Locking needed 
     */
    request = (struct request_node_s *) 
              calloc (1,sizeof(struct request_node_s));    
    
    request->req_status  = REQUEST_PENDING;

    request->req_data = (struct mqtt_request_s *)
                       calloc(1,sizeof(struct mqtt_request_s));
    INIT_LIST_HEAD(&((struct mqtt_request_s *)(request->req_data))->params);                                                      

    json_obj  = json_object_get(root,"senderid");
    ret = JSON_CHK_STRING(json_obj);
    if(!ret)
    {
      free(request);
      goto error;
    }
    DEBUG("Mqtt request received from %s",json_string_value(json_obj));
    /**
     * Store the sender name in the request control block. main thread 
     * will reply to this client's topic  
     */
    ((struct mqtt_request_s *)(request->req_data))->brokerConnection = cl;

    strncpy(((struct mqtt_request_s *)request->req_data)->req_sender_name,
              json_string_value(json_obj),
              strlen(json_string_value(json_obj))+1); 

    json_obj = json_object_get(root,"nodename");
    ret = JSON_CHK_STRING(json_obj);
    if(!ret)
    {
      free(request);
      goto error;
    }
    DEBUG("Mqtt request received for iot node named %s",
                  json_string_value(json_obj));

    strncpy(((struct mqtt_request_s *)request->req_data)->iotnode_name,
                json_string_value(json_obj),
                strlen(json_string_value(json_obj))+1);

    json_obj = json_object_get(root,"nodeid");

    if(json_obj)
    {
      ret = JSON_CHK_INTEGER(json_obj);
      if(!ret)
        goto error;
      ((struct mqtt_request_s *)request->req_data)->iotnode_id = (uint8_t)json_integer_value(json_obj);
    }
    else
    {
      DEBUG("nodeid not sent setting to 0");
      ((struct mqtt_request_s *)request->req_data)->iotnode_id = 0;
    }
    /*
     * Store the connection in the request structure 
     */
    switch(msg_type)
    {
      case MQTT_MSG_TYPE_GET: 
        /**
         * Store the request type  
         */ 
      request->req_type = GET_REQUEST; 

      DEBUG("Get request received in topic %s",
           topicName);

      json_obj  = json_object_get(root,"items");
      ret = JSON_CHK_ARRAY(json_obj);
      if(!ret)
        goto error;
      /* array is a JSON array */
      DEBUG("ARRAY items START:");
      json_array_foreach(json_obj, index, value) {

            /* block of code that uses index and value */
        DEBUG("id = %s",json_string_value(json_object_get(value, "id")));
        param = (struct request_param_s *)
                calloc(1,sizeof(struct request_param_s)); 
        
        strncpy(param->id,
                  json_string_value(json_object_get(value, "id")),
                  strlen(json_string_value(json_object_get(value,"id")))+1);
        DEBUG("Adding param node %p to request node %p",param,request);
        list_add(&param->node,&(((struct mqtt_request_s *)(request->req_data))->params));

      }
      DEBUG("ARRAY items END");
      /**
       * Now get the mutex lock and add the request to work_list and 
       * release the mutex lock
       */
      pthread_mutex_lock(&work_list_mutex);
      DEBUG("Got the lock to add request node %p to worklist %p",request,
                                                     &work_list);
      list_add(&request->node,&work_list); 
      DEBUG("Added now releasing the lock");
      pthread_mutex_unlock(&work_list_mutex);
      /**
       * Now wite a byte to pipe to signal main thread of new work
       */
      write(pipe_fd[1],"WORK LOAD",strlen("WORK LOAD")+1);
      break;
    case MQTT_MSG_TYPE_SET:
      request->req_type = SET_REQUEST; 

      DEBUG("SET request received in topic %s",topicName);
      json_obj  = json_object_get(root,"items");
      ret = JSON_CHK_ARRAY(json_obj);
      if(!ret)
      {
        ERROR("items should be an array of requested items");          
        goto error;
      }
      /* array is a JSON array */
      DEBUG("SET ARRAY items START:");
      json_array_foreach(json_obj, index, value) {

        /* block of code that uses index and value */
        DEBUG("id = %s",json_string_value(json_object_get(value, "id")));
        DEBUG("value = %s",json_string_value(json_object_get(value, "value")));
        param = (struct request_param_s *)
                calloc(1,sizeof(struct request_param_s)); 
        
        strncpy(param->id,
                  json_string_value(json_object_get(value, "id")),
                  strlen(json_string_value(json_object_get(value,"id")))+1);

        param->args = malloc(strlen(json_string_value(json_object_get(value, 
                      "value"))));
       
        strncpy((char *)param->args,
                json_string_value(json_object_get(value,"value")),
                strlen(json_string_value(json_object_get(value,"value")))+1); 

        DEBUG("Adding param node %p to request node %p",param,request);
        list_add(&param->node,&(((struct mqtt_request_s *)(request->req_data))->params));

      }
      DEBUG("SET ARRAY items END");

      appendRequestToWorkList(request,&work_list,&work_list_mutex,pipe_fd[1]);
      break;
    }
    json_decref(root); 
  }
return;
error:
json_decref(root); 
return;
}
