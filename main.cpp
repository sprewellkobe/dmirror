#include <string>
#include <vector>
#include <iostream>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <sys/inotify.h>
#include <stddef.h>
#include "ae.h"
#include "Maindef.hpp"
#include "Common.hpp"
#include "MyCurl.hpp"
#include "Conf.hpp"
#include "Watcher.hpp"
#include "Sender.hpp"
#include "HTTPProtocol.hpp"
#include "UnixSocketProtocol.hpp"
using namespace std;
//-------------------------------------------------------------------------------------------------

class ProcessBase
{
 public:
  int mainbase_pid;
  int watcher_pid;
  int sender_pid;
 public:
  ProcessBase():mainbase_pid(0),watcher_pid(0),sender_pid(0)
  {}
};//end class ProcessBase
//-------------------------------------------------------------------------------------------------
int mainbase_loop_timer_handler(struct aeEventLoop* eventLoop, long long id, void* clientData);
bool HTTPCommandFunction(const string& command,string& res);
bool UnixSocketCommandFunction(const string& command,string& res);
//-------------------------------------------------------------------------------------------------

static ProcessBase processbase;
static Conf conf;
static aeEventLoop* main_el=NULL;
//-----------------------------------------------
static int unix_socket_server_fd=0;
static int http_server_fd=0;
static int process_monitor_timer_id=0;
static int mainbase_loop_timer_id=0;
//static int watcher_loop_timer_id=0;
static int sender_loop_timer_id=0;

static int my_argc=0;
static char** my_argv=NULL;
static int my_role=0;
static int my_state=STATE_NULL;

static Watcher* watcher=NULL;
static Sender* sender=NULL;
//-------------------------------------------------------------------------------------------------

class SocketReceiver
{
 public:
  int client_sockfd;
  int protocol_type;
 public:
  AddrPair client_addr;
  string client_filename;
  HTTPProtocol httpprotocol;
  UnixSocketProtocol unixsocketprotocol;
 public:
  SocketReceiver(int fd,int pt):
                 client_sockfd(fd),protocol_type(pt)
  {}
  ~SocketReceiver()
  {Free();}
  void Free()
  {
   if(client_sockfd>0)
     {
      aeDeleteFileEvent(main_el,client_sockfd,AE_READABLE);
      close(client_sockfd);
     }
   client_sockfd=0;
  }
 public:
  void* GetProtocol()
  {
   switch(protocol_type)
         {
          case HTTP_PROTOCOL:
               return &httpprotocol;
          case UNIX_SOCKET_PROTOCOL:
               return &unixsocketprotocol;
          default:
               break;
         }
   return NULL;
  }
};//end class SocketReceiver
//-------------------------------------------------------------------------------------------------

void DisplayUsage(bool rv=false)
{
 cout<<"USAGE"<<endl;
 cout<<"    "<<EXENAME<<" mainbase ini_filename"<<endl;
 cout<<"AUTHORS"<<endl;
 cout<<"    kobe 20130724"<<endl;
 #ifdef BUILDVERSION
 cout<<"BUILDVERSION"<<endl;
 cout<<"    "<<BUILDVERSION<<endl;
 #endif
 if(rv) exit(0);
}
//-------------------------------------------------------------------------------------------------

static FILE* mainbase_log_fp=NULL;
static FILE* watcher_log_fp=NULL;
static FILE* sender_log_fp=NULL;
void kobe_printf(const char* format,...)//worker log wrapper
{
 va_list args;
 va_start(args,format);
 if(conf.log_switch==true)
   {
    FILE* fp=NULL;
    switch(my_role)
          {
           case ROLE_MAINBASE:
                if(mainbase_log_fp==NULL)
                  {
                   string fn=conf.log_path+MAINBASE_LOG_FILE_NAME+GetCurrentTime()+LOG_SURFIX;
                   mainbase_log_fp=fopen(fn.c_str(),"w");
                  }
                fp=mainbase_log_fp;
                break;
           case ROLE_WATCHER:
                if(watcher_log_fp==NULL)
                  {
                   string fn=conf.log_path+WATCHER_LOG_FILE_NAME+GetCurrentTime()+LOG_SURFIX;
                   watcher_log_fp=fopen(fn.c_str(),"w");
                  }
                fp=watcher_log_fp;
                break;
           case ROLE_SENDER:
                if(sender_log_fp==NULL)
                  {
                   string fn=conf.log_path+SENDER_LOG_FILE_NAME+GetCurrentTime()+LOG_SURFIX;
                   sender_log_fp=fopen(fn.c_str(),"w");
                  }
                fp=sender_log_fp;
                break;
           default:
                break;
          }//end switch
    if(fp!=NULL)
       vfprintf(fp,format,args);
    va_end(args);
    if(fp!=NULL&&ftell(fp)>LOG_FILE_MAX_SIZE)
      {
       fclose(fp);
       if(my_role==ROLE_MAINBASE)
          mainbase_log_fp=NULL;
       else if(my_role==ROLE_WATCHER)
          watcher_log_fp=NULL;
       else
          sender_log_fp=NULL;
      }
   }
 else
   {
    vprintf(format,args);
    va_end(args);
   }
}
//-------------------------------------------------------------------------------------------------

