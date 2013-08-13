#include "HTTPProtocol.hpp"
//-------------------------------------------------------------------------------------------------

bool HTTPCommand::Do(string& result,COMMAND_FUNCTION http_command_function)
{
 result="HTTP/1.0 200 OK\r\nContent-Type: text/HTML\r\n\r\n";
 int pos=uri.find('?');//cmd?key=value
 string scmd=uri.substr(0,pos);
 vector<string> vec;
 map<string,string> args;
 if(pos==int(string::npos))
    scmd=uri;
 else
   {
    scmd=uri.substr(0,pos);
    SplitString(uri.substr(pos+1),vec,"&");
    for(int i=0;i<int(vec.size());i++)
       {
        vector<string> temp;
        SplitString(vec[i],temp,"=");
        if(temp.size()==2)
           args.insert(make_pair(temp[0],temp[1]));
       }
   }
 if(scmd.empty()||scmd[0]!='/')
   {
    result+="wrong command\r\n";
    return true;
   }
 scmd=scmd.substr(1);
 if(scmd=="test")
   {
    result+="ok\r\n";
    return true;
   }
 else if((scmd==HTTP_COMMAND_SET_MASTER||
         scmd==HTTP_COMMAND_SET_SLAVE||
         scmd==HTTP_COMMAND_GET_STATE||
         scmd==HTTP_COMMAND_GET_STATUS||
         scmd==HTTP_COMMAND_SHOW_CONF||
         scmd==HTTP_COMMAND_UPDATE_CONF)
         &&http_command_function!=NULL)
   {
    string res;
    http_command_function(scmd,res);
    result+=res+"\r\n";
    return true;
   }
 else
    result+="unknown command "+scmd+"\r\n";
 return true;
}
//-------------------------------------------------------------------------------------------------

bool HTTPProtocol::Eat(const string& content,vector<HTTPCommand>& hcs)
{
 buffer+=content;
 int ep=0;
 HTTPCommand hc;
 while(current_pos<int(buffer.size()))
      {
       ep=buffer.find(TELNET_END_TOKEN,current_pos);
       if(ep==int(string::npos))
          return false;
       vector<string> items;
       char tc=buffer[ep];
       buffer[ep]=0;
       const string& s=&(buffer[current_pos]);
       if(s.empty())
         {
          buffer[ep]=tc;
          hcs.push_back(hc);
          return true;
         }
       if(current_line==0)
         {
          vector<string> items;
          SplitString(s,items," \t");
          if(items.size()==3)
            {
             ToLower(items[0]);
             if(items[0]=="get")
               {
                hc.cmd=items[0];
                hc.uri=items[1];
               }
            }
         }
       else
         {
          string ts=": ";
          int tp=s.find(ts);
          if(tp!=int(string::npos))
            {
             string k=s.substr(0,tp);
             string v=s.substr(tp+ts.size());
             hc.headers.insert(make_pair(k,v));
            }
         }
       buffer[ep]=tc;
       current_pos=ep+TELNET_END_TOKEN.size();
       current_line++;
      }//end while
 return true;
}
//-------------------------------------------------------------------------------------------------
