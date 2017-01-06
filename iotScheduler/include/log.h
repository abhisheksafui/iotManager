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
#ifndef LOG_H
#define LOG_H


#define DEBUG(M,...) log_msg(DEBUG,logfile,__FILE__,__FUNCTION__,__LINE__,M,##__VA_ARGS__)
#define ERROR(M,...) log_msg(ERROR,logfile,__FILE__,__FUNCTION__,__LINE__,M,##__VA_ARGS__)
#define PRINT(M,...) log_msg(NONE,logfile,__FILE__,__FUNCTION__,__LINE__,M,##__VA_ARGS__)
#define LOG_SET_FILE(F) { \
  if(strlen(F)!=0)\
    logfile = F;\
} 

extern char *logfile; 

typedef enum {
  DEBUG,
  ERROR,
  NONE
}LOG_TYPE; 

void log_msg(LOG_TYPE level ,char *log_file,  char *file, const char *function, int line,
    char *msg, ...);
#endif 