void mainbase_exit()
{
 if(processbase.watcher_pid!=0)
    kill(processbase.watcher_pid,SIGINT);
 if(processbase.sender_pid!=0)
    kill(processbase.sender_pid,SIGINT);
 if(main_el!=NULL) 
   {
    aeDeleteFileEvent(main_el,http_server_fd,AE_READABLE);
    if(unix_socket_server_fd>=0)
       aeDeleteFileEvent(main_el,unix_socket_server_fd,AE_READABLE);
    if(process_monitor_timer_id>0)
       aeDeleteTimeEvent(main_el,process_monitor_timer_id);
    if(mainbase_loop_timer_id>0)
       aeDeleteTimeEvent(main_el,mainbase_loop_timer_id);
    process_monitor_timer_id=0;
    mainbase_loop_timer_id=0;
    aeDeleteEventLoop(main_el);
   }
 main_el=NULL;
 kobe_printf("%s\tmainbase exit\n",GetCurrentTime().c_str());
}
//-----------------------------------------------
void watcher_exit()
{
 if(main_el!=NULL)
   {
    if(unix_socket_server_fd>=0)
       aeDeleteFileEvent(main_el,unix_socket_server_fd,AE_READABLE);
    aeDeleteEventLoop(main_el);
   }                         
 main_el=NULL;
 if(watcher!=NULL)
    delete watcher;
 watcher=NULL;
 kobe_printf("%s\twatcher exit\n",GetCurrentTime().c_str());
}
//-----------------------------------------------
void sender_exit()
{
 if(main_el!=NULL)
   {
    if(sender_loop_timer_id>0)
       aeDeleteTimeEvent(main_el,sender_loop_timer_id);
    sender_loop_timer_id=0;
    if(unix_socket_server_fd>=0)
       aeDeleteFileEvent(main_el,unix_socket_server_fd,AE_READABLE);
    aeDeleteEventLoop(main_el);
   }
 main_el=NULL;
 if(sender!=NULL)
    delete sender;
 sender=NULL;
 kobe_printf("%s\tsender exit\n",GetCurrentTime().c_str());
}
//-------------------------------------------------------------------------------------------------
void mainbase_cleanup(int)
{exit(1);}
void watcher_cleanup(int)
{exit(1);}
void sender_cleanup(int)
{exit(1);}
//-------------------------------------------------------------------------------------------------

static void SetState(int state)
{
 my_state=state;
 kobe_printf("%s\tstate=>%s\n",GetCurrentTime().c_str(),StateToString(state).c_str());
}
//-------------------------------------------------------------------------------------------------

static void http_server_drive_machine(aeEventLoop* el,int fd,void* arg,int mask)
{
 if(arg==NULL)
    return;
 SocketReceiver* sr=(SocketReceiver*)arg;
 static char buffer[SOCKET_READ_BUFFER_LENGTH];
 while(true)
      {      
       bzero(buffer,SOCKET_READ_BUFFER_LENGTH);
       int rl=read(sr->client_sockfd,buffer,SOCKET_READ_BUFFER_LENGTH);
       if(rl==-1)
         {
          if( (errno==EAGAIN||errno==EWOULDBLOCK) )
             return;
          else
            {
             kobe_printf("%s\tclosed client %s:%d [read from client error %d]\n",
                         GetCurrentTime().c_str(),sr->client_addr.host.c_str(),sr->client_addr.port,errno);
             delete sr;
             return;
            }
         }
       else if(rl==0)
         {
          kobe_printf("%s\tclosed client %s:%d\n",GetCurrentTime().c_str(),
                      sr->client_addr.host.c_str(),sr->client_addr.port);
          delete sr;
          return;
         }
       HTTPProtocol* hp=(HTTPProtocol*)(sr->GetProtocol());
       if(hp==NULL)
         {
          kobe_printf("%s\tclosed client(wrong protocol) %s:%d\n",GetCurrentTime().c_str(),
                      sr->client_addr.host.c_str(),sr->client_addr.port);
          delete sr;
          return;
         }
       vector<HTTPCommand> hcs;
       bool parsed=hp->Eat(buffer,rl,hcs);
       if(parsed==false||hcs.size()!=1)
          continue;
       #ifdef MYDEBUG
       hcs[0].Display();
       #endif
       string result;
       if(hcs[0].Do(result,HTTPCommandFunction)==false)
          continue;
       if(NonblockSend(sr->client_sockfd,result.c_str(),result.size())==false)
         {
          kobe_printf("%s\tclosed client %s:%d [nonblock send to client error]\n",
                      GetCurrentTime().c_str(),sr->client_addr.host.c_str(),sr->client_addr.port);
          delete sr;
          return;
         }
       else
         {
          #ifdef MYDEBUG
          kobe_printf("%s\thttp out=>%s\n",GetCurrentTime().c_str(),result.c_str());
          #endif
         }
       break;
      }//end while true
 delete sr;
 return;
}
//-------------------------------------------------------------------------------------------------

