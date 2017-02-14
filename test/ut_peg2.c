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


pegrule(test1) { //test1 = digit* upper
  pegstar {
    pegdigit;
  }
  pegupper;
}

pegrule(test2) { //test2 = digit+ upper
  pegplus {
    pegdigit;
  }
  pegupper;
}

pegrule(test3) { //test3 = digit? upper
  pegopt {
    pegdigit;
  }
  pegupper;
}

pegrule(test4) { // test4 = (AB){2,3}
  pegrpt(2,3) {
    pegstr("AB");
  }  
}

pegrule(lvowel) { // lower wovel
  pegswitch {
    pegcase {
      pegoneof("aeiou");
    }
    pegcase {
      pegpmx("<=àèìòù>");
    }
  }  
}

int main(int argc, char *argv[])
{
  const char *q;
  const char *p;
//  int ret;
  peg_t pg;
  
  logopen("l_peg2.log","w");

  logcheck((pg = pegnew())); 

  /* *** Check repetitions *** */
  
  logcheck(pegparse(pg,test1,"12488B"));
  logcheck(pegparse(pg,test1,"B"));
  logcheck(!pegparse(pg,test1,"12488z"));
  logcheck(!pegparse(pg,test1,"124889"));
  logcheck(!pegparse(pg,test1,"z"));
  
  logcheck(pegparse(pg,test2,"12488B"));
  logcheck(!pegparse(pg,test2,"B"));
  logcheck(!pegparse(pg,test2,"12488z"));
  logcheck(!pegparse(pg,test2,"124889"));
  logcheck(!pegparse(pg,test2,"z"));
  
  logcheck(pegparse(pg,test3,"1B"));
  logcheck(pegparse(pg,test3,"B"));
  logcheck(!pegparse(pg,test3,"12488z"));
  logcheck(!pegparse(pg,test3,"124889"));
  logcheck(!pegparse(pg,test3,"z"));
  
  logcheck(!pegparse(pg,test4,"PIPPO"));
  logcheck(!pegparse(pg,test4,"AB"));
  logcheck(pegparse(pg,test4,"ABAB"));
  
  q = "ABABAB"; 
  logcheck(pegparse(pg,test4,q) && (pegfailpos(pg)[0]=='\0'));
  
  // Only the first six characters are parsed correctly
  // The parse succeeds but *pegfailpos() is not '\0'
  q = "ABABABAB";
  logcheck(pegparse(pg,test4,q) && (pegfailpos(pg)[0]!='\0'));

  
  /* *** Check alternatives *** */
  q=pmxmatch("<utf>",""); // Set pmx to utf8
  
  logcheck(pegparse(pg,lvowel,"a"));
  logcheck(pegparse(pg,lvowel,"à"));
  logcheck(pegparse(pg,lvowel,"i"));
  logcheck(pegparse(pg,lvowel,"ù"));
  logcheck(!pegparse(pg,lvowel,"c"));
  logcheck(!pegparse(pg,lvowel,"ç"));


  
  pg = pegfree(pg);
  logclose();

  exit(0);
}
