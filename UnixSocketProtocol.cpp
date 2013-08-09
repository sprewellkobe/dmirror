#include "UnixSocketProtocol.hpp"
//-------------------------------------------------------------------------------------------------

bool UnixSocketCommand::Do(string& result,COMMAND_FUNCTION unix_socket_command_function)
{
 result="";
 if(cmd=="test")
    result="ok"+NC_END_TOKEN;
 else if(cmd==UNIX_SOCKET_COMMAND_START_WATCHER||
         cmd==UNIX_SOCKET_COMMAND_START_SENDER1||
         cmd==UNIX_SOCKET_COMMAND_STOP_SENDER||
         cmd==UNIX_SOCKET_COMMAND_STOP_WATCHER||
         cmd==UNIX_SOCKET_COMMAND_CHANGE_SENDER_MODE_TO_UPDATE||
         cmd==UNIX_SOCKET_COMMAND_GET_SENDER_LAST_READ_TIME||
         cmd==UNIX_SOCKET_COMMAND_GET_WATCHER_STATUS||
         cmd==UNIX_SOCKET_COMMAND_GET_SENDER_STATUS)
   {
    unix_socket_command_function(cmd,result);
    result+=NC_END_TOKEN;
   }
 else
    result="unknown command "+cmd+NC_END_TOKEN;
 return true;
}
//-------------------------------------------------------------------------------------------------

bool UnixSocketProtocol::Eat(const string& content,vector<UnixSocketCommand>& uscs)
{
 buffer+=content;
 int ep=0;
 while(current_pos<int(buffer.size()))
      {
       ep=buffer.find(NC_END_TOKEN,current_pos);
       if(ep==int(string::npos))
          return false;
       vector<string> items;
       char tc=buffer[ep];
       buffer[ep]=0;
       const string& s=&(buffer[current_pos]);
       SplitString(s,items," \t");
       if(items.empty()==false)
         {
          UnixSocketCommand usc;
          usc.cmd=items[0];
          for(int i=1;i<int(items.size());i++)
              usc.params.push_back(items[i]);
          uscs.push_back(usc);
         }
       buffer[ep]=tc;
       current_pos=ep+NC_END_TOKEN.size();
      }//end while
 return true;
}
//-------------------------------------------------------------------------------------------------