void http_server_accept_handler(aeEventLoop* el,int fd,void* arg,int mask)
{
 if(arg!=NULL)//must come from server_fd
    return;
 struct sockaddr_in client_address;
 socklen_t length=sizeof(client_address);
 int client_fd=accept(http_server_fd,(struct sockaddr*)&client_address,&length);
 AddrPair ap;
 ap.host=inet_ntoa(client_address.sin_addr);
 ap.port=ntohs(client_address.sin_port);
 if(client_fd<0)
   {
    if(errno== EAGAIN || errno == EWOULDBLOCK)
       return;
    kobe_printf("%s\tERROR: errno %d comes when call accept!\n",GetCurrentTime().c_str(),errno);
    return;
   }
 if(SetNBSocket(client_fd)==false)
   {
    kobe_printf("%s\tERROR: failed to set noblock client socket\n",GetCurrentTime().c_str());
    close(client_fd);
    return;
   }
 if(SetCESocket(client_fd)==false)
   {
    kobe_printf("%s\tERROR: failed to set cloexec client socket\n",GetCurrentTime().c_str());
    close(client_fd);
    return;
   }
 SocketReceiver* sr=new SocketReceiver(client_fd,HTTP_PROTOCOL);
 sr->client_addr=ap;

 int rv=aeCreateFileEvent(main_el,client_fd,AE_READABLE,http_server_drive_machine,sr);
 if(rv<0)
   {
    kobe_printf("%s\tERROR: failed to accept client %s:%d [%d]\n",
                GetCurrentTime().c_str(),ap.host.c_str(),ap.port,rv);
    return;
   }
 kobe_printf("%s\taccepted client %s:%d\n",GetCurrentTime().c_str(),ap.host.c_str(),ap.port);
 return;
}
//-------------------------------------------------------------------------------------------------

void HTTPServerRun()
{
 struct sockaddr_in server_address;
 if((http_server_fd=socket(AF_INET,SOCK_STREAM,0))<0)
   {
    kobe_printf("%s\tERROR: create socket error %d\n",GetCurrentTime().c_str(),errno);
    exit(1);
   }
 int reuseaddr=1;
 setsockopt(http_server_fd,SOL_SOCKET,SO_REUSEADDR,(const void*)&reuseaddr,sizeof(int));
 bzero(&server_address,sizeof(server_address));
 server_address.sin_family=AF_INET;
 server_address.sin_port=htons(conf.http_server_port);
 server_address.sin_addr.s_addr=htons(INADDR_ANY);
 if(bind(http_server_fd,(struct sockaddr*)&server_address,sizeof(server_address))<0)
   {
    kobe_printf("%s\tERROR: failed to bind port %d,%d\n",GetCurrentTime().c_str(),
                conf.http_server_port,errno);
    exit(1);
   }
 if(listen(http_server_fd,LISTEN_QUEUE_LENGTH)<0)
   {
    kobe_printf("%s\tERROR: failed to call listen %d\n",GetCurrentTime().c_str(),errno);
    exit(1);
   }
 if(SetNBSocket(http_server_fd)==false)
   {
    kobe_printf("%s\tERROR: failed to set noblock socket %d\n",GetCurrentTime().c_str(),errno);
    exit(1);
   }
 kobe_printf("%s\tlistening on port:%d ......\n",GetCurrentTime().c_str(),conf.http_server_port);
 aeCreateFileEvent(main_el,http_server_fd,AE_READABLE,http_server_accept_handler,NULL);
}
//-------------------------------------------------------------------------------------------------

