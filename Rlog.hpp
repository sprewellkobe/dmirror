#ifndef RLOGHPP
#define RLOGHPP
#include <errno.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <iostream>
#include "Maindef.hpp"
#include "Common.hpp"
#include "Conf.hpp"
using namespace std;
//-------------------------------------------------------------------------------------------------
//sec_usec.rlog
//-------------------------------------------------------------------------------------------------

class SenderStat
{
 public:
  string current_filename;
  off_t offset;
 public:
  Conf conf;
  string filename;
  FILE* fp;
 public:
  SenderStat(const Conf& c):
             offset(0),conf(c),fp(NULL)
  {
   filename=c.stat_file_path+STAT_FILE_NAME;
  }
  ~SenderStat()
  {
   if(fp!=NULL)
      fclose(fp);
  }
  void UpdateConf(const Conf& c)
  {conf=c;}
 private:
  SenderStat(const SenderStat& ss)
  {}
 public:
  void Reset()//move to end of rlog
  {
   conf.Display();
   vector<FNSortHelper> helpers;
   if(ListFileFromPath(conf.rlog_path,helpers,RLOG_SURFIX)==false)
      return;
   if(helpers.empty())
      return;
   sort(helpers.begin(),helpers.end());
   const string& fn=helpers.back().filefullname;
   off_t fl=GetFileSize(fn);
   Save(fn,fl);
   if(fp!=NULL)
     {
      fclose(fp);
      fp=NULL;
     }
   kobe_printf("%s\tsenderstat reset %s %lu\n",GetCurrentTime().c_str(),fn.c_str(),fl);
  }
  //---------------------------------------------
  bool Load()
  {
   string content;
   if(FileGetContent(filename,content)==false)
      return false;
   vector<string> items;
   SplitString(content,items,"\t");
   if(items.size()!=2)
      return false;
   current_filename=items[0];
   offset=atol(items[1].c_str());
   return true;
  }
  //---------------------------------------------
  bool Save(const string& cf,ULL o)
  {
   if(cf.empty()==false&&current_filename.empty()==false&&
      cf!=current_filename)
     {
      kobe_printf("%s\t%s done,unlinked\n",GetCurrentTime().c_str(),
             current_filename.c_str());
      unlink(current_filename.c_str());//read rotate
     }
   current_filename=cf;
   offset=o;
   if(fp==NULL)
      fp=fopen(filename.c_str(),"w");
   if(fp==NULL)
      return false;
   fseek(fp,0,SEEK_SET);
   fprintf(fp,"%s\t%lu",current_filename.c_str(),offset);
   fflush(fp);
   return true;
  }
};//end class SenderStat
//-------------------------------------------------------------------------------------------------

class RlogItem
{
 public:
  string times;
  int event_type;
  string filename;
 public:
  RlogItem():event_type(0)
  {}
};//end class RlogItem
//-------------------------------------------------------------------------------------------------

