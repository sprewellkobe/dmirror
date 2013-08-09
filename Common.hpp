#ifndef COMMONHPP
#define COMMONHPP
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <string>
#include <limits.h>
#include <iostream>
#include <vector>
#include <set>
#include "Maindef.hpp"
using namespace std;
//-------------------------------------------------------------------------------------------------
//typedef void (*VISIT_PATH_CALLBACK)(const string& filename);
class Watcher;
class WatcherStatus;
class Sender;
class SenderStatus;
class Conf;
void SplitString(const string& str,vector<string>& vec,const char* step=NULL);
//-------------------------------------------------------------------------------------------------
class FNSortHelper
{
 public:
  string filename;
  string filefullname;
  string sec;
  string usec;
 public:
  FNSortHelper()
  {}
  bool FromString(const string& fn)
  {
   filename=fn;
   vector<string> items;
   SplitString(fn,items,"_.");
   if(items.size()!=3)
      return false;
   sec=items[0];
   usec=items[1];
   return true;
  }
  friend bool operator<(const FNSortHelper& fnh1,const FNSortHelper& fnh2)
  {
   if(fnh1.sec==fnh2.sec)
      return fnh1.usec<fnh2.usec;
   return fnh1.sec<fnh2.sec;
  }
  friend bool operator>(const FNSortHelper& fnh1,const FNSortHelper& fnh2)
  {
   if(fnh1.sec==fnh2.sec)
      return fnh1.usec>fnh2.usec;
   return fnh1.sec>fnh2.sec;
  }
};//end class FNSortHelper
//-------------------------------------------------------------------------------------------------
string IntToStr(int v);
string FloatToStr(float v);
string ULLToStr(ULL v);
string GetCurrentTime(const unsigned char split=0);
string Timet2String(time_t timet);
bool FilePutContent(const string& filename,const string& content);
bool FileGetContent(const string& filename,string& content);
off_t GetFileSize(FILE* fp);
off_t GetFileSize(const string& filename);
bool FileExists(const string& filename);
void Trim(string& str,bool trimstring=false);
void ToLower(string& str);
bool SetNBSocket(int fd);
bool SetCESocket(int fd);
int CreateAndConnectUnixSocket(const string& usfn);
int CreateServerUnixSocket(const string& usfn);
bool VisitPath(const string& path,int& dir_count,Watcher* watcher);
bool ListFileFromPath(const string& path,vector<FNSortHelper>& files,const string& ext);
struct timeval BeginTiming();
double EndTiming(const struct timeval& bt);
char* EventType2String(int event_type);
int String2EventType(const string& str);
string ExtractFilename(const string& fullfilename);
string GetParentPath(string path);
bool NonblockSend(int fd,const char* buffer,unsigned int total_size);
bool UnixSocketBlockSend(int fd,const char* buffer,unsigned int total_size);
string StateToString(int state);
string BuildHTMLResult(const Conf& conf,int state,
                       const WatcherStatus& ws,const SenderStatus& ss);
//-------------------------------------------------------------------------------------------------

class AddrPair
{
 public:
  string host;
  unsigned short port;
  bool available;
 public:
  AddrPair():host(""),port(0),available(true)
  {}
  AddrPair(const string& h,unsigned short p):host(h),port(p),available(true)
  {}
  AddrPair(const AddrPair& ap)
  {
   host=ap.host;
   port=ap.port;
   available=ap.available;
  }
 public:
  bool IsEmpty() const
  {return host.empty();}
  string ToStr(bool fulldisplay=false) const
  {
   char temp[32];
   int a=available==true?1:0;
   if(fulldisplay)
      snprintf(temp,32,"%s:%d:%d",host.c_str(),port,a);
   else
      snprintf(temp,32,"%s:%d",host.c_str(),port);
   return temp;
  }
  bool FromStr(string s)
  {
   if(s.empty()==false&&(s[0]=='\''||s[0]=='"'))
      s.erase(0,1);
   if(s.empty()==false&&(s[s.size()-1]=='\''||s[s.size()-1]=='"'))
      s.erase(s.size()-1,1);
   vector<string> vec;
   SplitString(s,vec,":");
   if(vec.size()==2||vec.size()==3)
     {
      host=vec[0];
      port=atoi(vec[1].c_str());
      if(vec.size()==3)
         available=vec[2]=="1"?true:false;
      else
         available=true;
      return true;
     }
   return false;
  }
 public:
  friend bool operator<(const AddrPair& ap1,const AddrPair& ap2)
  {
   if(ap1.host==ap2.host)
      return ap1.port<ap2.port;
   return ap1.host<ap2.host;
  }
  friend bool operator>(const AddrPair& ap1,const AddrPair& ap2)
  {
   if(ap1.host==ap2.host)
      return ap1.port>ap2.port;
   return ap1.host>ap2.host;
  }
  friend bool operator==(const AddrPair& ap1,const AddrPair& ap2)
  {return ap1.host==ap2.host&&ap1.port==ap2.port;}
};//end class AddrPair
//-------------------------------------------------------------------------------------------------
#endif