static void unix_socket_server_drive_machine(aeEventLoop* el,int fd,void* arg,int mask)
{      
 if(arg==NULL)
    return;
 SocketReceiver* sr=(SocketReceiver*)arg;
 static char buffer[SOCKET_READ_BUFFER_LENGTH];
 while(true)
      {      
       bzero(buffer,SOCKET_READ_BUFFER_LENGTH);
       int rl=read(sr->client_sockfd,buffer,SOCKET_READ_BUFFER_LENGTH);
       if(rl==-1)
         {
          if( (errno==EAGAIN||errno==EWOULDBLOCK) )
             return;
          else
            {
             kobe_printf("%s\tclosed client %s [read from client error %d]\n",
                         GetCurrentTime().c_str(),sr->client_filename.c_str(),errno);
             delete sr;
             return;
            }
         }
       else if(rl==0)
         {
          kobe_printf("%s\tclosed client %s\n",GetCurrentTime().c_str(),
                      sr->client_filename.c_str());
          delete sr;
          return;
         }
       UnixSocketProtocol* usp=(UnixSocketProtocol*)(sr->GetProtocol());
       if(usp==NULL)
         {
          kobe_printf("%s\tclosed client(wrong protocol) %s\n",GetCurrentTime().c_str(),
                      sr->client_filename.c_str());
          delete sr;
          return;
         }
       vector<UnixSocketCommand> uscs;
       bool parsed=usp->Eat(buffer,rl,uscs);

       if(parsed==false||uscs.empty())
          continue;
       for(unsigned int i=0;i<uscs.size();i++)
          {
           #ifdef MYDEBUG
           uscs[i].Display();
           #endif
           string result;
           if(uscs[i].Do(result,UnixSocketCommandFunction)==false)
              continue;
           if(NonblockSend(sr->client_sockfd,result.c_str(),result.size())==false)
             {
              kobe_printf("%s\tclosed client %s [nonblock send to client error]\n",
                          GetCurrentTime().c_str(),sr->client_filename.c_str());
              delete sr;
              return;
             }
           else
             {
              #ifdef MYDEBUG
              kobe_printf("%s\tunix socket out=>%s\n",GetCurrentTime().c_str(),result.c_str());
              #endif
             }
          }//end for i
       break;
      }//end while
 delete sr;
 return;
}
//-------------------------------------------------------------------------------------------------

void unix_socket_server_accept_handler(aeEventLoop* el,int fd,void* arg,int mask)
{
 struct sockaddr_un client_address;
 socklen_t length=sizeof(client_address);
 int client_fd=accept(unix_socket_server_fd,(struct sockaddr*)&client_address,&length);
 if(client_fd<0)
    return;
 SetNBSocket(client_fd);
 SetCESocket(client_fd);

 length-=offsetof(struct sockaddr_un,sun_path);
 client_address.sun_path[length]=0;
 
 SocketReceiver* sr=new SocketReceiver(client_fd,UNIX_SOCKET_PROTOCOL);
 sr->client_filename=client_address.sun_path;
 
 int rv=aeCreateFileEvent(main_el,client_fd,AE_READABLE,unix_socket_server_drive_machine,sr);
 if(rv<0)
   {
    kobe_printf("%s\tERROR: failed to accept client %s [%d]\n",
                GetCurrentTime().c_str(),sr->client_filename.c_str(),rv);
    return;
   }
 kobe_printf("%s\tunix socket accept %s\n",GetCurrentTime().c_str(),sr->client_filename.c_str());
}
//-------------------------------------------------------------------------------------------------

void UnixSocketServerRun()
{
 string filename;
 switch(my_role)
       {
        case ROLE_MAINBASE:
             filename=conf.mainbase_unix_socket_path+MAINBASE_UNIX_SOCKET_FILENAME;
             break;
        case ROLE_WATCHER:
             filename=conf.watcher_unix_socket_path+WATCHER_UNIX_SOCKET_FILENAME;
             break;
        case ROLE_SENDER:
             filename=conf.sender_unix_socket_path+SENDER_UNIX_SOCKET_FILENAME;
             break;
        default:
             break;
       }//end switch
 unix_socket_server_fd=CreateServerUnixSocket(filename);
 if(unix_socket_server_fd<0)
   {
    kobe_printf("%s\tERROR: failed to create unix socket %s,%d\n",
                GetCurrentTime().c_str(),filename.c_str(),errno);
    exit(-1);
   }
  kobe_printf("%s\tlistening on unix_socket:%s ......\n",GetCurrentTime().c_str(),filename.c_str());
  aeCreateFileEvent(main_el,unix_socket_server_fd,AE_READABLE,unix_socket_server_accept_handler,NULL);
}
//-------------------------------------------------------------------------------------------------

void WatcherRun()
{
 if(main_el!=NULL)
   {
    aeDeleteEventLoop(main_el);
    main_el=NULL;
   }
 //----------------------------------------------
 atexit(watcher_exit);
 signal(SIGINT,watcher_cleanup);
 //watcher run
 string errmsg;
 if(conf.LoadFromFile(my_argv[2],errmsg)==false)
   {
    kobe_printf("%s\tERROR: failed to load conf,%s\n",
                GetCurrentTime().c_str(),errmsg.c_str());
    exit(-3);
   }
 //----------------------------------------------
 main_el=aeCreateEventLoop();
 if(main_el==NULL)
   {
    kobe_printf("%s\tERROR: failed to create event loop\n",GetCurrentTime().c_str());
    exit(-1);
   }
 //----------------------------------------------
 watcher=new Watcher(conf,main_el);
 //----------------------------------------------
 UnixSocketServerRun();
 //----------------------------------------------
 aeMain(main_el);
 kobe_printf("%s\twatcher exit\n",GetCurrentTime().c_str());
}
//-------------------------------------------------------------------------------------------------

