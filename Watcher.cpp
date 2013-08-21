#include <dirent.h>
#include "Watcher.hpp"
//-------------------------------------------------------------------------------------------------

void pipe_read_handler(aeEventLoop* el,int fd,void* arg,int mask)
{
 Watcher* watcher=(Watcher*)arg;
 char c;
 int rv=read(fd,&c,1);
 if(rv!=1)
    return;
 watcher->OnPipeRead();
}
//-------------------------------------------------------------------------------------------------

void Watcher::OnPipeRead()
{
 if(watch_dir_list.empty()==true)
    return;
 const string& path=watch_dir_list.front();
 if(path.empty()==false&&path[path.size()-1]=='/')
    OnScanDir(path);
 else
    OnScanDir(path+"/");
 watch_dir_count++;
 struct dirent** list=NULL;
 int ne=scandir(path.c_str(),&list,NULL,NULL);
 if(ne<0)
    return;
 for(int i=0;i<ne;i++)
    {
     if(strcmp(list[i]->d_name,".")==0||
        strcmp(list[i]->d_name,"..")==0)
       {
        free(list[i]);
        list[i]=NULL;
        continue;
       }
     if(list[i]->d_type==DT_DIR)
       {
        static char buf[4096];
        if(path[path.size()-1]!='/')
           snprintf(buf,4095,"%s/%s",path.c_str(),list[i]->d_name);
        else
           snprintf(buf,4095,"%s%s",path.c_str(),list[i]->d_name);
        watch_dir_list.push_back(buf);
        //write(pipe_pair[1],"x",1);
       }
     free(list[i]);
     list[i]=NULL;
    }//end for i
 free(list);
 watch_dir_list.pop_front();
 if(watch_dir_list.empty())
   {
    kobe_printf("%s\tprepare finished(%d), dir_count:%d, time_cost: %.3f sec\n",
                 GetCurrentTime().c_str(),int(addwatch),watch_dir_count,EndTiming(watch_init_tv));
    aeDeleteFileEvent(main_el,pipe_pair[0],AE_READABLE);
    close(pipe_pair[0]);
    close(pipe_pair[1]);
    pipe_pair[0]=0;
    pipe_pair[1]=0;
   }
 else
    write(pipe_pair[1],"x",1);
}
//-------------------------------------------------------------------------------------------------

bool Watcher::Prepare(int& err)
{
 if(inotify_fd>0)
   {
    close(inotify_fd);
    inotify_fd=0;
   }
 wd2path.clear();
 path2wd.clear();
 if(inotify_fd<=0)
   {
    inotify_fd=inotify_init();
    if(inotify_fd==-1)
      {
       kobe_printf("%s\tERROR: failed to init inotify %d\n",GetCurrentTime().c_str(),errno);
       return false;
      }
   }
 addwatch=true;
 watch_init_tv=BeginTiming();
 watch_dir_count=0;
 watch_dir_list.clear();
 if(pipe(pipe_pair)!=0)
   {
    err=errno;
    return false;
   }

 int rv=aeCreateFileEvent(main_el,pipe_pair[0],AE_READABLE,pipe_read_handler,this);
 if(rv<0)
   {
    kobe_printf("%s\tERROR: failed to create pipe file event [%d]\n",
                GetCurrentTime().c_str(),rv);
    return false;
   }

 watch_dir_list.push_back(conf.local_dir);
 write(pipe_pair[1],"x",1);
 kobe_printf("%s\twatcher preparing...\n",GetCurrentTime().c_str());
 /*
 if(VisitPath(conf.local_dir,dir_count,this)==false)
    return false;
 if(addwatch==false)
    return false;
 kobe_printf("%s\tprepare finished, dir_count:%d, time_cost: %.3f sec\n",
        GetCurrentTime().c_str(),dir_count,EndTiming(tv));
 */
 return true;
}
//-------------------------------------------------------------------------------------------------

void Watcher::OnScanDir(const string& dirname)
{
 int ret=inotify_add_watch(inotify_fd,dirname.c_str(),INOTIFY_EVENTS);
 if(ret==-1)
   {
    kobe_printf("%s\tERROR: inotify_add_watch failed %s,%d,%d\n",
           GetCurrentTime().c_str(),dirname.c_str(),inotify_fd,errno);
    addwatch=false;
   }
 /*#ifdef MYDEBUG
 kobe_printf("%s\tadd new watch %s,%d,%d\n",GetCurrentTime().c_str(),
        dirname.c_str(),inotify_fd,ret);
 #endif*/
 watcherstatus.watch_wd_number++; 
 wd2path[ret]=dirname.substr(conf.local_dir.size());//to save memory
 //path2wd[dirname]=ret;
}
//-------------------------------------------------------------------------------------------------

bool Watcher::Start(aeEventLoop* main_el)
{
 if(main_el==NULL)
    return false;
 if(inotify_fd<=0)
   {
    inotify_fd=inotify_init();
    if(inotify_fd==-1)
      {
       kobe_printf("%s\tERROR: failed to init inotify %d\n",GetCurrentTime().c_str(),errno);
       return false;
      }
   }
 aeCreateFileEvent(main_el,inotify_fd,AE_READABLE,on_inotify_read,this);
 return true;
}
//-------------------------------------------------------------------------------------------------

bool Watcher::Stop(aeEventLoop* main_el)
{
 if(main_el==NULL||inotify_fd<=0)
    return false;
 aeDeleteFileEvent(main_el,inotify_fd,AE_READABLE);
 close(inotify_fd);
 inotify_fd=0;
 wd2path.clear();
 path2wd.clear();
 return true;
}
//-------------------------------------------------------------------------------------------------

