#include"Sender.hpp"
//-------------------------------------------------------------------------------------------------

bool Sender::MakeRsyncFileList(const vector<RlogItem>& items,
                               string& content,int& item_new_number)
{
 string rsync_list_file_name=conf.sender_tmp_path+RSYNC_LIST_FILE_NAME;
 string rp;
 item_new_number=0;
 content="";
 vector<string> newitems;
 for(unsigned int i=0;i<items.size();i++)
    {
     if(items[i].filename.size()<=conf.local_dir.size())
       {
        printf("%s\tERROR: filename.size<=local_dir.size,%s\n",
               GetCurrentTime().c_str(),items[i].filename.c_str());
        continue;
       }
     switch(items[i].event_type)
           {
            case IN_CREATE:
            case IN_CLOSE_WRITE:
            case IN_MODIFY:
            case IN_MOVE_SELF:
            case IN_MOVED_TO:
                 if(FileExists(items[i].filename)==false)
                   {
                    #ifdef MYDEBUG
                    printf("skip not exists %s\n",items[i].filename.c_str());
                    #endif
                    break;
                   }
                 rp=items[i].filename.substr(conf.local_dir.size());
                 if(rp.empty())
                    rp="./";
                 if(newitems.empty()==false&&rp==newitems.back())
                    break;
                 newitems.push_back(rp);
                 break;
            case IN_DELETE:
            case IN_DELETE_SELF:
            case IN_MOVED_FROM:
                 rp=items[i].filename;
                 rp=GetParentPath(rp);
                 if(FileExists(rp)==false)
                   {
                    #ifdef MYDEBUG
                    printf("skip not exists %s\n",rp.c_str());
                    #endif
                    break;
                   }
                 rp=rp.substr(conf.local_dir.size());
                 if(rp.empty())
                    rp="./";
                 if(newitems.empty()==false&&rp==newitems.back())
                    break;
                 newitems.push_back(rp);
                 break;
            default:
                 break;
           }//end switch
    }//end for i
 if(newitems.empty())
    return true;
 CoveredFileFilter(newitems);
 for(unsigned int i=0;i<newitems.size();i++)
     content+=newitems[i]+"\n";
 return FilePutContent(rsync_list_file_name,content);
}
//-------------------------------------------------------------------------------------------------

bool Sender::Send(const vector<RlogItem>& items,string& errmsg)
{
 string content;
 int item_new_number=0;
 if(MakeRsyncFileList(items,content,item_new_number)==false)
   {
    errmsg="failed to write rsync list file";
    return false;
   }
 if(content.empty())
   {
    #ifdef MYDEBUG
    printf("%s\tskip empty rsync\n",GetCurrentTime().c_str());
    #endif
    return true;
   }
 string rsync_error_filename=conf.sender_tmp_path+RSYNC_ERROR_FILE_NAME;
 string command="rsync "+RSYNC_PARAMS;
 command+=" --files-from="+conf.sender_tmp_path+RSYNC_LIST_FILE_NAME;
 if(update_mode)
    command+=" --update";
 if(conf.rsync_bwlimit>0)
    command+=" --bwlimit="+IntToStr(conf.rsync_bwlimit);
 command+=" "+conf.local_dir+" "+conf.remote_dir;
 command+=" 2>"+rsync_error_filename;
 printf("%s\tshell [%s]\n",GetCurrentTime().c_str(),command.c_str());
 #ifdef MYDEBUG
 printf("%s\n",content.c_str());
 #endif
 int status=system(command.c_str());
 if(status==-1)
    return false;
 int exitcode=WEXITSTATUS(status);
 if(exitcode!=0&&exitcode!=24)//maybe error, 24 means vanished file
   {
    bool realerror=false;
    string ec;
    if(FileGetContent(rsync_error_filename,ec)&&ec.empty()==false)
      {
       vector<string> vec;
       SplitString(ec,vec,"\n");
       if(vec.size()>1)
         {
          for(unsigned int i=0;i>vec.size()-1;i++)
             {
              if(vec[i].find("No such file or directory")!=string::npos)
                {
                 realerror=true;
                 break;
                }
             }
         }
      }
    if(realerror)
      {
       errmsg="exitcode is "+IntToStr(exitcode);
       return false;
      }
   }
 //printf("%s\texit code %d\n",GetCurrentTime().c_str(),exitcode);
 senderstatus.sent_file_number+=item_new_number;
 return true;
}
//-------------------------------------------------------------------------------------------------

bool Sender::SendAll(string& errmsg)
{
 string command="rsync "+RSYNC_PARAMS;
 command+=" "+conf.local_dir+" "+conf.remote_dir;
 printf("%s\tshell [%s]\n",GetCurrentTime().c_str(),command.c_str());
 int status=system(command.c_str());
 if(status==-1)
   {
    errmsg="status is -1 "+IntToStr(errno);
    return false;
   }
 int exitcode=WEXITSTATUS(status);
 if(exitcode!=0)
   {
    errmsg="exitcode is "+IntToStr(exitcode);
    return false;
   }
 printf("%s\texit code %d\n",GetCurrentTime().c_str(),exitcode);
 return true;
}
//-------------------------------------------------------------------------------------------------
