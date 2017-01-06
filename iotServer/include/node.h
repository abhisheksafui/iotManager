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
#ifndef NODE_H
#define NODE_H


#include<list.h>
#include<jansson.h>
#include<event.h>

#if 0
typedef union capability_value_s
{
  ON_OFF status;
  float pot;
  uint8_t fan_speed;
  char *string;
}CAPABILITY_VALUE;

typedef struct caps_list_node
{
  struct list_head head;
  CAPABILITY  type;
  CAPABILITY_VALUE value;

}CAPABILITY_NODE;

typedef struct iot_node 
{
  char node_id[MAX_NODE_ID_LEN];
  list_head caps; //list of CAPABILITY_NODE
  //may also keep uptime
}
#endif 
uint8_t  parse_msg_type(json_t *);
void parse_and_fill_node_name(struct event_list_node *node, json_t *);
int8_t parse_and_fill_node_caps(struct event_list_node *node, json_t *);
void process_iot_msg(struct event_list_node *node,char *buff,uint16_t len);
void sendNodeMessage(struct event_list_node *event);
void try_do_some_work(void);
#endif
