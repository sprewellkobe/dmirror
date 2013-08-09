#ifndef UNIXSOCKETPROTOCOLHPP
#define UNIXSOCKETPROTOCOLHPP
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include "Maindef.hpp"
#include "Common.hpp"
using namespace std;
//-------------------------------------------------------------------------------------------------

class UnixSocketCommand
{
 public:
  string cmd;
  vector<string> params;
 public:
  bool IsEmpty()
  {return cmd.empty();}
  void Display()
  {
   printf("%s\tunixsocket command=>%s\n",GetCurrentTime().c_str(),cmd.c_str());
   for(unsigned int i=0;i<params.size();i++)
       printf("%s\n",params[i].c_str());
   printf("\n");
  }
 public:
  bool Do(string& result,COMMAND_FUNCTION unix_socket_command_function);
};//end class UnixSocketCommand
//-------------------------------------------------------------------------------------------------

class UnixSocketProtocol
{
 private:
  string buffer;
  int current_pos;
  int current_line;
 public:
  bool Eat(const char* content,int length,vector<UnixSocketCommand>& uscs)
  {
   string s(content,content+length);
   return Eat(s,uscs);
  }
  bool Eat(const string& content,vector<UnixSocketCommand>& uscs);
 public:
  UnixSocketProtocol():current_pos(0),current_line(0)
  {}
};//end class UnixSocketProtocol
//-------------------------------------------------------------------------------------------------
#endif
