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
 public:
  Conf():http_server_port(0),
         rlog_file_max_size(0),rlog_max_file_number(0),
         rsync_bwlimit(0),rlog_reader_batch_item_number(0)
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
   rsync_bwlimit=atoi(s.c_str());
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
   return true;
  }//end LoadFromFile
  void Display()
  {
   cout<<"///------------------------------------"<<endl;
   cout<<"http_server_port:"<<http_server_port<<endl;
   cout<<"mainbase_unix_socket_path:"<<mainbase_unix_socket_path<<endl;
   cout<<"remote_pair:"<<remote_pair.host.c_str()<<":"<<remote_pair.port<<endl;
   //--------------------------------------------
   cout<<"watcher_unix_socket_path:"<<watcher_unix_socket_path<<endl;
   cout<<"local_dir:"<<local_dir<<endl;
   cout<<"rlog_path:"<<rlog_path<<endl;
   cout<<"rlog_file_max_size:"<<rlog_file_max_size<<endl;
   cout<<"rlog_max_file_number:"<<rlog_max_file_number<<endl;
   //--------------------------------------------
   cout<<"sender_unix_socket_path:"<<sender_unix_socket_path<<endl;
   cout<<"sender_tmp_path:"<<sender_tmp_path<<endl;
   cout<<"remote_dir:"<<remote_dir<<endl;
   cout<<"rsync_bwlimit:"<<rsync_bwlimit<<endl;
   cout<<"rlog_reader_batch_item_number:"<<rlog_reader_batch_item_number<<endl;
   cout<<"stat_file_path:"<<stat_file_path<<endl;
   cout<<"///------------------------------------"<<endl;
  }
};//end class conf
//-------------------------------------------------------------------------------------------------
#endif
