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
#include<jansson.h>
#include<mqttdefs.h>
#include<mqttfunc.h>
#include<log.h>
#include<global.h>
#include<schedule.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include<errno.h>
#include<load_config.h>

void subscribe_topics(mqttConnection_t *cl, int qos)
{
  char buffer[256]; 
  snprintf(buffer,sizeof(buffer),"%s/%s/%s",networkId,gatewayName,"Scheduler");
  mqttClientSubscribe(cl,buffer,qos);
  snprintf(buffer,sizeof(buffer),"%s/%s",networkId,"broadcast");
  mqttClientSubscribe(cl,buffer,qos);
}

struct mosquitto * mqttClientInit(mqttConnection_t *cl,
    bool cleanSession)
{

  char buffer[256]; 
  snprintf(buffer,sizeof(buffer),"%s/%s/%s",networkId,gatewayName,"Scheduler");
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

  INIT_LIST_HEAD(&cl->subscribed_topics);

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
  subscriptionTopic_t *topic_node;
  int ret_code;
  switch(ret)
  {
    case 0: /*Success*/ 
      DEBUG("Successful connection for client %p\n",cl->client);                                            
      cl->connectionStatus = 1;     
      DEBUG("Subscribing to topics");                                                                     

      list_for_each_entry(topic_node,&cl->subscribed_topics,node){
        DEBUG("--Subscribing to topic %s",topic_node->topic);
        if ((ret_code = mosquitto_subscribe(cl->client,NULL,topic_node->topic,                  
                topic_node->qos)) != MOSQ_ERR_SUCCESS)                                        
        {                                                                           
          printf("Failed to start subscribe, return code %d\n", ret_code);          
          exit(-1);                                                                 
        }                                                                           
        else                                                                        
        {                                                                           
          DEBUG("Started async subscribe for client %p to topic %s",                
              cl->client,                                                     
              topic_node->topic);                                                         
        }  
      }
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
  process_subscriptions(cl,message->topic,message);
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
  struct subscriptionTopic_s *topic_node = (struct subscriptionTopic_s *)
    calloc(1,sizeof(struct subscriptionTopic_s));

  topic_node->topic = (char *) calloc(1,strlen(topic)+1);
  snprintf(topic_node->topic,strlen(topic)+1,"%s",topic);
  topic_node->qos = qos;

  list_add_tail(&topic_node->node, &cl->subscribed_topics);

  if(cl->connectionStatus == 1){
    if ((ret_code = mosquitto_subscribe(cl->client,NULL,topic,                  
            qos)) != MOSQ_ERR_SUCCESS)                                        
    {                                                                           
      printf("Failed to start subscribe, return code %d\n", ret_code);          
      exit(-1);                                                                 
    }                                                                           
    else                                                                        
    {                                                                           
      DEBUG("Started async subscribe for client %p to topic %s",                
          cl->client,                                                     
          topic);                                                         
    }  
  }else{
    DEBUG("Client %p is not connected yet, skipping subscription",
        cl);
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
      log_err("Error in MQTTClinet_connect");
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
    log_err("Error Creating thread");
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

int publishMqttMessage(char *message, const char *topic,
    mqttConnection_t *cl)
{
  int ret = RC_OK;
  /*
   * I may skip publish if connetion status is not down
   * For now not skipping. 
   */
  if((cl->connectionStatus==1) && (ret = mosquitto_publish(cl->client, NULL, topic,
          strlen(message),
          message,2,false)) != MOSQ_ERR_SUCCESS)
  {
    ERROR("Error in publishing mqtt message, reason %d",ret);
    ret = RC_ERROR;
    goto EXIT;
  }

EXIT:
  return ret; 
}

void appendTaskToTaskList(struct task_s *task,
    struct list_head *tlist, 
    pthread_mutex_t *mu)
{
  pthread_mutex_lock(mu);

  list_add_tail(&task->node,tlist); 
  task->id = ++task_count;
  DEBUG("Added task number %d to task list",task->id);
  pthread_mutex_unlock(mu); 

}

void sendMqttSetRequest(void *arg)
{
  struct task_s *task = (struct task_s *) arg; 
  int ret = RC_ERROR; 
  json_t *root, *caps_array, *json_obj; 

  if(arg == NULL)
    return;
  root = json_object();
  json_object_set_new(root,"msgtype",
      json_integer(MQTT_MSG_TYPE_SET));
  json_object_set_new(root,"nodename",
      json_string(task->iot_node_name));
  json_object_set_new(root,"senderid",
      json_string(task->req_sender_name));

  caps_array = json_array();

  json_obj = json_object();
  json_object_set_new(json_obj,"id",
      json_string(task->capability_name)); 

  if(task->type == SWITCH_OFF_TIMER)
  {
    json_object_set_new(json_obj,"value",
        json_string("OFF"));
  }else
  {

  }
  json_array_append_new(caps_array,json_obj); 
  json_object_set_new(root,"items",caps_array);

  char *msg = json_dumps(root,JSON_INDENT(1));
  DEBUG("About to send set request : \n\n%s\n",msg);

  /* Send topic needs to be modified to /NETWORK_ID/GATEWAY_NAME 
   * as it is considered that all devices talking to each other should share the 
   * same network */ 
  char buff[256]; 
  snprintf(buff,sizeof(buff),"%s/%s",networkId,gatewayName);
  ret = publishMqttMessage(msg,buff,&local_client);
  if(ret != RC_OK)
  {
    ERROR("Error in mosquitto publish");
  }
  free(msg);
  json_decref(root);
}

struct switch_event_s * allocSwitchOnEvent(char *nodename,
    char *capability_name,
    char *gateway_name)
{
  struct switch_event_s *event = (struct switch_event_s *)
    calloc(1,sizeof(struct switch_event_s));
  strncpy(event->iot_node_name,nodename,MAX_IOTNODE_NAME_LEN);
  strncpy(event->capability_name,capability_name,MAX_CAPABILITY_NAME_LEN);
  strncpy(event->gateway_name,gateway_name,MAX_GATEWAY_NAME_LEN);

  event->value = (char *)calloc(1,strlen("ON")+1);
  strncpy(event->value,"ON",strlen("ON")+1);
  return event;   
}

void deleteSwitchOnEvent(struct switch_event_s *event)
{
  if(event == NULL)
    return;
  if(event->value != NULL)
  {
    free(event->value);
  }
  free(event);

  return;
}
void deleteEventSubscription(struct event_subscription_s *subsc)
{
  switch(subsc->type)
  {
    case SWITCH_ON_EVENT:
      deleteSwitchOnEvent((struct switch_event_s *)subsc->event); 
      break;

  }
}
struct event_subscription_s *subscribeEvent(void *event,SUBSC_TYPE type,
    void (*action)(void *arg), void *arg)
{
  struct event_subscription_s *subscription = (struct event_subscription_s *)
    calloc(1,sizeof(struct event_subscription_s));
  subscription->type = type;
  subscription->event  =event;
  subscription->action = action;
  subscription->action_data = arg;
  // TODO Lock mutex 
  list_add_tail(&subscription->node,&event_subsc_list);
  DEBUG("Adding subscription type = %d to subscription list",type);

  return subscription;
}

void activateSwitchOffTimer(void *arg)
{
  struct task_s *task = (struct task_s *)arg;
  if(task->oper_state == ACTIVE)
    return;
  task->oper_state = ACTIVE;
  time(&task->timer_data.timeval);
  task->timer_data.timeval +=  task->interval;
  DEBUG("Activated timer type %d, for iotnode %s, switchname %s, expire time set to %d",
      task->type, task->iot_node_name, task->capability_name,
    task->timer_data.timeval);  
}

void deActivateSwitchOffTimer(void *arg)
{
  struct task_s *task = (struct task_s *)arg;
  task->oper_state = INACTIVE;
  DEBUG("De-activated timer type %d, for iotnode %s, switchname %s",
      task->type, task->iot_node_name, task->capability_name);  
}

void saveTimerSetRequest(const struct mosquitto_message *m, int id)
{
  DIR *sd;
  char filepath[MAX_FILE_PATH_LEN]; 
  
  sd = opendir(save_dir);
  if((sd == NULL) && (ENOENT == errno))
  {
    DEBUG("save directory does not exist. Creating it..");
    if(mkdir(save_dir,0700) == -1)
    {
      ERROR("Error creating save directory. Cannot save request");
      return;
    }
    else
    {
      DEBUG("Save directory created : %s",save_dir);
    }
  }
  else
  {
    DEBUG("Directory Exists");
    closedir(sd);
  }
  snprintf(filepath,MAX_FILE_PATH_LEN,"%s/%u",save_dir,id);
  FILE *file = fopen(filepath,"w+");
  fwrite(m->payload,m->payloadlen,1,file);
  fclose(file);

}

int process_json_msg(json_t *root)
{
  int ret = RC_ERROR;
  uint8_t msg_type; 
  json_t *json_obj;
  struct task_s *task;
  uint32_t interval = 0 ; 

  json_obj = json_object_get(root,"msgtype");
  if(!json_obj)
  {
    ERROR("Error getting msgtype object");
    goto error; 
  }
  msg_type = (uint8_t)json_integer_value(json_obj);

  switch(msg_type)
  {
    case MQTT_MSG_TYPE_TIMER_SET_REQ:
      /* Convert the message to task and store in list*/ 
      task = (struct task_s *) calloc(1, 
          sizeof(struct task_s));
      json_obj = json_object_get(root,"reqSenderName");
      if(!json_obj){
        ERROR("Error getting json object");
        json_decref(root);
        free(task); 
      }
      ret = JSON_CHK_STRING(json_obj);
      if(!ret)
      {
        json_decref(root);
        free(task);
        goto error; 
      }
      snprintf(task->req_sender_name,MAX_MQTTCLIENT_NAME_LEN,"%s",
          json_string_value(json_obj)); 

      json_obj = json_object_get(root,"iotNodeName");
      ret = JSON_CHK_STRING(json_obj);
      if(!ret)
      {
        json_decref(root);
        free(task);
        goto error; 
      }
      snprintf(task->iot_node_name,MAX_IOTNODE_NAME_LEN,"%s",
          json_string_value(json_obj)); 

      json_obj = json_object_get(root,"iotCapName");
      ret = JSON_CHK_STRING(json_obj);
      if(!ret)
      {
        json_decref(root);
        free(task);
        goto error; 
      }
      snprintf(task->capability_name,MAX_CAPABILITY_NAME_LEN,"%s",
          json_string_value(json_obj)); 

      json_obj = json_object_get(root,"iotGatewayName");
      ret = JSON_CHK_STRING(json_obj);
      if(!ret)
      {
        json_decref(root);
        free(task);
        goto error; 
      }
      snprintf(task->gateway_name,MAX_GATEWAY_NAME_LEN,"%s",
          json_string_value(json_obj)); 

      /* Save the timer persistance integer  value */ 
      json_obj = json_object_get(root,"timerPersist");
      task->persist = json_integer_value(json_obj); 

      json_obj = json_object_get(root,"timerType");
      ret = JSON_CHK_INTEGER(json_obj);
      if(!ret)
      {
        json_decref(root);
        free(task);
        goto error; 
      }
      task->type = json_integer_value(json_obj);
      if(task->type == SWITCH_OFF_TIMER)
      {
        /* It will be set active when we receive 
         * broadcast message that the switch is on*/
        task->oper_state=INACTIVE; 

        json_obj = json_object_get(root,"time");
        ret = JSON_CHK_STRING(json_obj);
        if(!ret)
        {
          json_decref(root);
          free(task);
          goto error; 
        }
        interval = strtol(json_string_value(json_obj),NULL,10);
        if(interval == 0 )
        {
          ERROR("ERROR in iimer value");
          json_decref(root);
          free(task);
          goto error; 
        }
        //time(&time_val);
        //time_val += interval; 
        task->interval = interval;
        /* register the function to be called on timer expire */
        task->cmd = sendMqttSetRequest;
        task->cmd_arg = (void *)task; 
        /* subscribe for on event */ 
        struct switch_event_s *event = allocSwitchOnEvent(task->iot_node_name,
            task->capability_name,
            task->gateway_name); 
        task->subscription = subscribeEvent(event,SWITCH_ON_EVENT,
            activateSwitchOffTimer,
            task);
      }
      else if(task->type == ON_TIMER)
      {

      }  
      else if(task->type == OFF_TIMER)
      {

      } 

      appendTaskToTaskList(task,&task_list,&task_list_mutex);
     // saveTimerSetRequest(message,task->id);
      //json_object_set_new(root,"id",json_integer(task->id));
      //json_object_set_new(root,"msgtype",json_integer(MQTT_MSG_TYPE_TIMER_SET_RESP));
      //char *msg = json_dumps(root,0);
      //if(publishMqttMessage(msg,task->gateway_name,cl) != RC_OK)
      //{ 
      //  ERROR("Error replying to task set request");
      //}
      //free(msg);
      break; 
  }
error: 
  return ret;
}
void respond_to_delete_request(json_t *root, mqttConnection_t *cl)
{
  json_t *jobj;
  const char *req_sender_name; 
  /* Just change the message type and reply */
  json_object_set_new(root,"msgtype",json_integer(MQTT_MSG_TYPE_TIMER_DEL_RESP));
  
  jobj = json_object_get(root,"reqSenderName");
  req_sender_name = json_string_value(jobj);
  if(req_sender_name == NULL)
    return; 
  char *msg = json_dumps(root,1);
  DEBUG("Sending timer delete response -> to topic %s", req_sender_name);
  PRINT("%s", msg);
  
  char buff[256]; 
  snprintf(buff,sizeof(buff),"%s/%s",networkId,req_sender_name);
  if(publishMqttMessage(msg,buff,cl) != RC_OK)
  { 
    ERROR("Error replying to message get request");
  }
  free(msg);
  
}
void respond_to_get_request(json_t *root, mqttConnection_t *cl)
{
  const char *req_node_name;
  const char *req_gateway_name;
  const char *req_capability_id; 
  const char *req_sender_name; 
  struct task_s *task_iter;
  char timer_value_string[50]; 
  json_t *jobj; 
  json_t *jarray;

  jobj = json_object_get(root,"iotNodeName");
  req_node_name = json_string_value(jobj);
  jobj = json_object_get(root,"iotGatewayName");
  req_gateway_name = json_string_value(jobj);
  jobj = json_object_get(root,"iotCapName");
  req_capability_id = json_string_value(jobj);
  jobj = json_object_get(root,"reqSenderName");
  req_sender_name = json_string_value(jobj);
  if(req_sender_name == NULL)
    return; 

  jarray = json_array(); 
  
  json_object_set_new(root,"msgtype",json_integer(MQTT_MSG_TYPE_TIMER_GET_RESP));

  list_for_each_entry(task_iter,&task_list,node)
  {

    if((!req_node_name || !strcmp(req_node_name,task_iter->iot_node_name)) &&
       (!req_gateway_name || !strcmp(req_gateway_name,task_iter->gateway_name)) && 
       (!req_capability_id || !strcmp(req_capability_id,
                                      task_iter->capability_name)))
    {
      jobj = json_object();
      json_object_set_new(jobj,"timerType",json_integer(task_iter->type));
      json_object_set_new(jobj,"id",json_integer(task_iter->id));
      if(SWITCH_OFF_TIMER == task_iter->type)
      {
        snprintf(timer_value_string,sizeof(timer_value_string),"%d Seconds",
            task_iter->interval);
        json_object_set_new(jobj,"timerValue",
                              json_string(timer_value_string));
      }
      else
      {
        //TODO 
      }

      json_object_set_new(jobj,"timerPersist",
                            json_integer(task_iter->persist));
      json_object_set_new(jobj,"timerOperState",
                            json_integer(task_iter->oper_state));
      json_array_append_new(jarray,jobj);
    }
  }

  json_object_set_new(root,"timers",jarray);
  char *msg = json_dumps(root,1);
  PRINT("%s", msg);
  
  char buff[256]; 
  snprintf(buff,sizeof(buff),"%s/%s",networkId,req_sender_name);
  DEBUG("Sending timer get response -> to topic %s", buff);
  if(publishMqttMessage(msg,buff,cl) != RC_OK)
  { 
    ERROR("Error replying to message get request");
  }
  free(msg);
}

void delete_task(struct task_s *task)
{
  char filename[100];
  if(task == NULL)
    return;
  /* Delete the subscription event that this timer is associated with*/
  if(task->subscription != NULL)
  {
    /* first get the node out of list */ 
    list_del(&task->subscription->node);
    free(task->subscription);
  }
  free(task);
              /* delete the saved db file */
              snprintf(filename,sizeof(filename),"%s/%d",save_dir,
                  task->id);
              remove(filename);
}

void perform_timer_delete(json_t *root,mqttConnection_t *cl)
{
  json_t *jobj; 
  int ret;
  int timer_id;
  struct task_s *task_iter,*temp; 
  jobj = json_object_get(root,"timerId");
  if(jobj == NULL)
  {
    ERROR("Delete timer request does not contain timerId");
    return;
  }
  ret = JSON_CHK_INTEGER(jobj);
  if(!ret)
  {
    ERROR("Delete timer request timerId not string");
    return;
  }
  timer_id = json_integer_value(jobj);

  /*Search for the timer and delete it after taking lock*/
  list_for_each_entry_safe(task_iter,temp,&task_list,node)
  {
    if(task_iter->id == timer_id)
    {
      DEBUG("Match found for timer %d to delete",timer_id);
    }
    GET_SHARED_DATA_LOCK();
    list_del(&task_iter->node);
    delete_task(task_iter);
    RELEASE_SHARED_DATA_LOCK();
  }
  respond_to_delete_request(root,cl);
}

void process_mqtt_msg(mqttConnection_t *cl,char *topicName, 
    const struct mosquitto_message *message)
{
  json_t *root,*json_obj; 
  json_error_t error; 
  int ret; 
  uint8_t msg_type;
  //time_t time_val;
  uint32_t interval = 0 ; 
  struct task_s *task;
  root = json_loadb(message->payload, message->payloadlen, 0, &error); 

  if(root == NULL)
  {
    ERROR("json load error");    
  }

  ret = JSON_CHK_OBJECT(root);
  if(!ret)
    goto error;

  json_obj = json_object_get(root,"msgtype");
  if(!json_obj)
  {
    ERROR("Error getting msgtype object");
    goto error; 
  }
  msg_type = (uint8_t)json_integer_value(json_obj);

  switch(msg_type)
  {
    case MQTT_MSG_TYPE_TIMER_SET_REQ:
      /* Convert the message to task and store in list*/ 
      task = (struct task_s *) calloc(1, 
          sizeof(struct task_s));
      json_obj = json_object_get(root,"reqSenderName");
      if(!json_obj){
        ERROR("Error getting json object");
        json_decref(root);
        free(task); 
      }
      ret = JSON_CHK_STRING(json_obj);
      if(!ret)
      {
        json_decref(root);
        free(task);
        goto error; 
      }
      snprintf(task->req_sender_name,MAX_MQTTCLIENT_NAME_LEN,"%s",
          json_string_value(json_obj)); 

      json_obj = json_object_get(root,"iotNodeName");
      ret = JSON_CHK_STRING(json_obj);
      if(!ret)
      {
        json_decref(root);
        free(task);
        goto error; 
      }
      snprintf(task->iot_node_name,MAX_IOTNODE_NAME_LEN,"%s",
          json_string_value(json_obj)); 

      json_obj = json_object_get(root,"iotCapName");
      ret = JSON_CHK_STRING(json_obj);
      if(!ret)
      {
        json_decref(root);
        free(task);
        goto error; 
      }

      snprintf(task->capability_name,MAX_CAPABILITY_NAME_LEN,"%s",
          json_string_value(json_obj)); 


      json_obj = json_object_get(root,"timerPersist");
      task->persist = json_integer_value(json_obj); 
      
      DEBUG("Timer Persistance on set req = %d", task->persist);

      json_obj = json_object_get(root,"iotGatewayName");
      ret = JSON_CHK_STRING(json_obj);
      if(!ret)
      {
        json_decref(root);
        free(task);
        goto error; 
      }
      snprintf(task->gateway_name,MAX_GATEWAY_NAME_LEN,"%s",
          json_string_value(json_obj)); 

      json_obj = json_object_get(root,"timerType");
      ret = JSON_CHK_INTEGER(json_obj);
      if(!ret)
      {
        json_decref(root);
        free(task);
        goto error; 
      }
      task->type = json_integer_value(json_obj);
      if(task->type == SWITCH_OFF_TIMER)
      {
        /* It will be set active wen we receive 
         * broadcast message that the switch is on*/
        task->oper_state=INACTIVE; 

        json_obj = json_object_get(root,"time");
        ret = JSON_CHK_STRING(json_obj);
        if(!ret)
        {
          json_decref(root);
          free(task);
          goto error; 
        }
        interval = strtol(json_string_value(json_obj),NULL,10);
        if(interval == 0 )
        {
          ERROR("ERROR in iimer value");
          json_decref(root);
          free(task);
          goto error; 
        }
        //time(&time_val);
        //time_val += interval; 
        task->interval = interval;
        /* register the function to be called on timer expire */
        task->cmd = sendMqttSetRequest;
        task->cmd_arg = (void *)task; 
        /* subscribe for on event */ 
        struct switch_event_s *event = allocSwitchOnEvent(task->iot_node_name,
            task->capability_name,
            task->gateway_name); 
        task->subscription = subscribeEvent(event,SWITCH_ON_EVENT,
            activateSwitchOffTimer,
            task);
      }
      else if(task->type == ON_TIMER)
      {

      }  
      else if(task->type == OFF_TIMER)
      {

      } 

      appendTaskToTaskList(task,&task_list,&task_list_mutex);
      saveTimerSetRequest(message,task->id);
      json_object_set_new(root,"id",json_integer(task->id));
      json_object_set_new(root,"msgtype",json_integer(MQTT_MSG_TYPE_TIMER_SET_RESP));
      json_object_set_new(root,"timerOperState",json_integer(task->oper_state));
      //task->oper_state = json_integer_value(json_obj); 

      char *msg = json_dumps(root,0);
      char buff[256]; 
      snprintf(buff,sizeof(buff),"%s/%s",networkId,task->req_sender_name);
      if(publishMqttMessage(msg,buff,cl) != RC_OK)
      { 
        ERROR("Error replying to task set request");
      }
      free(msg);
      break; 

    case MQTT_MSG_TYPE_TIMER_GET_REQ:
      respond_to_get_request(root,cl);
      break;
    case MQTT_MSG_TYPE_TIMER_DEL_REQ:
      perform_timer_delete(root,cl);
      break;
  }
  json_decref(root);
error: 
  return; 
}
int matchIotNodeName(json_t *root, char *name)
{

  int ret = 1;
  const char *json_node_name;
  json_t *json_obj=NULL; 
  json_obj = json_object_get(root,"nodename");
  if(!json_obj)
  {
    ERROR("Error getting nodename object");
    ret = 0;
    goto EXIT_LABEL;
  }
  ret = JSON_CHK_STRING(json_obj);
  if(!ret)
  {
    ERROR("Error: Json value is not string");
    ret = 0;
    goto EXIT_LABEL;
  } 
  json_node_name = json_string_value(json_obj);

  DEBUG("parsed nodename = %s name = %s",json_node_name,name);
  return(strcmp(json_node_name,name) == 0);

EXIT_LABEL:
  return ret;
}
int matchCapIdValue(json_t *root, char *capId, char *value)
{
  int ret = 0;
  json_t *json_array_items=NULL, *json_obj_id= NULL, *json_obj_value=NULL;;
  json_t *item = NULL;
  size_t index; 
  json_array_items = json_object_get(root,"items");
  
  json_array_foreach(json_array_items, index, item)
  {
    json_obj_id = json_object_get(item,"id");
    json_obj_value = json_object_get(item,"value");
    if((strcmp(capId,json_string_value(json_obj_id)) == 0) &&
        (strcmp(value,json_string_value(json_obj_value))==0))
    {
      DEBUG("Matched capid = %s and value = %s", capId,value);
      ret = 1; 
      break;
    } 
    DEBUG("parsed capId = %s id = %s",json_obj_id,capId);
    DEBUG("parsed value = %s value = %s",json_obj_value,value);
  } 

  return ret;

}
int matchMessageType(json_t *root, uint8_t type)
{

  int ret = 1;
  json_t *json_obj=NULL;
  uint8_t msg_type; 
  json_obj = json_object_get(root,"msgtype");
  if(!json_obj)
  {
    ERROR("Error getting msgtype object");
    ret = 0;
    goto EXIT_LABEL;
  }
  ret = JSON_CHK_INTEGER(json_obj);
  if(!ret)
  {
    ERROR("Error: Json value is not integer");
    ret = 0;
    goto EXIT_LABEL;
  } 
  msg_type = (uint8_t)json_integer_value(json_obj);

  DEBUG("parsed msg_type = %d type = %d",msg_type,type);
  return(msg_type == type);

EXIT_LABEL:
  return ret;
}


void process_subscriptions(mqttConnection_t *cl,char *topicName, 
    const struct mosquitto_message *message)
{
  struct event_subscription_s *subscription_node; 
  json_error_t error; 
  int ret;
  struct switch_event_s *sw_event; 
  json_t *root = json_loadb(message->payload, message->payloadlen, 0, &error); 
  if(root == NULL)
  {
    ERROR("json load error"); 
    goto error;   
  }
  ret = JSON_CHK_OBJECT(root);
  if(!ret)
    goto error;
  
  DEBUG("About to loop through subscriptions");

  list_for_each_entry(subscription_node,&event_subsc_list,node)
  {
    DEBUG("Found entry subscrtion type = %d", subscription_node->type);
    switch(subscription_node->type)
    {
      case SWITCH_ON_EVENT: 
        sw_event = (struct switch_event_s *)
          subscription_node->event;

        if((matchMessageType(root,MQTT_MSG_TYPE_SET_RESP)||
              matchMessageType(root,MQTT_MSG_TYPE_NOTIF_BROADCAST))&& 
            matchIotNodeName(root,sw_event->iot_node_name) && 
            matchCapIdValue(root,sw_event->capability_name,sw_event->value))
        {
          DEBUG("Subscription event matched. About to execute action");
          subscription_node->action(subscription_node->action_data);
        }

        break; 
    }
  }
error: 
  return;
}
