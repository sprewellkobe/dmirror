#include"Sender.hpp"
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------

bool Sender::MakeRsyncFileList(const vector<RlogItem>& items,
                               string& include_content,string& exclude_content,
                               int& include_item_number,int& exclude_item_number)
{
 string rsync_list_file_name=conf.sender_tmp_path+RSYNC_LIST_FILE_NAME;
 string rsync_exclude_file_name=conf.sender_tmp_path+RSYNC_EXCLUDE_FILE_NAME;
 string rp,frp;
 include_item_number=0;
 exclude_item_number=0;
 include_content="";
 vector<string> include_items;
 vector<string> exclude_items;
 set<string> iset;
 for(unsigned int i=0;i<items.size();i++)
    {
     if(items[i].filename.size()<=conf.local_dir.size())
       {
        kobe_printf("%s\tERROR: filename.size<=local_dir.size,%s\n",
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
                    kobe_printf("skip not exists %s\n",items[i].filename.c_str());
                    #endif
                    break;
                   }
                 rp=items[i].filename.substr(conf.local_dir.size());
                 if(rp.empty())
                    rp="./";
                 if(include_items.empty()==false&&rp==include_items.back())
                    break;
                 RecheckExcludeFiles(items[i].filename,exclude_items);
                 /*#ifdef MYDEBUG
                 if(items[i].event_type==IN_MOVED_TO)
                   {
                    cout<<"item2: "<<items[i].filename<<endl;
                    for(unsigned int i=0;i<exclude_items.size();i++)
                        cout<<"\t"<<exclude_items[i]<<endl;
                   }
                 #endif*/
                 include_items.push_back(rp);
                 iset.insert(items[i].filename);
                 break;
            case IN_DELETE:
            case IN_DELETE_SELF:
            case IN_MOVED_FROM:
                 rp=items[i].filename;
                 if(rp.size()<conf.local_dir.size()||
                    (rp.size()==conf.local_dir.size()&&rp!=conf.local_dir)
                   )
                   {
                    #ifdef MYDEBUG
                    kobe_printf("skip not invalid %s\n",rp.c_str());
                    #endif        
                    break; 
                   }
                 if(rp.size()>conf.local_dir.size())
                   {
                    rp=GetParentPath(rp);
                    if(FileExists(rp)==false)                    
                      {                                          
                       #ifdef MYDEBUG                            
                       kobe_printf("skip not exists %s\n",rp.c_str());
                       #endif                                    
                       break;                                    
                      }
                    frp=rp;
                    rp=rp.substr(conf.local_dir.size());
                    if(rp.empty())   
                       rp="./"; 
                   }
                 else
                   {
                    frp=rp;
                    rp="./";
                   }
                 if(include_items.empty()==false&&rp==include_items.back())
                    break;
                 RecheckExcludeFiles(frp,exclude_items);
                 AddExcludeFiles(frp,items[i].filename,
                                 iset,exclude_items);
                 include_items.push_back(rp);
                 iset.insert(items[i].filename);
                 break;
            default:
                 break;
           }//end switch
    }//end for i
 if(include_items.empty())
    return true;
 CoveredIncludeFileFilter(include_items);
 for(unsigned int i=0;i<include_items.size();i++)
     include_content+=include_items[i]+"\n";
 //sort(exclude_items.begin(),exclude_items.end());
 unique(exclude_items.begin(),exclude_items.end());
 for(unsigned int i=0;i<exclude_items.size();i++)
    {
     exclude_items[i]=exclude_items[i].substr(conf.local_dir.size());
     exclude_content+=exclude_items[i]+"\n";
    }
 return FilePutContent(rsync_exclude_file_name,exclude_content)==true&&
        FilePutContent(rsync_list_file_name,include_content)==true;
}
//-------------------------------------------------------------------------------------------------

bool Sender::Send(const vector<RlogItem>& items,string& errmsg)
{
 string include_content,exclude_content;
 int include_item_number=0;
 int exclude_item_number=0;
 if(MakeRsyncFileList(items,include_content,exclude_content,
                      include_item_number,exclude_item_number)==false)
   {
    errmsg="failed to write rsync list file";
    return false;
   }
 if(include_content.empty())
   {
    #ifdef MYDEBUG
    kobe_printf("%s\tskip empty rsync\n",GetCurrentTime().c_str());
    #endif
    return true;
   }
 string rsync_error_filename=conf.sender_tmp_path+RSYNC_ERROR_FILE_NAME;
 string command="rsync "+RSYNC_PARAMS;
 command+=" --files-from="+conf.sender_tmp_path+RSYNC_LIST_FILE_NAME;
 command+=" --exclude-from="+conf.sender_tmp_path+RSYNC_EXCLUDE_FILE_NAME;
 if(update_mode)
    command+=" --update";
 if(conf.rsync_bwlimit>0)
    command+=" --bwlimit="+IntToStr(conf.rsync_bwlimit);
 command+=" "+conf.local_dir+" "+conf.remote_dir;
 command+=" 2>"+rsync_error_filename;
 kobe_printf("%s\tshell [%s]\n",GetCurrentTime().c_str(),command.c_str());
 #ifdef MYDEBUG
 kobe_printf("%s\n",include_content.c_str());
 kobe_printf("/---------/\n");
 kobe_printf("%s\n",exclude_content.c_str());
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
 senderstatus.sent_file_number+=include_item_number;
 return true;
}
//-------------------------------------------------------------------------------------------------

bool Sender::SendAll(string& errmsg)
{
 string command="rsync "+RSYNC_PARAMS;
 command+=" "+conf.local_dir+" "+conf.remote_dir;
 kobe_printf("%s\tshell [%s]\n",GetCurrentTime().c_str(),command.c_str());
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
 return true;
}
//-------------------------------------------------------------------------------------------------
