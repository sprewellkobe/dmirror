#ifndef HTTPPROTOCOLHPP
#define HTTPPROTOCOLHPP
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include "Maindef.hpp"
#include "Common.hpp"
using namespace std;
//-------------------------------------------------------------------------------------------------

class HTTPCommand
{
 public:
  string cmd;
  string uri;
  map<string,string> headers;
  string body;
 public:
  //---------------------------------------------
  HTTPCommand()
  {
  }
  bool IsEmpty()
  {return cmd.empty();}
  void Display()
  {
   printf("%s\thttp=>%s %s\n",GetCurrentTime().c_str(),cmd.c_str(),uri.c_str());
   map<string,string>::iterator mi=headers.begin();
   for(;mi!=headers.end();mi++)
       printf("%s => %s\n",mi->first.c_str(),mi->second.c_str());
   printf("\n");
  }
  string ToStr()
  {return cmd+" "+uri;}
 public:
  bool Do(string& result,COMMAND_FUNCTION http_command_function);
};//end class HTTPCommand
//-------------------------------------------------------------------------------------------------

class HTTPProtocol
{
 private:
  string buffer;
  int current_pos;
  int current_line;
 public:
  bool Eat(const char* content,int length,vector<HTTPCommand>& hcs)
  {
   string s(content,content+length);
   return Eat(s,hcs);
  }
  bool Eat(const string& content,vector<HTTPCommand>& hcs);
 public:
  HTTPProtocol():current_pos(0),current_line(0)
  {}
};//end class HTTPProtocol
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#endif
