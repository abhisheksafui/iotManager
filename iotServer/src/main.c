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
#include<sys/socket.h>
#include<sys/types.h>
#include<pthread.h>
#include<unistd.h>
#include<stdlib.h>
#include<global.h>
#include<event.h>
#include<time.h>
#include<sys/timerfd.h>
//#include<MQTTClient.h>
#include<sys/time.h>
#include<unistd.h>
#include<sys/select.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<jansson.h>
#include<pthread.h>
#include<mqttJsonIf.h>
#include<mqttdefs.h>
#include<mqttfunc.h>
#include<workqueue.h>
#include<node.h>
#include<assert.h>
#include<json_utils.h>
#include<getopt.h>
#include<log.h>
#include<string.h>
#include <unistd.h>
#include <fcntl.h>


/**
 * START Declaration of Global Variables  
 */
struct event_list_s events_list;
int max_fd; 
fd_set read_fd_set;

char networkId[MAX_NETWORK_ID_LEN];
char gatewayName[MAX_GATEWAY_NAME_LEN];
uint16_t gatewayPort; 
mqttConnection_t local_client;
mqttConnection_t remote_client;
char notif_broadcast_topic[MAX_NETWORK_ID_LEN + 
                            MAX_GATEWAY_NAME_LEN + 20] ; 
char topology_topic[MAX_NETWORK_ID_LEN + MAX_GATEWAY_NAME_LEN + 20]; 
/**
 * Pipe is used for signaling from mqtt receive thread to main thread 
 */
int pipe_fd[2];

/**
 * WOrk will be added for each message to be sent to IOT node, to this list.
 * For example Hello message will be added. Main thread or some ther thread will
 * process work from this queue 
 */
pthread_mutex_t work_list_mutex = PTHREAD_MUTEX_INITIALIZER; 
LIST_HEAD(work_list);


uint8_t mqtt_msg_count=0;
int global_work_scan_timer_fd; 
/**
 * END Declaration of Global Variables 
 */

void print_pthread_id(pthread_t id)
{
    size_t i;
    for (i = sizeof(i); i; --i)
      STD_LOG("%02x", *(((unsigned char*) &id) + i - 1));
}



#if 0
void fill_caps(struct capability_node_s *node,char **msg)
{
  char *p = *msg;
  *msg = strchr(p,',')+1;
  do
  {
    p = strtok(p,"=,");
    if(!strcmp(p,"TYPE"))
    {
      /**
       * Store the type of capabilty 
       */
      p = strtok(NULL,",");
      if(!strcmp(p,"SWITCH"))
        node->type = CAP_SWITCH;
      else if(!strcmp(p,"LED_MATRIX"))
        node->type = CAP_LED_MATRIX_DISPLAY; 
      else if(!strcmp(p,"TEMP"))
        node->type = CAP_TEMP_SENSOR; 
      else if(!strcmp(p,"POT"))
        node->type = CAP_POT; 
        
    }
    if(!strcmp(p,"ID"))
    {
      p = strtok(NULL,",");
      snSTD_LOG(node->cap_id,MAX_CAP_ID_LEN,p);
      DEBUG("Adding cap_id %s",node->cap_id);
    }
  }while((p=strtok(NULL,"="))!= NULL);

}
#endif

void read_iot_network_id(json_t *conf)
{
  const char *str_ptr;
  if(JSON_GET_OBJECT(conf,"networkId"))
  {
    str_ptr = JSON_GET_STRING(conf,"networkId");
    snprintf(networkId,sizeof(networkId),"/%s",str_ptr);
  }
  else
  {
    strncpy(networkId,"",sizeof(networkId));
  }
}

void read_gateway_id_port(json_t *conf)
{
  const char *str_ptr;
  str_ptr = JSON_GET_STRING(conf,"gatewayName");
  snprintf(gatewayName,sizeof(gatewayName),"%s",str_ptr);
  gatewayPort = JSON_GET_INTEGER(conf,"gatewayPort"); 
}

