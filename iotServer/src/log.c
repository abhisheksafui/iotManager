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
#include<log.h>
#include<stdarg.h>
#include<stdio.h>

char *logfile = NULL;

void log_msg(LOG_TYPE level ,char *log_file,  char *file, const char *function, int line,
    char *msg, ...)
{
  va_list args;
  FILE *log_f;
  va_start(args,msg);
  if(log_file == NULL){
    log_f = stdout; 
  }else {
    log_f = fopen(log_file,"a");
  }
  if(level == DEBUG){
    fprintf(log_f," [DEBUG] ");
  }else if(level == ERROR){
    fprintf(log_f," [ERROR] ");
  }else if(level == CHECK){
    fprintf(log_f," [CHECK_FAIL] ");
  }

  vfprintf(log_f,msg,args);
  fprintf(log_f,"\n");
  va_end(args);
  if(log_f!=stdout)
    fclose(log_f);
}

