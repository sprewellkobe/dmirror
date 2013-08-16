#ifndef WATCHERHPP
#define WATCHERHPP
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <deque>
#include <sys/inotify.h>
#include "ae.h"
#include "Maindef.hpp"
#include "Common.hpp"
#include "Conf.hpp"
#include "Rlog.hpp"
using namespace std;
//-------------------------------------------------------------------------------------------------

const static uint32_t INOTIFY_EVENTS=IN_CREATE|IN_DELETE|
                                     //IN_DELETE_SELF|
                                     //IN_MODIFY|
                                     IN_CLOSE_WRITE|
                                     IN_MOVE_SELF|
                                     IN_MOVED_FROM|IN_MOVED_TO;
//-------------------------------------------------------------------------------------------------

class WatcherStatus
{
 public:
  ULL watch_wd_number;
  string safe;
  string current_watcher_write_rlog_filename;
  off_t current_watcher_write_rlog_offset;
 public:
  Rlog* rlog;
 public:
  WatcherStatus():watch_wd_number(0),safe("true"),
                  current_watcher_write_rlog_offset(0),rlog(NULL)
  {}
  void Fresh()
  {
   if(rlog==NULL)
      return;
   if(rlog->danger)
     {
      safe="false";
     }
   else
      safe="true";
   current_watcher_write_rlog_filename=rlog->current_filename;
   current_watcher_write_rlog_offset=rlog->current_file_wrotten_offset;
  }
 public:                                                                           
  string ToString()                                                                
  {
   return "1:"+ULLToStr(watch_wd_number)+
          " 2:"+safe+
          " 3:"+current_watcher_write_rlog_filename+
          " 4:"+IntToStr(current_watcher_write_rlog_offset);
  }
  bool FromString(const string& str)
  {
   vector<string> vec;
   SplitString(str,vec," \n");
   for(unsigned int i=0;i<vec.size();i++)
      {
       vector<string> vec2;
       SplitString(vec[i],vec2,":");
       if(vec2.size()!=2)
          continue;
       switch(atoi(vec2[0].c_str()))
             {
              case 1:
                   watch_wd_number=atoll(vec2[1].c_str());
                   break;
              case 2:
                   safe=vec2[1];
                   break;
              case 3:
                   current_watcher_write_rlog_filename=vec2[1];
                   break;
              case 4:
                   current_watcher_write_rlog_offset=atol(vec2[1].c_str());
                   break;
              default:
                   break;
             }
      }
   return true;
  }
};//end class WatcherStatus
//-------------------------------------------------------------------------------------------------

class Watcher
{
 public:
  Conf conf;
  aeEventLoop* main_el;
  int inotify_fd;
  bool addwatch; 
  deque<string> watch_dir_list;
  int pipe_pair[2];
  struct timeval watch_init_tv;
  unsigned int watch_dir_count; 
 public:
  Rlog* rlog;
  WatcherStatus watcherstatus;
 public:
  map<int,string> wd2path;
  map<string,int> path2wd;
 public:
  Watcher(const Conf& c,aeEventLoop* me):
                         conf(c),main_el(me),inotify_fd(0),
                         addwatch(true),
                         watch_dir_count(0),
                         rlog(NULL)
  {
   pipe_pair[0]=0;
   pipe_pair[1]=0;
   rlog=new Rlog(conf);
   watcherstatus.rlog=rlog;
  }
  ~Watcher()
  {
   if(pipe_pair[0]!=0)
     {
      close(pipe_pair[0]);
      pipe_pair[0]=0;
     }
   if(pipe_pair[1]!=0)
     {
      close(pipe_pair[1]);
      pipe_pair[1]=0;
     }
   if(rlog!=NULL)
      delete rlog;
   rlog=NULL;
  }
 private:
  Watcher(const Watcher& w){}
 public:
  void OnPipeRead();
  bool Prepare(int& err);
  bool Start(aeEventLoop* main_el);
  bool Stop(aeEventLoop* main_el);
 public:
  void OnScanDir(const string& dirname); 
  void OnInotifyRead(int fd);
 public:
  bool AddDir(const string& fullname);
  bool RemoveDir(const string& fullname);
 private:
  bool DoEvent(struct inotify_event* ievent);
};//end class Watcher
//-------------------------------------------------------------------------------------------------

void on_inotify_read(aeEventLoop* el,int fd,void* arg,int mask);
//-------------------------------------------------------------------------------------------------
#endif