void read_internet_control_configuration(json_t *conf)
{
  const char *str_ptr; 
  str_ptr = JSON_GET_STRING(conf,"remoteUrl");
  if(strlen(str_ptr) == 0)
  {
    DEBUG("Internet control option not selected so skipping");
    return;
  }
  remote_client.serverUri = (char *)malloc(strlen(str_ptr)+1);
  strncpy(remote_client.serverUri,str_ptr,strlen(str_ptr)+1);
  remote_client.serverPort = JSON_GET_INTEGER(conf,"remotePort");

  if(JSON_GET_OBJECT(conf,"remoteUsername"))
  {
    str_ptr = JSON_GET_STRING(conf,"remoteUsername");
    remote_client.username = (char *)malloc(strlen(str_ptr)+1);
    strncpy(remote_client.username,str_ptr,strlen(str_ptr)+1);
  }
  else{
    remote_client.username = NULL;
  }
  if(JSON_GET_OBJECT(conf,"remotePassword"))
  {
    str_ptr = JSON_GET_STRING(conf,"remotePassword");
    remote_client.password = (char *)malloc(strlen(str_ptr)+1);
    strncpy(remote_client.password,str_ptr,strlen(str_ptr)+1);
  }
  else{
    remote_client.password = NULL;
  } 
}

void read_local_control_configuration(json_t *conf)
{
  const char *str_ptr;

  if(JSON_GET_OBJECT(conf,"localUrl"))
  { 
    str_ptr = JSON_GET_STRING(conf,"localUrl");
    local_client.serverUri = (char *)malloc(strlen(str_ptr)+1);
    strncpy(local_client.serverUri,str_ptr,strlen(str_ptr)+1);
  }
  else{
    local_client.serverUri = "localhost";
  }

  if(JSON_GET_OBJECT(conf,"localPort"))
  { 
    local_client.serverPort = JSON_GET_INTEGER(conf,"localPort");
  }
  else
  {
    local_client.serverPort = 1883;
  }

  if(JSON_GET_OBJECT(conf,"localUsername"))
  {
    str_ptr = JSON_GET_STRING(conf,"localUsername");
    local_client.username = (char *)malloc(strlen(str_ptr)+1);
    strncpy(local_client.username,str_ptr,strlen(str_ptr)+1);
  }
  else{
    local_client.username = NULL;
  }

  if(JSON_GET_OBJECT(conf,"localPassword"))
  {
    str_ptr = JSON_GET_STRING(conf,"localPassword");
    local_client.password = (char *)malloc(strlen(str_ptr)+1);
    strncpy(local_client.password,str_ptr,strlen(str_ptr)+1);
  }else{
    local_client.password = NULL;
  }
}

void setup_topology_topic(void)
{
  snprintf(topology_topic,sizeof(topology_topic), "%s/topics",
      networkId);
}
void setup_broadcast_topic(void)
{
  snprintf(notif_broadcast_topic,sizeof(notif_broadcast_topic), "%s/broadcast",
      networkId);
}

