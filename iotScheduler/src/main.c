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
#include <unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include <getopt.h>
#include<stdarg.h>
#include<mqttdefs.h>
#include<mqttfunc.h>
#include<log.h>
#include<string.h> 
#include<pthread.h>
#include<schedule.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include<jansson.h>
#include<load_config.h>

FILE *log_f;

/** 
 * networkId Configured by read_iot_network_id function
 */
char networkId[MAX_NETWORK_ID_LEN];
/**
 * gatewayName, gatewayPort configured by read_gateway_id_port function
 */
char gatewayName[MAX_GATEWAY_NAME_LEN];
uint16_t gatewayPort; 

//char gateway_topic[MAX_GATEWAY_NAME_LEN]; 
char save_dir[MAX_FILE_PATH_LEN]; 
char logfile_name[MAX_FILE_PATH_LEN];

LIST_HEAD(event_subsc_list); 

struct list_head task_list;
uint32_t task_count;
pthread_mutex_t task_list_mutex = PTHREAD_MUTEX_INITIALIZER;  

mqttConnection_t local_client; 
mqttConnection_t remote_client; 
char *remote_url;
int remote_port; 
int local_port;

void print_start_message(void){
  PRINT("-*-*-*-*-*-* STARTED SCHEDULAR *-*-*-*-*-*-*");
}
void print_settings(void){
  PRINT("============= SETTINGS ===========");  
  PRINT("\tLogger Started..");
  PRINT("\t--logfile set to %s",logfile);
  PRINT("\t--Save Directory set to %s",save_dir);
}

