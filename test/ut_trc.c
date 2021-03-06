/*
**  (C) by Remo Dentato (rdentato@gmail.com)
** 
** This software is distributed under the terms of the MIT license:
**  https://opensource.org/licenses/MIT
**     
**                        ___   __
**                     __/  /_ /  )
**             ___  __(_   ___)  /
**            /  / /  )/  /  /  /
**           /  (_/  //  (__/  / 
**          (____,__/(_____(__/
**    https://github.com/rdentato/clibutl
**
*/

#include "utl.h"


int main(int argc, char *argv[])
{
  int chk ;
  
  logopen("l_trc.log","w");
  
  logwatch("pluto") {
    logtrace("pluto");
  }

  chk = utl_log_check_fail;
  logwatch("event<*s>1", "event 2") {
    logtrace("event 1");
    logtrace("event2"); // will fail
    logtrace("event 3");
  }
  logprintf("In the watch above a test is supposed to FAIL! (but will not be counted)");
  if (chk == utl_log_check_fail-1) utl_log_check_fail--;
  else utl_log_check_fail++;
  
  logwatch("pippo", "!pluto") {
    chk = utl_log_check_fail;
    logtrace("pluto");
    logprintf("The check above is supposed to FAIL! (but will not be counted)");
    if ((unsigned int)chk < utl_log_check_fail) utl_log_check_fail--;
    else utl_log_check_fail++;
    logtrace("pippo");
  }
  
  logclose();
  exit(0);
}