void loadConfig(char *config_file)
{
  //int ret;                                                                      
  json_t *config = NULL; // *jobj = NULL;                                          
  json_error_t error;                                                           

  assert(config_file);
  config = json_load_file(config_file,0,&error);                                
  if(!config) {                                                                 
    /* the error variable contains error information */                         
    ERROR("Error in loading config file  at position %d",error.position);       
    exit(1);                                                                    
  }     

  read_iot_network_id(config);
  read_gateway_id_port(config);
  read_internet_control_configuration(config);
  read_local_control_configuration(config);

  /* Should be called at last of loadConfig */
  setup_topology_topic();
  setup_broadcast_topic();

  json_decref(config);
/*  char tmp[200];
  if(getenv("TOPOLOGY_TOPIC"))
  {
    topology_topic = getenv("TOPOLOGY_TOPIC");
  } 
  else{
    topology_topic = "/topics";
  }
  if(getenv("REMOTE_BROKER_URL"))
  {
    remote_client.serverUri = getenv("REMOTE_BROKER_URL");
  }
  else
  {
    remote_client.serverUri = "m10.cloudmqtt.com";
  }

  if(getenv("REMOTE_BROKER_PORT"))
  {
    remote_client.serverPort =(uint16_t) atoi(getenv("REMOTE_BROKER_PORT"));
  }
  else
  {
    remote_client.serverPort = 12291;
  }
  remote_client.connectionStatus = 0;
  remote_client.username = NULL;
  if(getenv("REMOTE_BROKER_USERNAME"))
  {
    remote_client.username = getenv("REMOTE_BROKER_USERNAME");
  }
  else
  {
    remote_client.username = "abhi";
  }
  if(getenv("REMOTE_BROKER_PASSWORD"))
  {
    remote_client.password = getenv("REMOTE_BROKER_PASSWORD");
  }
  else
  {
    remote_client.password = "abhi123";
  }
  if(getenv("REMOTE_BROKER_CLIENT_ID"))
  {
    remote_client.clientId = getenv("REMOTE_BROKER_CLIENT_ID");
  }
  else
  {
    remote_client.clientId = "DefaultClientId";
  }
 */ 
  /*
   * Local client configuration
   */
 /* if(getenv("LOCAL_BROKER_URL"))
  {
    local_client.serverUri = getenv("LOCAL_BROKER_URL");
  }
  else
  {
    local_client.serverUri = "localhost";
  }

  if(getenv("LOCAL_BROKER_PORT"))
  {
    local_client.serverPort = (uint16_t)atoi(getenv("LOCAL_BROKER_PORT"));
  }
  else
  {
    local_client.serverPort = 1883;
  }
  local_client.connectionStatus = 0;
  local_client.username = NULL;
  if(getenv("LOCAL_BROKER_USERNAME"))
  {
    local_client.username = getenv("LOCAL_BROKER_USERNAME");
  }
  else
  {
    local_client.username = NULL;
  }
  if(getenv("LOCAL_BROKER_PASSWORD"))
  {
    local_client.username = getenv("LOCAL_BROKER_PASSWORD");
  }
  else
  {
    local_client.username = NULL;
  }
  if(getenv("LOCAL_BROKER_CLIENT_ID"))
  {
    local_client.clientId = getenv("LOCAL_BROKER_CLIENT_ID");
  }
  else
  {
    local_client.clientId = remote_client.clientId;
  }*/
  /*
   * Topic on which I need to broadcast notification events.
   * By default it would be remoteClientId/broadcast 
   */ 
 /* if(getenv("NOTIFICATION_BROADCAST_TOPIC"))
  {
    notif_broadcast_topic = getenv("NOTIFICATION_BROADCAST_TOPIC");  
  }
  else
  {
    snprintf(tmp,sizeof(tmp),"%s/broadcast",remote_client.clientId); 
    notif_broadcast_topic = (char *) malloc(strlen(tmp)+1);
    if(notif_broadcast_topic == NULL)
    {
      ERROR("malloc failed..exiting");
      exit(-1);
    }
    strcpy(notif_broadcast_topic,tmp); 
    
  }*/
}

void broadcast_gateway_advertise(mqttConnection_t *client)
{

  DEBUG("Going to broadcast gateway advertise message");
  json_t *root = json_object();
  json_object_set_new(root,"msgtype",
                    json_integer(MQTT_MSG_TYPE_GW_COMING_UP));

  json_object_set_new(root,"gwname",
                    json_string(gatewayName));

  char *msg = json_dumps(root,0);
  /*
   * Send the message to either NOTIFICATION BROADCAST TOPIC or to /topics
   * For now sending to /topics to both local and remote 
   * */ 
  publishMqttMessage(msg,topology_topic,0,                     
                        client);

  free(msg);
  json_decref(root);
}

