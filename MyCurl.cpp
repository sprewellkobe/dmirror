//coded by kobe, 2008.9.9
#include "MyCurl.hpp"
//-----------------------------------------------------------------

bool MyCurl::GetURL(const string& url,long& response_code,string& result,string& errormsg)
{
 response_code=-1;
 bool suc=false;
 if(curl==NULL)
    curl=curl_easy_init();
 if(curl==NULL)
    return suc;
 if(curl)
    curl_easy_setopt(curl, CURLOPT_URL,url.c_str());
 //curlcode=curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
 curlcode=curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunction);
 if(curlcode!=CURLE_OK)
   {
    curl_easy_cleanup(curl);
    result="";
    return false;
   }
 curlcode=curl_easy_setopt(curl, CURLOPT_WRITEDATA, (string*)(&result));
 if(curlcode!=CURLE_OK)
   {
    curl_easy_cleanup(curl);
    result="";
    return false;
   }
 curlcode=curl_easy_perform(curl);
 if(curlcode==CURLE_OK)
    curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&response_code); 
 curl_easy_cleanup(curl);
 if(curlcode==CURLE_OK)
   {
    suc=true;
    errormsg="";
   }
 else
   {
    suc=false;
    result="";
    errormsg=curl_easy_strerror(curlcode);
   }
 curl=NULL;
 return suc;
}
//-----------------------------------------------------------------

bool MyCurl::SetUserAgent(const string& str)
{
 if(curl==NULL)
    curl=curl_easy_init();
 curl_easy_setopt(curl,CURLOPT_USERAGENT,str.c_str());
 return true;
}
//-----------------------------------------------------------------

bool MyCurl::SetHttpLogin(const string& username,const string& password)
{
 if(curl==NULL)
    curl=curl_easy_init();
 string tmp=username+":"+password;
 curl_easy_setopt(curl,CURLOPT_USERPWD,tmp.c_str());
 return true;
}
//-----------------------------------------------------------------

bool MyCurl::AddHttpRequestHeader(const string& s1,const string& s2)
{
 if(curl==NULL)
    curl=curl_easy_init();
 string item=s1+": "+s2;
 slist=curl_slist_append(slist,item.c_str());
 curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
 return true;
}
//-----------------------------------------------------------------

bool MyCurl::SetConnectionTimeout(int seconds)
{
 if(curl==NULL)
    curl=curl_easy_init();
 curl_easy_setopt(curl,CURLOPT_NOSIGNAL,1);
 curl_easy_setopt(curl,CURLOPT_CONNECTTIMEOUT,long(seconds));
 return true;
}
//-----------------------------------------------------------------

bool MyCurl::SetTimeout(int seconds)
{
 if(curl==NULL)
    curl=curl_easy_init();
 curl_easy_setopt(curl,CURLOPT_NOSIGNAL,1);
 curl_easy_setopt(curl,CURLOPT_TIMEOUT,long(seconds));
 return true;
}
//-----------------------------------------------------------------

bool MyCurl::SetPostField(const string& str)
{
 if(curl==NULL)
    curl=curl_easy_init();
 curl_easy_setopt(curl,CURLOPT_POSTFIELDS,str.c_str());
 curl_easy_setopt(curl,CURLOPT_POST,1);
 return true;
}
//-----------------------------------------------------------------

bool MyCurl::SetPostField(const char* buff,int size)
{
 if(curl==NULL)
    curl=curl_easy_init();
 curl_easy_setopt(curl,CURLOPT_POSTFIELDS,buff);
 curl_easy_setopt(curl,CURLOPT_POSTFIELDSIZE,size);
 curl_easy_setopt(curl,CURLOPT_POST,1);
 return true;
}
//-----------------------------------------------------------------

bool MyCurl::SetNoSignal()
{
 if(curl==NULL)
    curl=curl_easy_init();
 curl_easy_setopt(curl,CURLOPT_NOSIGNAL,1);
 return true;
}
//-----------------------------------------------------------------

bool MyCurl::SetNoDNSCache()
{
 if(curl==NULL)
    curl=curl_easy_init();
 curl_easy_setopt(curl,CURLOPT_DNS_CACHE_TIMEOUT,0);
 return true;
}
//-----------------------------------------------------------------

int MyCurl::writefunction(void* ptr,size_t size,size_t nmemb,void* stream)
{
 pthread_testcancel();
 int irsize=size*nmemb;
 string* pt_str_Source=NULL;
 pt_str_Source =(string*)stream;
 if(pt_str_Source!=NULL)
    pt_str_Source->append((char*)ptr,irsize);
 return irsize;
}
//-----------------------------------------------------------------

int MyCurl::readfunction(void* ptr,size_t size,size_t nmemb,void* stream)
{
 pthread_testcancel();
 int irsize=size*nmemb;
 string* pt_str_Source=NULL;
 pt_str_Source =(string*)stream;
 if(pt_str_Source!=NULL)
    pt_str_Source->append((char*)ptr,irsize);
 return irsize; 
}
//-------------------------------------------------------------------------------------------------
