#ifndef SENDERHPP
#define SENDERHPP
#include <sys/inotify.h>
#include <string>
#include <iostream>
#include <vector>
#include "Maindef.hpp"
#include "Common.hpp"
#include "Conf.hpp"
#include "Rlog.hpp"
using namespace std;
//-------------------------------------------------------------------------------------------------
class SenderStatus
{
 public:
  ULL sent_file_number;
  string current_sender_read_rlog_filename;
  off_t current_sender_read_rlog_offset;
 public:
  Rlog* rlog;
 public:
  SenderStatus():sent_file_number(0),
                 current_sender_read_rlog_offset(0),
                 rlog(NULL)
  {}
  void Fresh()
  {
   if(rlog==NULL)
      return;
   current_sender_read_rlog_filename=rlog->current_filename;
   current_sender_read_rlog_offset=rlog->current_file_readed_offset;
  }
 public:
  string ToString()
  {
   return "1:"+ULLToStr(sent_file_number)+
          " 2:"+current_sender_read_rlog_filename+
          " 3:"+IntToStr(current_sender_read_rlog_offset);
  }
  bool FromString(const string& str)
  {
   vector<string> vec;
   SplitString(str,vec," \n");
   if(vec.size()!=3)
      return false;
   for(unsigned int i=0;i<vec.size();i++)
      {
       vector<string> vec2;
       SplitString(vec[i],vec2,":");
       if(vec2.size()!=2)
          continue;
       switch(atoi(vec2[0].c_str()))
             {
              case 1:
                   sent_file_number=atoll(vec2[1].c_str());
                   break;
              case 2:
                   current_sender_read_rlog_filename=vec2[1];
                   break;
              case 3:
                   current_sender_read_rlog_offset=atol(vec2[1].c_str());
                   break;
              default:
                   break;
             }
      }
   return true;
  }
};//end class SenderStatus
//-------------------------------------------------------------------------------------------------

class Sender
{
 public:
  Conf conf;
  bool update_mode;
  Rlog* rlog;
  SenderStat* senderstat;
  SenderStatus senderstatus;
 public:
  Sender(const Conf& c):conf(c),update_mode(false),
                        rlog(NULL),senderstat(NULL)
  {
   rlog=new Rlog(conf);
   senderstatus.rlog=rlog;
  }
  ~Sender()
  {
   if(rlog!=NULL)
      delete rlog;
   rlog=NULL;
   if(senderstat!=NULL)
      delete senderstat;
   senderstat=NULL;
  }
  void UpdateConf(const Conf& c)
  {
   conf=c;
   if(rlog!=NULL)
      rlog->UpdateConf(c);
   if(senderstat!=NULL)
      senderstat->UpdateConf(c);
  }
 private:
  Sender(const Sender& s){}
  bool MakeRsyncFileList(const vector<RlogItem>& items,
                         string& include_content,string& exclude_content,
                         int& include_item_number,int& exclude_item_number);
 public:
  void SetUpdateMode(bool c)
  {update_mode=c;}
 public:
  bool Send(const vector<RlogItem>& items,string& errmsg);
  bool SendAll(string& errmsg);
 //---------------------------------------------- 
};//end class Sender
//-------------------------------------------------------------------------------------------------
#endif
