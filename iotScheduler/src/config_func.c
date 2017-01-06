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
#include<json_utils.h>
#include<jansson.h>
#include<log.h>
#include<string.h>
#include<assert.h>
#include<inttypes.h>
#include<global.h>
/**
 *  Read theh network id (Identifier to distinguish iot networks)
 *  Identifier that you have allocated from safuiiot.com
 */ 
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