class Rlog
{
 public:
  Conf conf;
  FILE* fp;
  string current_filename;
  off_t current_file_readed_offset;
  off_t current_file_wrotten_offset;
  time_t last_read_time;
  int safe_mode;//danger rotate, cause dir not sync
 public:
  Rlog(const Conf& c):conf(c),fp(NULL),
                      current_file_readed_offset(0),current_file_wrotten_offset(0),
                      last_read_time(time(NULL)),safe_mode(SYNC_SAFE)
  {}
  void UpdateConf(const Conf& c)
  {conf=c;}
 private:
  Rlog(const Rlog& r){}
 private:
  //---------------------------------------------
  FILE* FindProperFileToRead(const SenderStat& ss)
  {
   if(ss.current_filename.empty()==false&&ss.offset<GetFileSize(ss.current_filename))
     {
      current_filename=ss.current_filename;
      fp=fopen(ss.current_filename.c_str(),"r");
      fseek(fp,ss.offset,SEEK_SET);
      kobe_printf("%s\tcontinue read from %s\n",GetCurrentTime().c_str(),
                  ss.current_filename.c_str());
      return fp;
     }
   //new file
   vector<FNSortHelper> helpers;
   if(ListFileFromPath(conf.rlog_path,helpers,RLOG_SURFIX)==false)
      return NULL;
   if(helpers.empty())
      return NULL;
   sort(helpers.begin(),helpers.end());
   FNSortHelper fnhelper;
   fnhelper.filefullname=ss.current_filename;
   fnhelper.FromString(ExtractFilename(ss.current_filename));
   unsigned int i=0;
   for(;i<helpers.size();i++)
      {
       if(helpers[i]>fnhelper)
          break;
      }
   if(i>=helpers.size())
      return NULL;
   current_filename=helpers[i].filefullname;
   fp=fopen(current_filename.c_str(),"r");
   kobe_printf("%s\tfirst read from %s\n",GetCurrentTime().c_str(),
          current_filename.c_str());
   return fp;
  }
  //---------------------------------------------
  FILE* FindProperFileToWrite()
  {
   vector<FNSortHelper> helpers;
   if(ListFileFromPath(conf.rlog_path,helpers,RLOG_SURFIX)==false)
      return NULL;
   sort(helpers.begin(),helpers.end());
   if(helpers.empty()||
      (uint64_t)(GetFileSize(helpers.back().filefullname))>=conf.rlog_file_max_size)
     {
      struct timeval tv;
      memset(&tv,sizeof(tv),0);
      gettimeofday(&tv,NULL);
      string s1=Timet2String(tv.tv_sec);
      char s2[8];
      snprintf(s2,8,"%07u",(unsigned int)(tv.tv_usec));
      string filename=conf.rlog_path+s1+"_"+string(s2)+RLOG_SURFIX;
      fp=fopen(filename.c_str(),"a");
      if(fp==NULL)
         kobe_printf("%s\tERROR: failed to open %s,%d\n",
                     GetCurrentTime().c_str(),filename.c_str(),errno);
      else
        {
         current_filename=filename;
         kobe_printf("%s\tfirst write to %s\n",GetCurrentTime().c_str(),
                     current_filename.c_str());
         if(helpers.size()+1>=(unsigned int)(conf.rlog_max_file_number))
           {
            unlink(helpers.front().filefullname.c_str());//rotate
            kobe_printf("%s\tWARNING: rlog so full that unlink %s to rotate, maybe some update lost!\n",
                        GetCurrentTime().c_str(),helpers.front().filefullname.c_str());
            safe_mode=SYNC_DANGER_RLOG_OVERFLOW;
           }
        }
      return fp;
     }
   current_filename=helpers.back().filefullname;
   fp=fopen(current_filename.c_str(),"a");
   if(fp==NULL)
      kobe_printf("%s\tERROR: failed to open %s,%d\n",
                  GetCurrentTime().c_str(),current_filename.c_str(),errno);
   else
      kobe_printf("%s\tcontinue write to %s\n",GetCurrentTime().c_str(),
                  current_filename.c_str());
   return fp;
  }
  //---------------------------------------------
 public:
  bool Read(const SenderStat& ss,vector<RlogItem>& items,int& err)
  {
   if(fp==NULL)
      fp=FindProperFileToRead(ss);
   if(fp==NULL)
     {
      err=-1;
      return false;
     }
   last_read_time=time(NULL);
   int i=0;
   static char buffer[4096];
   while(i++<conf.rlog_reader_batch_item_number)
        {
         char* s=fgets(buffer,4096,fp);
         if(s==NULL)
           {
            if(feof(fp))
              {
               current_file_readed_offset=ftell(fp);
               fclose(fp);
               fp=NULL;
               return true;
              }
            err=errno;
            return false;
           }
         buffer[4095]=0;
         vector<string> tokens;
         SplitString(buffer,tokens,"\t\n");
         if(tokens.size()!=3)
           {
            kobe_printf("%s\tERROR: wrong format %s\n",GetCurrentTime().c_str(),buffer);
            continue;
           }
         RlogItem ri;
         ri.times=tokens[0];
         ri.event_type=String2EventType(tokens[1]);
         ri.filename=tokens[2];
         items.push_back(ri);
         if(time(NULL)-last_read_time>RLOG_BATCH_PROCESS_TIMEOUT)//force batch process return
            break;
        }//end while
   current_file_readed_offset=ftell(fp);
   return true;
  }
  //---------------------------------------------
  bool Write(int event_type,const string& filename,int& err)
  {
   if(fp==NULL)
      fp=FindProperFileToWrite();
   if(fp==NULL)
     {
      err=-1;
      kobe_printf("no file to write\n");
      return false;
     }
   int rv=fprintf(fp,"%s\t%s\t%s\n",GetCurrentTime().c_str(),
                  EventType2String(event_type),filename.c_str());
   //#ifdef MYDEBUG
   //kobe_printf("%s\twrote2 %d %s %s\n",GetCurrentTime().c_str(),event_type,
   //             EventType2String(event_type),filename.c_str());
   //#endif
   if(rv<0)
     {
      err=errno;
      return false;
     }
   fflush(fp);
   current_file_wrotten_offset=ftell(fp);
   if(GetFileSize(fp)>=(long)(conf.rlog_file_max_size))
     {
      fclose(fp);
      fp=NULL;
      kobe_printf("%s\tclosed full file %s\n",
                  GetCurrentTime().c_str(),current_filename.c_str());
     }
   return true;
  }
};//end class Rlog
//-------------------------------------------------------------------------------------------------
#endif
