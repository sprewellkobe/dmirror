//coded by kobe, 2008.9.9
#ifndef MYCURLHPP
#define MYCURLHPP
#include <unistd.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <string>
#include <iostream>
using namespace std;
//----------------------------------------------------------------------

class MyCurl
{
public:
 CURL* curl;
 CURLcode curlcode;
 curl_slist* slist;
public:
 MyCurl():curl(NULL),slist(NULL)
 {
  curl=curl_easy_init();
 };
 ~MyCurl()
 {
  if(slist!=NULL)
    {
     curl_slist_free_all(slist);
     slist=NULL;
    }
  if(curl!=NULL)
    curl_easy_cleanup(curl);
  curl=NULL;
 }
 void Clean()
 {
  if(slist!=NULL)
    {
     curl_slist_free_all(slist);
     slist=NULL;
    }
  if(curl!=NULL)
    curl_easy_cleanup(curl);
  curl=NULL;
 }
 void SetMultiThreadMode()
 {
  if(curl==NULL)
     curl=curl_easy_init();
  curl_easy_setopt(curl,CURLOPT_FORBID_REUSE,1); 
  curl_easy_setopt(curl,CURLOPT_NOSIGNAL,1);
 }
public:
 bool GetURL(const string& url,int& response_code,string& result,string& errormsg);
public:
 static int writefunction(void* ptr,size_t size,size_t nmemb,void* stream);
 static int readfunction(void* ptr,size_t size,size_t nmemb,void* stream);
public:
 bool SetUserAgent(const string& str);
 bool SetHttpLogin(const string& username,const string& password);
 bool SetReferer(const string& str);
 bool SetConnectionTimeout(int seconds=10);
 bool SetTimeout(int seconds=80);
 bool SetPostField(const string& str);
 bool SetPostField(const char* buff,int size);
 bool SetNoSignal();
 bool AddHttpRequestHeader(const string& s1,const string& s2);
 bool SetNoDNSCache();
};
//----------------------------------------------------------------------
#endif
