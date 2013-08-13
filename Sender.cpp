#include"Sender.hpp"
//-------------------------------------------------------------------------------------------------

bool Sender::MakeRsyncFileList(const vector<RlogItem>& items,
                               string& content,int& item_new_number)
{
 string rsync_list_file_name=conf.sender_tmp_path+RSYNC_LIST_FILE_NAME;
 string prerp;
 string rp;
 item_new_number=0;
 content="";
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
                    printf("skip not exists %s\n",items[i].filename.c_str());
                    break;
                   }
                 rp=items[i].filename.substr(conf.local_dir.size());
                 if(rp==prerp)
                    break;
                 content+=rp+"\n";
                 item_new_number++;
                 prerp=rp;
                 break;
            case IN_DELETE:
            case IN_DELETE_SELF:
            case IN_MOVED_FROM:
                 rp=items[i].filename.substr(conf.local_dir.size());
                 rp=GetParentPath(rp);
                 if(FileExists(rp)==false)
                   {
                    printf("skip not exists %s\n",rp.c_str());
                    break;
                   }
                 if(rp==prerp)
                    break;
                 content+=rp+"\n";
                 item_new_number++;
                 prerp=rp;
                 break;
            default:
                 break;
           }//end switch
    }//end for i
 if(content.empty())
    return true;
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
    printf("%s\tskip empty rsync\n",GetCurrentTime().c_str());
    return true;
   }
 string command="rsync "+RSYNC_PARAMS;
 command+=" --files-from="+conf.sender_tmp_path+RSYNC_LIST_FILE_NAME;
 if(update_mode)
    command+=" --update";
 if(conf.rsync_bwlimit>0)
    command+=" --bwlimit="+IntToStr(conf.rsync_bwlimit);
 command+=" "+conf.local_dir+" "+conf.remote_dir;
 printf("%s\tshell [%s]\n",GetCurrentTime().c_str(),command.c_str());
 #ifdef MYDEBUG
 printf("%s\n",content.c_str());
 #endif
 int status=system(command.c_str());
 if(status==-1)
    return false;
 int exitcode=WEXITSTATUS(status);
 if(exitcode!=0)
   {
    errmsg="exitcode is "+IntToStr(exitcode);
    return false;
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
