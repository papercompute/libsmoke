#include "smoke_str.h"

#include <iostream>

#include <fstream>
#include <atomic>




int main (int argc, char *argv[])
{
  if (argc != 2) {
    fprintf (stderr, "Usage: %s [name]\n", argv[0]);
    exit (EXIT_FAILURE);
  }
  const char* name = argv[1];

  switch(smoke::str::s_hash(name))
  {
    case smoke::str::s_hash("popka"):
    std::cout<<"popka"<<std::endl; 
    break;
    case smoke::str::s_hash("/user/1"):
    std::cout<<"/user/1"<<std::endl; 
    break;
    default:
        std::cout<<"def"<<std::endl; 

  } // switch


    
  return EXIT_SUCCESS;  
}
