#ifndef CONFHPP
#define CONFHPP
#include <string>
#include "IniParser.hpp"
using namespace std;
//-------------------------------------------------------------------------------------------------

class Conf
{
 public://mainbase
  unsigned short http_server_port;
  string mainbase_unix_socket_path;
  AddrPair remote_pair;
 public://watcher
  string watcher_unix_socket_path;
  string local_dir;
  string rlog_path;
  uint64_t rlog_file_max_size;
  int rlog_max_file_number;
 public://sender
  string sender_unix_socket_path;
  string sender_tmp_path;
  string remote_dir;
  unsigned int rsync_bwlimit;
  int rlog_reader_batch_item_number;
  string stat_file_path;
 public://log
  bool log_switch;
  string log_path;
 public:
  Conf():http_server_port(0),
         rlog_file_max_size(0),rlog_max_file_number(0),
         rsync_bwlimit(0),rlog_reader_batch_item_number(0),
         log_switch(false)
  {}
 public:
  bool LoadFromFile(const string& filename,string& errmsg)
  {
   IniParser inip;
   if(inip.ReadFromFile(filename)==false)
     {
      errmsg="failed to load file "+filename;
      return false;
     }
   string s;
   if(inip.Get("mainbase","http_server_port",s)==false)
     {
      errmsg="failed to load item http_server_port";
      return false;
     }
   http_server_port=atoi(s.c_str());
   if(inip.Get("mainbase","mainbase_unix_socket_path",s)==false)
     {
      errmsg="failed to load item mainbase_unix_socket_path";
      return false;
     }
   mainbase_unix_socket_path=s;
   if(inip.Get("mainbase","remote_pair",s)==false||remote_pair.FromStr(s)==false)
     {
      errmsg="failed to load item remote_pair";
      return false;
     }
   if(inip.Get("watcher","watcher_unix_socket_path",s)==false)
     {
      errmsg="failed to load item watcher_unix_socket_path";
      return false;
     }
   watcher_unix_socket_path=s;
   if(inip.Get("watcher","local_dir",s)==false)
     {
      errmsg="failed to load item local_dir";
      return false;
     }
   local_dir=s;
   if(inip.Get("watcher","rlog_path",s)==false)
     {
      errmsg="failed to load item rlog";
      return false;
     }
   rlog_path=s;
   if(inip.Get("watcher","rlog_file_max_size",s)==false)
     {
      errmsg="failed to load item rlog_file_max_size";
      return false;
     }
   rlog_file_max_size=atol(s.c_str());
   if(inip.Get("watcher","rlog_max_file_number",s)==false)
     {
      errmsg="failed to load item rlog_max_file_number";
      return false;
     }
   rlog_max_file_number=atoi(s.c_str());
   if(inip.Get("sender","sender_unix_socket_path",s)==false)
     {
      errmsg="failed to load item sender_unix_socket_path";
      return false;
     }
   sender_unix_socket_path=s;
   if(inip.Get("sender","sender_tmp_path",s)==false)
     {
      errmsg="failed to load item sender_tmp_path";
      return false;
     }
   sender_tmp_path=s;
   if(inip.Get("sender","remote_dir",s)==false)
     {
      errmsg="failed to load item remote_dir";
      return false;
     }
   remote_dir=s;
   if(inip.Get("sender","rsync_bwlimit",s)==false)
     {
      errmsg="failed to load item rsync_bwlimit";
      return false;
     }
   unsigned int an=1;
   ToLower(s);
   if(s.empty()==false&&(s[s.size()-1]=='m'||s[s.size()-1]=='k'))
     {
      an=1024;
      if(s[s.size()-1]=='m')
         an*=1024;
      s.erase(s.size()-1);
     }
   rsync_bwlimit=atoi(s.c_str())*an;
   if(inip.Get("sender","rlog_reader_batch_item_number",s)==false)
     {
      errmsg="failed to load item rlog_reader_batch_item_number";
      return false;
     }
   rlog_reader_batch_item_number=atoi(s.c_str());
   if(inip.Get("sender","stat_file_path",s)==false)
     {
      errmsg="failed to load item stat_file_path";
      return false;
     }
   stat_file_path=s;
   if(inip.Get("log","log_switch",s)==false)
     {
      errmsg="failed to load item log_switch";
      return false;
     }
   log_switch=s=="on";
   if(inip.Get("log","log_path",s)==false)
     {
      errmsg="failed to load item log_path";
      return false;
     }
   log_path=s;
   return true;
  }//end LoadFromFile
  string ToString()
  {
   string str;
   str+="http_server_port:"+IntToStr(http_server_port)+"\n";
   str+="mainbase_unix_socket_path:"+mainbase_unix_socket_path+"\n";
   str+="remote_pair:"+remote_pair.host+":"+IntToStr(remote_pair.port)+"\n";
   //--------------------------------------------
   str+="watcher_unix_socket_path:"+watcher_unix_socket_path+"\n";
   str+="local_dir:"+local_dir+"\n";
   str+="rlog_path:"+rlog_path+"\n";
   str+="rlog_file_max_size:"+IntToStr(rlog_file_max_size)+"\n";
   str+="rlog_max_file_number:"+IntToStr(rlog_max_file_number)+"\n";
   //--------------------------------------------
   str+="sender_unix_socket_path:"+sender_unix_socket_path+"\n";
   str+="sender_tmp_path:"+sender_tmp_path+"\n";
   str+="remote_dir:"+remote_dir+"\n";
   str+="rsync_bwlimit:"+IntToStr(rsync_bwlimit)+"\n";
   str+="rlog_reader_batch_item_number:"+IntToStr(rlog_reader_batch_item_number)+"\n";
   str+="stat_file_path:"+stat_file_path+"\n";
   //--------------------------------------------
   str+="log_switch:"+IntToStr(int(log_switch))+"\n";
   str+="log_path:"+log_path+"\n";
   return str;
  }
  void Display()
  {
   cout<<"//------------------------------------"<<endl;
   cout<<ToString()<<endl;
   cout<<"//------------------------------------"<<endl;
  }
};//end class conf
//-------------------------------------------------------------------------------------------------
#endif