void StartWatcher(int argc,char* argv[])
{
 int rv=0;
 if((rv=fork())==0)//watcher
   {
    bzero(argv[1],strlen(argv[1]));
    sprintf(argv[1],"watcher");
    my_role=ROLE_WATCHER;
    WatcherRun();
    kobe_printf("%s\twatcher exit\n",GetCurrentTime().c_str());
    exit(1);
   }
 else if(rv==-1)
   {
    kobe_printf("%s\tERROR: failed to create watcher\n",GetCurrentTime().c_str());
    exit(-2);
   }
 else
   {
    kobe_printf("%s\tstart watcher process ok\n",GetCurrentTime().c_str());
    processbase.watcher_pid=rv;
   }
}
//-------------------------------------------------------------------------------------------------

void SenderRun()
{
 if(main_el!=NULL)
   {
    aeDeleteEventLoop(main_el);
    main_el=NULL;
   }
 //----------------------------------------------
 atexit(sender_exit);
 //watcher run
 string errmsg;
 if(conf.LoadFromFile(my_argv[2],errmsg)==false)
   { 
    kobe_printf("%s\tERROR: failed to load conf,%s\n",GetCurrentTime().c_str(),
                errmsg.c_str());
    exit(-3);
   }
 //----------------------------------------------
 sender=new Sender(conf);
 main_el=aeCreateEventLoop();
 if(main_el==NULL)
   {
    kobe_printf("%s\tERROR: failed to create event loop\n",
                GetCurrentTime().c_str());
    exit(-1);
   }
 //----------------------------------------------
 UnixSocketServerRun();
 //----------------------------------------------
 signal(SIGINT,sender_cleanup);
 aeMain(main_el);
}
//-------------------------------------------------------------------------------------------------

void StartSender(int argc,char* argv[])
{
 int rv=0;
 if((rv=fork())==0)//sender
   {
    bzero(argv[1],strlen(argv[1]));
    sprintf(argv[1],"sender");
    my_role=ROLE_SENDER;
    signal(SIGCHLD,SIG_DFL);
    SenderRun();
    exit(1);
   }
 else if(rv==-1)
   {
    kobe_printf("%s\tERROR: failed to create sender\n",GetCurrentTime().c_str());
    exit(-2);
   }
 else
   {
    kobe_printf("%s\tstart sender process ok\n",GetCurrentTime().c_str());
    processbase.sender_pid=rv;
   }
}
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------

int process_monitor_timer_handler(struct aeEventLoop* eventLoop, long long id, void* clientData)
{
 if(my_role!=ROLE_MAINBASE)
    return AE_NOMORE;
 if(processbase.sender_pid!=0)
   {   
    if(kill(processbase.sender_pid,0)!=0&&errno==ESRCH)
       StartSender(my_argc,my_argv);
   }
 if(processbase.watcher_pid!=0)
   {
    if(kill(processbase.watcher_pid,0)!=0&&errno==ESRCH)
       StartWatcher(my_argc,my_argv);
   }
 return PROCESSES_MONITOR_TIME_INTERVAL_MS;
}
//-------------------------------------------------------------------------------------------------

void ProcessMonitorRun()
{
 process_monitor_timer_id=aeCreateTimeEvent(main_el,PROCESSES_MONITOR_TIME_INTERVAL_MS,
                                         process_monitor_timer_handler,NULL,NULL);
}
//-------------------------------------------------------------------------------------------------

void mytest(int argc,char* argv[])//for kobetest
{
 string errmsg;
 if(conf.LoadFromFile(argv[2],errmsg)==false)
    printf("ERROR: failed to load conf,%s\n",errmsg.c_str());
 conf.Display();
 /*
 Watcher watcher(conf);
 watcher.Init();
 watcher.Prepare(err);
 */
 /* 
 Rlog rlog(conf);
 int i=0;
 while(i++<50000)
      {
       string filename="haha."+IntToStr(i)+".txt";
       rlog.Write(IN_CREATE,filename.c_str(),err);
      }
 */
 /*
 int err=0;
 Rlog rlog(conf);
 SenderStat ss(conf);
 ss.Reset();
 ss.Load();
 int count=0;
 Sender sender(conf);
 if(sender.SendAll(errmsg))
    cout<<"send all ok"<<endl;
 else
    cout<<errmsg<<endl;
 while(true)
      {
       vector<RlogItem> items;
       if(rlog.Read(ss,items,err)==false)
         {
          if(err==-1)
            {
             printf("no file to read\n");
             sleep(1);
             continue;
            }
          else
             break;
         }
       count+=items.size();
       printf("read %zu items,total %d,offset %lu\n",items.size(),count,rlog.current_file_readed_offset);
       if(items.empty())
          continue;
       if(sender.Send(items,errmsg))
          ss.Save(rlog.current_filename,rlog.current_file_readed_offset);
       else
          cout<<errmsg<<endl;
      }//end while
 */
 //----------------------------------------------
 /*string content;
 FileGetContent("dirfilter.txt",content);
 vector<string> items;
 SplitString(content,items,"\n");
 CoveredFileFilter(items);
 for(unsigned int i=0;i<items.size();i++)
     cout<<items[i]<<endl;*/
 vector<string> items;
 set<string> iset;
 iset.insert("/home/conglei/saegit/dmirror/test/1/2/c/d.txt");
 AddExcludeFiles("/home/conglei/saegit/dmirror/",
                 "/home/conglei/saegit/dmirror/main.cpp",iset,items);
 RecheckExcludeFiles("/home/conglei/saegit/dmirror/ae.cpp",items);
 for(unsigned int i=0;i<items.size();i++)
     cout<<items[i]<<endl;
 //----------------------------------------------
 exit(1);
}
//-------------------------------------------------------------------------------------------------