void Watcher::OnInotifyRead(int fd)
{
 static char buffer[INOTIFY_READ_BUFFER_SIZE];
 ssize_t rl=read(fd,buffer,INOTIFY_READ_BUFFER_SIZE);
 if(rl<=0)
    return;
 ssize_t buffer_index=0;
 while(buffer_index<rl)
      {
       struct inotify_event* ievent=NULL;
       ievent=(inotify_event*)&(buffer[buffer_index]);
       DoEvent(ievent);
       buffer_index+=sizeof(struct inotify_event)+ievent->len;
      }//end while
}
//-------------------------------------------------------------------------------------------------

bool Watcher::AddDir(const string& fullname)
{
 int dir_count=0;
 if(VisitPath(fullname,dir_count,this)==false)
    return false;
 return true;
}
//-------------------------------------------------------------------------------------------------

bool Watcher::RemoveDir(const string& fullname)
{
 //delete automatically
 /*map<string,int>::iterator mi2=path2wd.find(fullname);
 if(mi2==path2wd.end())
   {
    kobe_printf("%s\tERROR: RemoveDir failed cannot find %s\n",
           GetCurrentTime().c_str(),fullname.c_str());
    return false;
   }
 int ret=inotify_rm_watch(inotify_fd,mi2->second);
 if(ret==-1)           
   {                   
    kobe_printf("%s\tERROR: inotify_rm_watch %s failed %d,%d,%d\n",
           GetCurrentTime().c_str(),fullname.c_str(),errno,inotify_fd,mi2->second);
    return false;      
   }
 #ifdef MYDEBUG
 kobe_printf("%s\trm existed watch %s\n",GetCurrentTime().c_str(),fullname.c_str());
 #endif
 map<int,string>::iterator mi=wd2path.find(mi2->second);
 if(mi!=wd2path.end())
    wd2path.erase(mi);
 path2wd.erase(mi2);*/
 watcherstatus.watch_wd_number--;
 return true;
}
//-------------------------------------------------------------------------------------------------

bool Watcher::DoEvent(struct inotify_event* ievent)
{
 bool isdir=false;
 if(ievent->len<=0)
    return false;
 string fullname=ievent->name;
 int wd=ievent->wd;
 if(ievent->mask&IN_ISDIR)
   {
    isdir=true;
    fullname+="/";
   }
 if(ievent->mask&IN_Q_OVERFLOW)
    rlog->safe_mode=SYNC_DANGER_INOTIFY_OVERFLOW;
 map<int,string>::iterator mi=wd2path.find(wd);
 if(mi==wd2path.end())
   {
    kobe_printf("%s\tERROR: Doevent cannot find %d\n",GetCurrentTime().c_str(),wd);
    return false;
   }
 fullname=conf.local_dir+mi->second+fullname;
 int err=0;
 switch(ievent->mask&INOTIFY_EVENTS)
       {
        case IN_CREATE:
             #ifdef MYDEBUG
             kobe_printf("%s\tcreate %s,%d\n",
                    GetCurrentTime().c_str(),fullname.c_str(),ievent->wd);
             #endif
             if(isdir&&AddDir(fullname)==false)
                return false;
             rlog->Write(IN_CREATE,fullname,err);
             break;
        case IN_DELETE:
             #ifdef MYDEBUG
             kobe_printf("%s\tdelete %s\n",GetCurrentTime().c_str(),fullname.c_str());
             #endif
             if(isdir&&RemoveDir(fullname)==false)
                return false;
             rlog->Write(IN_DELETE,fullname,err);
             break;
        case IN_DELETE_SELF:
             #ifdef MYDEBUG
             kobe_printf("%s\tdelete_self %s\n",GetCurrentTime().c_str(),fullname.c_str());
             #endif
             rlog->Write(IN_DELETE_SELF,fullname,err);
             break;
        case IN_CLOSE_WRITE:
        //case IN_MODIFY:
             #ifdef MYDEBUG
             kobe_printf("%s\tmodified %s,%d\n",
                    GetCurrentTime().c_str(),fullname.c_str(),ievent->wd);
             #endif
             rlog->Write(IN_CLOSE_WRITE,fullname,err);
             break;
        case IN_MOVE_SELF:
             #ifdef MYDEBUG
             kobe_printf("%s\tmove_self %s\n",GetCurrentTime().c_str(),fullname.c_str());
             #endif
             rlog->Write(IN_MOVE_SELF,fullname,err);
             break;
        case IN_MOVED_FROM:
             #ifdef MYDEBUG
             kobe_printf("%s\tmoved_from %s\n",GetCurrentTime().c_str(),fullname.c_str());
             #endif
             if(isdir&&RemoveDir(fullname)==false)
                return false;
             rlog->Write(IN_MOVED_FROM,fullname,err);
             break;
        case IN_MOVED_TO:
             if(isdir&&AddDir(fullname)==false)
               {
                #ifdef MYDEBUG
                kobe_printf("%s\tERROR: failed to add %s %d\n",GetCurrentTime().c_str(),
                            fullname.c_str(),errno);
                #endif
                return false;
               }
             rlog->Write(IN_MOVED_TO,fullname,err);
             break;
        default:
             #ifdef MYDEBUG
             kobe_printf("%s\tunknown event_type\n",GetCurrentTime().c_str());
             #endif
             break;
       }//end switch
 return true;
}
//-------------------------------------------------------------------------------------------------

void on_inotify_read(aeEventLoop* main_el,int fd,void* arg,int mask)
{
 if(arg==NULL)
    return;
 Watcher* watcher=(Watcher*)arg;
 watcher->OnInotifyRead(fd);
}
//-------------------------------------------------------------------------------------------------
