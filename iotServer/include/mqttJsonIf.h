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
#ifndef MQTT_JSON_IF
#define MQTT_JSON_IF

typedef enum MQTT_IF_MSGTYPE
{

  MQTT_MSG_TYPE_GET=1,
  MQTT_MSG_TYPE_GET_RESP,
  MQTT_MSG_TYPE_SET,
  MQTT_MSG_TYPE_SET_RESP,
  MQTT_MSG_TYPE_GET_TOPO,
  MQTT_MSG_TYPE_GET_TOPO_RESP,
  MQTT_MSG_TYPE_NOTIF_BROADCAST,
  MQTT_MSG_TYPE_ERROR_RESP,
  MQTT_MSG_TYPE_GW_NODE_COMMING_UP,
  MQTT_MSG_TYPE_GW_NODE_GOING_DOWN,
  MQTT_MSG_TYPE_GW_COMING_UP

}MQTT_IF_MSGTYPE;

#endif 