bool Notify(int role,const string& command,string& result)
{
 result="";
 string filename;
 switch(role)
       {
        case ROLE_WATCHER:
             filename=conf.watcher_unix_socket_path+WATCHER_UNIX_SOCKET_FILENAME;
             break;
        case ROLE_SENDER:
             filename=conf.sender_unix_socket_path+SENDER_UNIX_SOCKET_FILENAME;
             break;
        case ROLE_MAINBASE:
             filename=conf.mainbase_unix_socket_path+MAINBASE_UNIX_SOCKET_FILENAME;
             break;
        default:
             return false;
       }
 int fd=0;
 string cs=command+NC_END_TOKEN;
 if((fd=CreateAndConnectUnixSocket(filename))!=-1&&
    UnixSocketBlockSend(fd,cs.c_str(),cs.size()))
   {
    char buffer[1024];
    while(true)
         {
          int rv=read(fd,buffer,1024);
          if(rv<=0)
             break;
          buffer[rv]=0;
          result+=buffer;
         }
    close(fd);//need read
    Trim(result);
    return true;
   }
 if(fd!=0)
    close(fd);
 return false;
}
//-------------------------------------------------------------------------------------------------

bool HTTPCommandFunction(const string& command,string& res)
{
 res="ok";
 if(command==HTTP_COMMAND_SET_MASTER)
   {
    MyCurl mycurl;
    mycurl.SetConnectionTimeout(1);
    mycurl.SetTimeout(1);
    int response_code;
    string result;
    string errmsg;
    if(my_state==STATE_NULL)
      {
       string url="http://"+conf.remote_pair.ToStr()+"/"+HTTP_COMMAND_SET_SLAVE;
       if(mycurl.GetURL(url,response_code,result,errmsg)==false||
                       response_code!=200)
         {
          res="error: failed to set remote to slave "+IntToStr(response_code);
          return true;
         }
       Trim(result);
       if(result!="ok")
         {
          res="error: "+result;
          return true;
         }
       SetState(STATE_TO_MASTER_WAIT_SLAVE_OK);
      }
    else if(my_state==STATE_SLAVE)
       SetState(STATE_TO_MASTER_WAIT_SLAVE_OK);
    else
       res="error: cannot set master when state is "+my_state;
   }//end HTTP_COMMAND_SET_MASTER
 //----------------------------------------------
 else if(command==HTTP_COMMAND_SET_SLAVE)
   {
    if(my_state==STATE_NULL||my_state==STATE_SLAVE)
       SetState(STATE_SLAVE);
    else if(my_state==STATE_MASTER)
       SetState(STATE_MASTER_TO_SLAVE1);
    else
       res="error: cannot set slave when state is "+my_state;
   }//end HTTP_COMMAND_SET_SLAVE
 //----------------------------------------------
 else if(command==HTTP_COMMAND_GET_STATE)
    res=IntToStr(my_state);
 else if(command==HTTP_COMMAND_GET_STATUS)
   {
    WatcherStatus ws;
    SenderStatus ss;
    string rs;
    if(Notify(ROLE_WATCHER,UNIX_SOCKET_COMMAND_GET_WATCHER_STATUS,rs)&&
       strncmp(rs.c_str(),"ok ",3)==0)
      {
       rs=rs.substr(3);
       ws.FromString(rs);
      }
    if(Notify(ROLE_SENDER,UNIX_SOCKET_COMMAND_GET_SENDER_STATUS,rs)&&
       strncmp(rs.c_str(),"ok ",3)==0)
      {
       rs=rs.substr(3);
       ss.FromString(rs);
      }
    res=BuildHTMLResult(conf,my_state,ws,ss);
   }
 //----------------------------------------------
 else if(command==HTTP_COMMAND_SHOW_CONF)
    res=conf.ToString();
 else if(command==HTTP_COMMAND_UPDATE_CONF)
   {
    string errmsg;
    string rs;
    if(conf.LoadFromFile(my_argv[2],errmsg)==false)
       res="error: "+errmsg;
    else if(Notify(ROLE_WATCHER,UNIX_SOCKET_COMMAND_UPDATE_CONF,rs)==false||
            Notify(ROLE_SENDER,UNIX_SOCKET_COMMAND_UPDATE_CONF,rs)==false)
       res="error: failed update conf to watcher/sender";
   }
 return true;
}
//-------------------------------------------------------------------------------------------------