int main(int argc, char   *argv[])
{
  struct event_list_node *iter, *tmp; 
  int listen_fd=0, client_fd = 0, timer_fd;
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;
  int ret_code;
  char *msg;
  json_t *root;
  int opt;
  char *config_file = NULL; 
  /**
   * Initialise Global Variables 
   */
  INIT_LIST_HEAD(&(events_list.head));

  static struct option long_option[] = {
    {"config", required_argument, NULL, 'c'},
    {"logfile", required_argument, NULL, 'l'}
  };
  
  while((opt=getopt_long(argc,argv,"c:l:",long_option,NULL))!=-1)
  {
    switch(opt)
    {
      case 'c':
        STD_LOG("Configuration file option given -- %s",optarg); 
        config_file = optarg;
        break; 
      case 'l':
        STD_LOG("Logfile option given -- %s",optarg); 
        LOG_SET_FILE(optarg);
        break; 

      default: 
        STD_LOG("\nUsage : %s\n\t[--config <configuration file name>]\n"
            "\t[--logfile <logfile name>]\n\n", 
            argv[0]);
    }
  }
  //fd_set read_fd_set;
  fd_set tmp_fd_set;
  char buff[BUFFSIZ];
  socklen_t socklen;
  struct event_list_node *iot_event = NULL, *timer_event = NULL;
  pipe(pipe_fd);
  /**
   * Open Listen Socket for IOT CLIENTS 
   */
  memset(&server_addr, 0, sizeof(server_addr));

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(SERVER_LISTEN_PORT);
  listen_fd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
  ret_code = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
                   &(int){ 1 }, sizeof(int)); 

  if(ret_code < 0) 
  {
    ERROR("Error in setsockopt call for REUSEADDR");
  }
  if(listen_fd < 0 )
    STD_LOG("\nError in listen");
  CHECK(listen_fd > 0,"Error opening listen socket");
  DEBUG("======>>> Socket opened fd = %d <<<======",listen_fd);
  
  ret_code = bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in)); 
  if(ret_code < 0 )
    STD_LOG("\nError in listen");
  
  CHECK(ret_code==0,"Error in bind call for listen_fd");

  ret_code = listen(listen_fd,MAX_NUMBER_OF_CLIENTS);
  CHECK(ret_code==0,"listen call failed");
  DEBUG("Listen Socket created successfully");
  memset(&server_addr, 0, sizeof(server_addr));
  socklen = sizeof server_addr;
  ret_code = getsockname(listen_fd, (struct sockaddr *)&server_addr, &socklen);

  if(ret_code < 0 || socklen != sizeof server_addr)
     STD_LOG("\n Error getsockname");
  DEBUG("Listen Port = %d", ntohs(server_addr.sin_port));
 
  mosquitto_lib_init();

  /*
   * get configuration from environment
   */ 
  loadConfig(config_file);  //load broker connection configurations

  /**
   * Connect to Local MQTT broker 
   */
  //mqttClientInit(&local_client,LOCAL_MQTT_BROKER_URL,LOCAL_MQTT_BROKER_PORT,
  //                      CLIENTID,true);
  mqttClientInit(&local_client,1);
  mqttSetCallbacks(local_client.client);
  mqttClientConnect(&local_client);
  mqttStartClientThread(&local_client);
  subscribe_topics(&local_client,0);
  //sleep(3);
  /*
   * Connect to remote broker
   * */

#if 1
  if(remote_client.serverUri)
  {
    DEBUG("Connecting to remote  broker..");
    DEBUG("Starting remote broker connection...");
    mqttClientInit(&remote_client,1);
    mqttSetCallbacks(remote_client.client);
    mqttClientConnect(&remote_client);
    mqttStartClientThread(&remote_client);
    subscribe_topics(&remote_client,0);
  }
