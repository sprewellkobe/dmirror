#ifndef INIPARSER
#define INIPARSER
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include "Common.hpp"
using namespace std;
//-------------------------------------------------------------------------------------------------
class IniParser
{
 private:
  map<string,map<string,string> > items;
 public:
  bool Get(const string& section,const string& key,string& value) const;
  bool Set(const string& section,const string& key,const string& value);
 public:
  bool ReadFromFile(const string& filename);
  bool WriteToFile(const string& filename);
  bool ReadFromString(const string& content);
  bool WriteToString(string& content);
 public:
  void Display()
  {
   map<string,map<string,string> >::iterator mi;
   for(mi=items.begin();mi!=items.end();mi++)
      {
       cout<<mi->first<<endl;
       map<string,string>::iterator mii=mi->second.begin();
       for(;mii!=mi->second.end();mii++)
           cout<<"\t"<<mii->first<<":"<<mii->second<<endl;
      }
  }
};//end class IniParser
//-------------------------------------------------------------------------------------------------
#endif