int sender_loop_timer_handler(struct aeEventLoop* eventLoop, long long id, void* clientData)
{
 if(sender->senderstat==NULL)
   {
    sender->senderstat=new SenderStat(conf);
    sender->senderstat->Load();
   }
 int count=0;
 int err=0;
 string errmsg;
 while(count++<16)
      {
       vector<RlogItem> items;
       if(sender->rlog->Read(*(sender->senderstat),items,err)==false)
         {
          if(err==-1)
            {
             kobe_printf("%s\tno more to read %s:%lu\n",GetCurrentTime().c_str(),
                    sender->senderstat->current_filename.c_str(),
                    sender->senderstat->offset);
             return SENDER_LOOP_TIME_INTERVAL_MS;
            }
          else
             break;
         }
       if(items.empty())
          continue;
       if(sender->Send(items,errmsg))
         {
          sender->senderstat->Save(sender->rlog->current_filename,sender->rlog->current_file_readed_offset);
          //sleep(1);//kobetest
         }
       else
         {
          kobe_printf("%s\tERROR: failed to send %s\n",
                      GetCurrentTime().c_str(),errmsg.c_str());
          return SENDER_LOOP_TIME_INTERVAL_MS;
         }
      }//end while
 if(my_role==ROLE_SENDER)
    return 1;
 return AE_NOMORE;
}
//-------------------------------------------------------------------------------------------------

void SenderLoopRun()
{
 sender->SetUpdateMode(false);
 sender_loop_timer_id=aeCreateTimeEvent(main_el,SENDER_LOOP_TIME_INTERVAL_MS,
                                        sender_loop_timer_handler,NULL,NULL);
}
//-------------------------------------------------------------------------------------------------

void SenderLoopStopRun()
{
 if(sender_loop_timer_id!=0)
    aeDeleteTimeEvent(main_el,sender_loop_timer_id);
 sender_loop_timer_id=0;
}
//-------------------------------------------------------------------------------------------------

bool UnixSocketCommandFunction(const string& command,string& res)
{
 if(command==UNIX_SOCKET_COMMAND_START_WATCHER)//when watcher received start
   {
    int err;
    if(watcher->Prepare(err)&&watcher->Start(main_el))
      {
       res="ok";
       kobe_printf("%s\twatcher started\n",GetCurrentTime().c_str());
       return true;
      }
    res="error: "+IntToStr(err);
    return false;
   }
 else if(command==UNIX_SOCKET_COMMAND_STOP_WATCHER)//when watcher received stop
   {
    watcher->Stop(main_el);
    res="ok";
    kobe_printf("%s\twatcher stoped\n",GetCurrentTime().c_str());
    return true;
   }
 else if(command==UNIX_SOCKET_COMMAND_START_SENDER1)//when sender received start1
   {
    if(sender->senderstat==NULL)
      {
       sender->senderstat=new SenderStat(conf);
       sender->senderstat->Reset();
       sender->senderstat->Load();
      }
    string errmsg;
    struct timeval tv;
    tv=BeginTiming();
    kobe_printf("%s\tbegin send all...\n",GetCurrentTime().c_str());
    if(sender->SendAll(errmsg))
      {
       kobe_printf("%s\tsend all finished %.2f sec, sender loop started\n",
                   GetCurrentTime().c_str(),EndTiming(tv));
       SenderLoopRun();
       res="ok";
      }
    else
       res="error: send all "+errmsg;
    return true;
   }
 else if(command==UNIX_SOCKET_COMMAND_SET_SENDER_UPDATE_MODE_TRUE)//when sender received update mode true
   {
    sender->SetUpdateMode(true);
    res="ok";
    return true;
   }
 else if (command==UNIX_SOCKET_COMMAND_SET_SENDER_UPDATE_MODE_FALSE)//when sender received update mode false
   {
    sender->SetUpdateMode(false);
    res="ok";
    return true;
   }
 else if(command==UNIX_SOCKET_COMMAND_STOP_SENDER)
   {
    SenderLoopStopRun();
    res="ok";
    return true;
   }
 else if(command==UNIX_SOCKET_COMMAND_GET_SENDER_LAST_READ_TIME)
   {
    res="ok "+IntToStr(sender->rlog->last_read_time);
    return true;
   }
 else if(command==UNIX_SOCKET_COMMAND_GET_WATCHER_STATUS&&my_role==ROLE_WATCHER)
   {
    res="ok ";
    watcher->watcherstatus.Fresh();
    res+=watcher->watcherstatus.ToString();
    return true; 
   }
 else if(command==UNIX_SOCKET_COMMAND_GET_SENDER_STATUS&&my_role==ROLE_SENDER)
   {
    res="ok ";
    sender->senderstatus.Fresh();
    res+=sender->senderstatus.ToString();
    return true;
   }
 else if(command==UNIX_SOCKET_COMMAND_UPDATE_CONF)
   {
    string errmsg;
    if(conf.LoadFromFile(my_argv[2],errmsg)==false)
       res="error "+errmsg;
    else
       res="ok";
    return true;
   }
 return true;
}
//-------------------------------------------------------------------------------------------------