#endif
  
  //snprintf(mqtt_username,MAX_MQTT_USERNAME_LEN,"%s",MQTT_USERNAME);
  //snprintf(mqtt_password,MAX_MQTT_PASSWORD_LEN,"%s",MQTT_PASSWORD);
  //conn_opts.username = mqtt_username;
  //conn_opts.password = mqtt_password;

  /**
   * Subscribe for initial topics "/topics""
   * Any device on start of day will send GET in this topic so that all IOT GW
   * answer with the list of topics it serves
   */
  global_work_scan_timer_fd = timerfd_create(CLOCK_MONOTONIC,TFD_CLOEXEC);
  int flags = fcntl(global_work_scan_timer_fd, F_GETFL, 0);
  if(fcntl(global_work_scan_timer_fd, F_SETFL, flags | O_NONBLOCK))
  {
    DEBUG("Error setting timer fd to unblocking");
  }

  //Select and wait for message to come form an IOT NODE
  FD_ZERO(&read_fd_set);
  FD_SET(listen_fd,&read_fd_set);

  add_event(&events_list,&read_fd_set,listen_fd,LISTEN_SOCKET);
  STD_LOG("HERE\n");
  add_event(&events_list,&read_fd_set,pipe_fd[0],MQTT_MSG_EVENT);
  STD_LOG("HERE\n");
  add_event(&events_list,&read_fd_set,global_work_scan_timer_fd,
                                          WORK_SCAN_TIMER_EVENT);
  STD_LOG("HERE\n");

  sleep(10); 
  list_for_each_entry(iter,&events_list.head,head) {                                  
       DEBUG("itertype = %d, iterfd = %d", iter->event_type, iter->event_fd);                                    
        } 
  while(1)
  {
    gc_events_list();

    FD_ZERO(&tmp_fd_set);
    tmp_fd_set = read_fd_set;
    
    /*DEBUG("Before calling select, max_fd = %d,"
       " FD_ISSET(listen_fd,&tmp_fd_set)=%d",
        max_fd, FD_ISSET(listen_fd,&tmp_fd_set));*/

    /*list_for_each_entry(iter,&events_list.head,head) {                            
             DEBUG("itertype = %d, iterfd = %d", iter->event_type, iter->event_fd);   
    } */
    ret_code = select(max_fd,&tmp_fd_set,NULL,NULL,NULL);
    DEBUG("Select event occured");
    iter = (void *) NULL; 
    
    list_for_each_entry_safe(iter,tmp,&(events_list.head),head)
    {
      if(FD_ISSET(iter->event_fd,&tmp_fd_set))
      {
        if(iter->event_type == LISTEN_SOCKET)
        {
          socklen = sizeof(client_addr);
          client_fd = accept(iter->event_fd,
                            (struct sockaddr *)&client_addr,
                            &socklen); 
          
          iot_event = add_event(&events_list,&read_fd_set, client_fd, IOT_EVENT);
          CHECK(iot_event != NULL, "Error in adding event (malloc fail)");
         
          timer_fd = timerfd_create(CLOCK_MONOTONIC,TFD_CLOEXEC);
          timer_event =  add_event(&events_list,&read_fd_set,timer_fd,TIMER_EVENT); 
          if(timer_event == NULL)
          {
            del_event(&events_list,iot_event,&read_fd_set);
            DEBUG("errro in adding timer fd for IOT_NODE");
          }
        
          timer_event->event_data.timer_data.node_ptr = iot_event;
          EVENT_TO_IOT_DATA(iot_event).timer_ptr = timer_event;

          DEBUG("added IOT_EVENT %p and corresponding TIMER_EVENT %p for"
              " client \%d"
              ,iot_event, timer_event,
              ((timer_event->event_data).timer_data.node_ptr)->event_fd);
          
        }
        if(iter->event_type == IOT_EVENT)
        {
          DEBUG("IOT_EVENT occured"); 
          ret_code = read(iter->event_fd, buff, BUFFSIZ);
          /* TBD error handling in read**/
          buff[ret_code] = 0;

          if(ret_code <= 0)
          {
            DEBUG("Connection closed by client ret_code = %d."
                  " Deleting timer event.",ret_code);

            /**
             * Mark this IOT node for deletion and also mark the timer 
             * associated with it.   
             */
            MARK_FOR_DELETION(iter);
            MARK_FOR_DELETION(IOT_PTR_TO_TIMER_PTR(iter));

            root = json_object();
            json_object_set_new(root,"msgtype",
                                json_integer(MQTT_MSG_TYPE_GW_NODE_GOING_DOWN));
            
            json_object_set_new(root,"gw_name",
                json_string(gatewayName));
           
            json_object_set_new(root,"nodename",
                  json_string(EVENT_TO_IOT_DATA(iter).client_name));
            
            msg = json_dumps(root,0);

            /*
             * Send the message to either NOTIFICATION BROADCAST TOPIC or to /topics
             * For now sending to /topics to both local and remote 
             * */ 
            publishMqttMessage(msg,notif_broadcast_topic,0,                  
                                  &remote_client);
            publishMqttMessage(msg,notif_broadcast_topic,0,                   
                                  &local_client);

            free(msg);  
            EVENT_TO_IOT_DATA(iter).work_state = IOT_NODE_FREE;
            json_decref(root);

            continue; 
          } 
          process_iot_msg(iter, buff, ret_code);
        }   

        if(iter->event_type == TIMER_EVENT)
        {
          ret_code = read(iter->event_fd, buff, BUFFSIZ);
          if(ret_code < 0) 
          {
            ERROR("TIMER_EVENT read error");
            continue;
          }
          DEBUG("WOW Timer event has occurred for client %d"
               ", %ld times",((iter->event_data).timer_data.node_ptr)->event_fd,
            *(long int *)buff);

          if(CLIENT_DEAD((iter->event_data).timer_data.node_ptr))
          {
            /**
             * Client missed 3 HELLOS. Close Connection and clean up.
             * First delete the client then this timer 
             */
            MARK_FOR_DELETION(iter);
            MARK_FOR_DELETION(TIMER_PTR_TO_IOT_PTR(iter));
            /*
             * Publish the message that Client has disconnected
             */ 
            root = json_object();
            json_object_set_new(root,"msgtype",
                                json_integer(MQTT_MSG_TYPE_GW_NODE_GOING_DOWN));
            
            json_object_set_new(root,"gw_name",
                json_string(gatewayName));
           
            json_object_set_new(root,"nodename",
                  json_string(TIMER_TO_IOT_DATA(iter).client_name));
            
            msg = json_dumps(root,0);

            /*
             * Send the message to either NOTIFICATION BROADCAST TOPIC or to /topics
             * For now sending to /topics to both local and remote 
             * */ 
            publishMqttMessage(msg,notif_broadcast_topic,0,                     
                                  &remote_client);
            publishMqttMessage(msg,notif_broadcast_topic,0,                     
                                  &local_client);

            free(msg);  
            EVENT_TO_IOT_DATA(iter).work_state = IOT_NODE_FREE;
            json_decref(root);

          }
          else
          {
            /**
             * client is not dead so need to send HELLO and decrement hello_count 
             */
            
            /**
             * Append work to the worklist 
             */
            struct request_node_s *request;
            request = (struct request_node_s *) 
                              calloc (1,sizeof(struct request_node_s));   
            request->req_type = HELLO_REQUEST;  
            request->req_status = REQUEST_PENDING;
            request->req_data = NULL;

            ENQUEUE_IOT_REQUEST(TIMER_PTR_TO_IOT_PTR(iter),request);
            /*
            ret_code = write(((iter->event_data).timer_data.node_ptr)->event_fd,
                            buff,ret_code); 
            if(ret_code < 0 ) 
            {
              DEBUG("Error Writing to client %d. Need to clean up",
                  (TIMER_PTR_TO_IOT_PTR(iter)->event_fd));
            }*/

            TIMER_TO_IOT_DATA(iter).hello_count--;
            DEBUG("ENqueued Hello request %p for client %s. "
                "Current Hello count is %d",request,
                TIMER_TO_IOT_DATA(iter).client_name, 
                TIMER_TO_IOT_DATA(iter).hello_count);
          }
        }
        if(iter->event_type == MQTT_MSG_EVENT)
        {
          ret_code = read(iter->event_fd, buff, BUFFSIZ);
          if(ret_code < 0) 
          {
            ERROR("MQTT_MSG_EVENT read error");
            continue;
          }
          buff[ret_code]=0;
          DEBUG("MQTT Message event occured: %s, ret_code = %d", buff,ret_code);
          if(strcmp(buff,"WORK LOAD"))
          {
            DEBUG("DID not match WORK LOAD");
            continue;
          }
          DEBUG("Matched  WORK LOAD");
          pthread_mutex_lock(&work_list_mutex);
          struct request_node_s *work_node=NULL,*work_node_tmp=NULL; 
          struct event_list_node *search_node = NULL;
          uint8_t node_found = 0;
          list_for_each_entry_safe(work_node,work_node_tmp,
                                          &work_list,node)
          {
            /**
             * Search for an IOT node whose name and id matches the work's
             * iotnode_name
             */
            DEBUG("Work type = %d",work_node->req_type); 
            DEBUG("Work for node named %s",
                ((struct mqtt_request_s *)(work_node->req_data))->iotnode_name); 

            list_for_each_entry(search_node,&events_list.head,head) 
            { 
              if(search_node->event_type == IOT_EVENT &&
                  !strcmp(EVENT_TO_IOT_DATA(search_node).client_name,
                  ((struct mqtt_request_s *)(work_node->req_data))->iotnode_name))
              {
                DEBUG("Found IOT_NODE( fd %d ) for which request arrived",
                          EVENT_TO_IOT_DATA(search_node).client_id);
                list_del(&work_node->node);
                ENQUEUE_IOT_REQUEST(search_node, work_node);
                node_found = 1;  
                break;
              }           
            }

            if(node_found == 0) 
            {
              ERROR("Did not find requested node %s", 
               ((struct mqtt_request_s *)(work_node->req_data))->iotnode_name);
              
              root = json_object();
              json_object_set_new(root,"msgtype",
                              json_integer(MQTT_MSG_TYPE_ERROR_RESP));

              json_object_set_new(root,"nodename",
                    json_string(((struct mqtt_request_s *)
                        (work_node->req_data))->iotnode_name));

              json_object_set_new(root,"reason", 
                                  json_string("IOTNODE_NOT_FOUND"));

              msg = json_dumps(root,0);
              /*
               * Now send the messge to reques_sender_name
               */
              DEBUG("\nGoing to send msg on topic %s->\n%s\n",
                    ((struct mqtt_request_s *)
                     (work_node->req_data))->req_sender_name,
                    msg);

              snprintf(buff,BUFFSIZ,"%s/%s",networkId,
                  ((struct mqtt_request_s *)(work_node->req_data))->req_sender_name);
              publishMqttMessage(msg,buff,0,
                ((struct mqtt_request_s *)(work_node->req_data))->brokerConnection);

              free(msg);  
              json_decref(root);
              deleteReqBeforeEnqueue(work_node);
            }
          }
          pthread_mutex_unlock(&work_list_mutex);
        }
        if(iter->event_type == WORK_SCAN_TIMER_EVENT)
        {
          DEBUG("Timer event. Need to CHECK if some work can be processed");
          ret_code = read(iter->event_fd, buff, BUFFSIZ);
          if(ret_code < 0) 
          {
            ERROR("WORK_SCAN_TIMER_EVENT read error");
            continue;
          }
          try_do_some_work(); 
        }
      }

    } 
  }
error:
  return (1);
}

