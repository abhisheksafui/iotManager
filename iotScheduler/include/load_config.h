/*Copyright 2016 Abhishek Safui

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef LOAD_CONF_H
#define LOAD_CONF_H

#include<jansson.h>


void read_iot_network_id(json_t *conf);
void read_gateway_id_port(json_t *conf);
void read_internet_control_configuration(json_t *conf);
void read_local_control_configuration(json_t *conf);

#endif 