int mainbase_loop_timer_handler(struct aeEventLoop* eventLoop, long long id, void* clientData)
{
 if(my_role!=ROLE_MAINBASE)
    return AE_NOMORE;
 if(my_state==STATE_TO_MASTER_WAIT_SLAVE_OK)
   {
    string url="http://"+conf.remote_pair.ToStr()+"/"+HTTP_COMMAND_GET_STATE;
    MyCurl mycurl;
    mycurl.SetConnectionTimeout(1);
    mycurl.SetTimeout(1);
    int response_code;
    string result;
    string errmsg;
    if(mycurl.GetURL(url,response_code,result,errmsg)==true&&
                     response_code==200)
      {
       Trim(result);
       if(result==IntToStr(STATE_SLAVE))
         {
          string res;
          if(Notify(ROLE_WATCHER,UNIX_SOCKET_COMMAND_START_WATCHER,res)==true
             &&
             Notify(ROLE_SENDER,UNIX_SOCKET_COMMAND_START_SENDER1,res)==true)
             SetState(STATE_MASTER);
         }
      }
    else
       kobe_printf("%s\twait for slave ok %s\n",GetCurrentTime().c_str(),
                   result.c_str());
   }//end if my_state==STATE_TO_MASTER_WAIT_SLAVE_OK
 else if(my_state==STATE_MASTER_TO_SLAVE1)
   {
    string res;
    if(Notify(ROLE_WATCHER,UNIX_SOCKET_COMMAND_STOP_WATCHER,res)==true&&
       Notify(ROLE_SENDER,UNIX_SOCKET_COMMAND_SET_SENDER_UPDATE_MODE_TRUE,res)==true)
       SetState(STATE_MASTER_TO_SLAVE2);
   }//end if my_state==STATE_MASTER_TO_SLAVE;
 else if(my_state==STATE_MASTER_TO_SLAVE2)
   {
    string res;
    if(Notify(ROLE_SENDER,UNIX_SOCKET_COMMAND_GET_SENDER_LAST_READ_TIME,res)==true)
      {
       vector<string> vec;
       SplitString(res,vec," ");
       if(vec.size()==2&&vec[0]=="ok")
         {
          time_t last_read_time=atoi(vec[1].c_str());
          if(time(NULL)-last_read_time>MASTER_WAIT_RLOG_NO_CHANGE_TO_SLAVE_TIME&&
            Notify(ROLE_SENDER,UNIX_SOCKET_COMMAND_STOP_SENDER,res))
             SetState(STATE_SLAVE);
         }
      }
   }//end if my_state==STATE_MASTER_TO_SLAVE2
 //----------------------------------------------
 return MAINBASE_LOOP_TIME_INTERVAL_MS;
}
//-------------------------------------------------------------------------------------------------

void MainbaseLoopRun()
{
 mainbase_loop_timer_id=aeCreateTimeEvent(main_el,MAINBASE_LOOP_TIME_INTERVAL_MS,
                                          mainbase_loop_timer_handler,NULL,NULL);
}
//-------------------------------------------------------------------------------------------------

int main(int argc,char* argv[])
{
 //mytest(argc,argv);
 srandom(time(NULL));
 processbase.mainbase_pid=getpid();
 if(argc!=3||string(argv[1])!="mainbase")
   {
    DisplayUsage();
    return -1;
   }
 my_argc=argc;
 my_argv=argv;
 signal(SIGCHLD,SIG_IGN); 
 //----------------------------------------------
 StartWatcher(argc,argv);//fork watcher
 StartSender(argc,argv);//fork sender
 my_role=ROLE_MAINBASE;
 //----------------------------------------------
 atexit(mainbase_exit);
 //mainbase run
 string errmsg;
 if(conf.LoadFromFile(argv[2],errmsg)==false)
   {
    kobe_printf("%s\tERROR: failed to load conf,%s\n",GetCurrentTime().c_str(),
                errmsg.c_str());
    return -3;
   }
 conf.Display();
 //----------------------------------------------
 main_el=aeCreateEventLoop();
 if(main_el==NULL)
   {
    kobe_printf("%s\tERROR: failed to create event loop\n",
                GetCurrentTime().c_str());
    return -1;
   }
 //----------------------------------------------
 HTTPServerRun();
 UnixSocketServerRun();
 ProcessMonitorRun();
 MainbaseLoopRun();
 //----------------------------------------------
 signal(SIGINT,mainbase_cleanup);
 aeMain(main_el);
 //----------------------------------------------
 return 0;
}
//-------------------------------------------------------------------------------------------------