void load_config(char *config_file)
{

  if(!config_file)
  {
    ERROR("Config file not given");
    exit(1);
  }
  json_t *config = NULL; 
  json_error_t error;
  config = json_load_file(config_file,0,&error);
  if(!config) {
    /* the error variable contains error information */
    ERROR("Error in loading config file  at position %d",error.position);
    exit(1);
  }

  /*Get the IOT network identifier to connect to */
  read_iot_network_id(config);

  /* Get the IOT gateway Name and the port to run on (default 10001)*/
  read_gateway_id_port(config);

  /* Get the local and internet mosquitto server setting */
  read_internet_control_configuration(config);
  read_local_control_configuration(config);

#if 0
  gateway_topic 
  /* get the gateway name from config file */
  jobj = json_object_get(config,"gatewayName");
  if(!jobj)
  {
    ERROR("gatewayName not provided.");
    json_decref(config);
    exit(1);
  }
  ret = JSON_CHK_STRING(jobj);
  if(!ret) 
  {
    DEBUG("Error processing gatewayName in config file: Not string"); 
    json_decref(config);
    exit(1);
  }
  strncpy(gateway_topic,json_string_value(jobj),MAX_GATEWAY_NAME_LEN);
  /* get the savedir name, if not provided then will log to stdout */
  jobj = json_object_get(config,"databaseDir");
  if(!jobj)
  {
    ERROR("Database store dir not provided.");
    json_decref(config);
    exit(1);
  }
  ret = JSON_CHK_STRING(jobj);
  if(!ret) 
  {
    DEBUG("Error processing databaseDir in config file: Not string"); 
    json_decref(config);
    exit(1);
  }
  strncpy(save_dir,json_string_value(jobj),MAX_FILE_PATH_LEN);
  /* get the log file name, if not provided then will log to stdout */
  jobj = json_object_get(config,"logfileName");
  if(!jobj)
  {
    DEBUG("Logfile name not provided.");
  }
  else
  {
  ret = JSON_CHK_STRING(jobj);
  if(!ret) 
  {
    DEBUG("Error processing logfileName in config file: Not string"); 
    json_decref(config);
    exit(1);
  }
  strncpy(logfile_name,json_string_value(jobj),MAX_FILE_PATH_LEN);
  LOG_SET_FILE(logfile_name);
  }
  /* Load internet broker url */
  jobj = json_object_get(config,"internetServerUri");
  if(!jobj)
  {
    DEBUG("Internet server option not given. Only Local access will be possible.");
  }
  else
  {
    remote_client = (mqttConnection_t *)calloc(1,sizeof(mqttConnection_t));
    ret = JSON_CHK_STRING(jobj);
    if(!ret) 
    {
      DEBUG("Error processing logfileName in config file: Not string"); 
      json_decref(config);
      free(remote_client);
      exit(1);
    }
    remote_url = calloc(1, strlen(json_string_value(jobj)+1));
    if(!remote_url)
    {
      ERROR("ERROR in calloc for remote_url");
      json_decref(config);
      free(remote_client);
      exit(1);
    }
    strcpy(remote_url,json_string_value(jobj));

    /* Load internet server port setting */ 
    jobj = json_object_get(config,"internetServerPort");
    if(!jobj)
    {
      DEBUG("Internet server port not given. Setting to 1883");
      remote_port = 1883; 
    }
    else
    {
      ret = JSON_CHK_INTEGER(jobj);
      if(!ret)
      {
        ERROR("Internet server port not integer");
        json_decref(config);
        free(remote_client);
        free(remote_url);
        exit(1);
      }
      remote_port = json_integer_value(jobj);
    }

    /*Load internet server username */
    jobj = json_object_get(config,"internetServerUsername");
    if(jobj)
    {
      ret = JSON_CHK_STRING(jobj);
      if(!ret) 
      {
        DEBUG("Error processing internetServerUsernme in config file: Not string"); 
        json_decref(config);
        free(remote_client);
        free(remote_url);
        exit(1);
      }
      remote_client->username = calloc(1, strlen(json_string_value(jobj)+1));
      if(!remote_client->username)
      {
        ERROR("ERROR in calloc for internetServerUsername");
        json_decref(config);
        free(remote_client);
        free(remote_url);
        exit(1);
      }
      strcpy(remote_client->username,json_string_value(jobj));
    }
    /*Load internet server password */
    jobj = json_object_get(config,"internetServerPassword");
    if(jobj)
    {
      ret = JSON_CHK_STRING(jobj);
      if(!ret) 
      {
        DEBUG("Error processing internetServerPassword in config file: Not string"); 
        json_decref(config);
        free(remote_client);
        free(remote_url);
        free(remote_client->username);
        exit(1);
      }
      remote_client->password = calloc(1, strlen(json_string_value(jobj)+1));
      if(!remote_client->password)
      {
        ERROR("ERROR in calloc for internetServerPassword");
        json_decref(config);
        free(remote_client);
        free(remote_url);
        free(remote_client->username);
        exit(1);
      }
      strcpy(remote_client->password,json_string_value(jobj));
    }
    remote_client->serverUri = remote_url; 
    remote_client->serverPort = remote_port;

  }
#endif
  json_decref(config);
  DEBUG("RETURNING FROM LOAD_CONFIG");
}
void load_db(const char *save_dir)
{
  DIR *sd;
  struct dirent *dp;
  sd = opendir(save_dir);
  char filename[100]; 

  if(sd == NULL)
  {
    ERROR("Error opening save_dir, could not load database");
    return; 
  }
  while ((dp = readdir(sd)) != NULL)
  {
    struct stat stbuf ;
    sprintf( filename , "%s/%s",save_dir,dp->d_name) ;
    if( stat(filename,&stbuf ) == -1 )
    {
      ERROR("Unable to stat file: %s\n",filename) ;
      continue ;
    }

    if ( ( stbuf.st_mode & S_IFMT ) == S_IFDIR )
    {
      continue;
      // Skip directories
    }
    else
    {
      json_t *json;
      json_error_t error;

      json = json_load_file(filename, 0, &error);
      if(!json) {
        /* the error variable contains error information */
        ERROR("Error in json load at position %d",error.position);
        continue;
      }
      if(process_json_msg(json) != RC_OK)
      {
        ERROR("Error processing db");
      } 
      json_decref(json);
    }
  }  
}
int main(int argc,char *argv[])
{

  int opt; 
  int index; 
  char *config_file = NULL; 
  time_t timeval; 
  INIT_LIST_HEAD(&task_list);

  static struct option long_option[] = {
    // {"topic", required_argument,0,'t'},
    {"config",  required_argument,0,'c'},
    {"logfile",required_argument,0,'l'},
    {"savedir",required_argument,0,'s'}
  };

  while((opt = getopt_long(argc,argv,"c:l:s:",long_option,&index))!=-1){
    switch(opt)
    {
      /* case 't': //get the listen topic
         printf("\n>> Setting Listen topic to %s\n",optarg);
         scheduler_topic = optarg;  
         break;*/
      case 'l':
        printf("\n>> Setting Logfile to %s\n",optarg);
        strncpy(logfile_name,optarg,MAX_FILE_PATH_LEN);
        LOG_SET_FILE(logfile_name);
        break;
      case 's':
        printf("\n>> Setting Save Directory to %s\n",optarg);
        strncpy(save_dir,optarg,MAX_FILE_PATH_LEN);
        break;
      case 'c':
        printf("\n>> Setting config file  to %s\n",optarg);
        config_file = optarg;
        break;
      default:
        printf("\nUsage %s\n\t[-t <Listen Topic>]\n\t[-l <logfile name>]\n\n",argv[0]);
        exit(1);
    }
  }

  DEBUG("BEFORE Load Config..");
  load_config(config_file); 
  DEBUG("After Load Config..");
  print_start_message(); 
  print_settings();

  load_db(save_dir);

  mosquitto_lib_init();
  /*
   * Connect to MQTT local broker
   */
  mqttClientInit(&local_client,1);
  mqttSetCallbacks(local_client.client);
  mqttClientConnect(&local_client);
  mqttStartClientThread(&local_client);
  subscribe_topics(&local_client,0);

  /*
   * Connect to MQTT remote broker
   */
  mqttClientInit(&remote_client,1);
  mqttSetCallbacks(remote_client.client);
  mqttClientConnect(&remote_client);
  mqttStartClientThread(&remote_client);
  subscribe_topics(&remote_client,0);
#if 0
  /*
   * Connect to MQTT internet broker
   */
  /*remote_client->serverUri="m10.cloudmqtt.com";
  remote_client->serverPort = 12291;
  remote_client->username  = "abhi";
  remote_client->password = "abhi123";
  remote_client->clientId = "Abhi1234456";*/
  snprintf(topic,sizeof(topic),"%s/%s",gateway_topic,"Scheduler");
  remote_client.clientId = calloc(1,strlen(topic)+1);
  if(remote_client.clientId == NULL)
  {
    ERROR("Error in calloc local clientId");
    exit(1);
  }
  strcpy(remote_client->clientId, topic);
  mqttClientInit(remote_client,1);                                              
  mqttSetCallbacks(remote_client->client);                                        
  mqttClientConnect(remote_client);                                             
  mqttStartClientThread(remote_client); 
  snprintf(topic,sizeof(topic),"%s/%s",gateway_topic,"Scheduler");
  mqttClientSubscribe(remote_client,topic,2);                          
  snprintf(topic,sizeof(topic),"%s/%s",gateway_topic,"broadcast");
  mqttClientSubscribe(remote_client,topic,2);
#endif

  while(1)
  {
    //Event Loop that checks for any match in time 
    //list_for_each_entry(task,&task_list,node){

    //}
    struct task_s *task_node,*tmp; 
    sleep(1);

    list_for_each_entry_safe(task_node,tmp,&task_list,node)
    {
      switch(task_node->type)
      {
        case SWITCH_OFF_TIMER: 
          //DEBUG("task operstate = %d",task_node->oper_state);
          if(task_node->oper_state != ACTIVE)
          {
            DEBUG("task %d not active", task_node->id);
            break;
          }
          else
          {
            DEBUG("task %d  active", task_node->id);
          }
          time(&timeval);
          if(timeval >= task_node->timer_data.timeval)
          {
            DEBUG("Timer expired sending SWITCH OFF request");
            task_node->cmd((void *)task_node->cmd_arg);
            //sendMqttSetRequest((void *)task_node);
            //If timerPersist was requested to be once then delete the task
            if(task_node->persist == ONCE)
            {
              /* first get the node out of list */
              DEBUG("timer was set up to be once so delete");
              list_del(&task_node->node);
              delete_task(task_node);
            }
            else{
              DEBUG("Timer was set ALWAYS not deleting");
              deActivateSwitchOffTimer(task_node);
            } 
          } 
          break; 
        case ON_TIMER:

          break;
        case OFF_TIMER:

          break; 
      } 
    }
  }
  return 0; 
}

