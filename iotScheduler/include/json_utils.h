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
#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#define JSON_GET_OBJECT(C,N) ({\
  json_object_get(C,N);\
})
#define JSON_CHK_OBJECT(j) ({\
    uint8_t ret = json_is_object(j);\
    if(!ret)\
    {DEBUG("Json value is not object"); }\
    ret;\
    })
#define JSON_CHK_INTEGER(j) ({\
    uint8_t ret = json_is_integer(j);\
    if(!ret)\
    {DEBUG("Json value is not integer"); }\
    ret;\
    })
#define JSON_CHK_STRING(j) ({\
    uint8_t ret = json_is_string(j);\
    if(!ret)\
    {DEBUG("Json value is not string"); }\
    ret;\
    })
#define JSON_CHK_ARRAY(j) ({\
    uint8_t ret = json_is_array(j);\
    if(!ret)\
    {\
    DEBUG("Json value is not array");\
    }\
    ret;\
    })

#define JSON_GET_STRING(C,N) ({\
  int ret;\
  json_t *obj = json_object_get(C,N);\
  if(!obj)\
  {\
    ERROR("Configuration file does not contain %s..",N);\
    json_decref(C);\
    assert(0);\
  }\
  ret = JSON_CHK_STRING(obj);\
  if(!ret)\
  {\
    ERROR("Error processing %s..",N);\
    json_decref(C);\
    assert(0);\
  }\
  const char *j = json_string_value(obj);\
  j;\
}) 

#define JSON_GET_INTEGER(C,N) ({\
    int ret;\
  json_t *obj = json_object_get(C,N);\
  if(!obj)\
  {\
    ERROR("Configuration file does not contain %s..",N);\
    json_decref(C);\
    assert(0);\
  }\
  ret = JSON_CHK_INTEGER(obj);\
  if(!ret)\
  {\
    ERROR("Error processing %s..",N);\
    json_decref(C);\
    assert(0);\
  }\
  json_integer_value(obj);\
}) 

#endif
