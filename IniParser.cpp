#include "IniParser.hpp"
//-------------------------------------------------------------------------------------------------

bool IniParser::Get(const string& section,const string& key,string& value) const
{
 map<string,map<string,string> >::const_iterator mi=items.find(section);
 if(mi==items.end())
    return false;
 map<string,string>::const_iterator mii=mi->second.find(key);
 if(mii==mi->second.end())
    return false;
 value=mii->second;
 return true;
}
//-------------------------------------------------------------------------------------------------

bool IniParser::Set(const string& section,const string& key,const string& value)
{
 map<string,map<string,string> >::iterator mi=items.find(section);
 if(mi==items.end())
   {
    map<string,string> tm;
    tm[key]=value;
    items[section]=tm;
    return true;
   }
 mi->second[key]=value;
 return true;
}
//-------------------------------------------------------------------------------------------------

bool IniParser::ReadFromFile(const string& filename)
{
 string content;
 if(FileGetContent(filename,content))
    return ReadFromString(content);
 return false;
}
//-------------------------------------------------------------------------------------------------

bool IniParser::WriteToFile(const string& filename)
{
 string content;
 if(WriteToString(content))
    return FilePutContent(filename,content);
 return false;
}
//-------------------------------------------------------------------------------------------------

bool IniParser::ReadFromString(const string& content)
{
 vector<string> lines;
 SplitString(content,lines,(char*)("\r\n"));
 string current_section;
 int pos=0;
 for(int i=0;i<int(lines.size());i++)
    {
     string line=lines[i];
     if(
        (int)(pos=line.find("//"))!=int(string::npos)
       )
       {
        if(pos<5||line[pos-1]!=':')
           line=line.substr(0,pos);
       }
     Trim(line);
     if(line.size()<3)
        continue;
     if(line[0]=='['&&line[line.size()-1]==']')
       {
        current_section=line.substr(1,line.size()-2);
        Trim(current_section);
        if(current_section.empty())
           continue;
        map<string,string> tm;
        items[current_section]=tm;
       }
     else if(current_section.empty())
        continue;
     pos=line.find('=');
     if(pos==int(string::npos))
        continue;
     string key=line.substr(0,pos);
     string value=line.substr(pos+1);
     Trim(key);
     Trim(value,true);
     items[current_section][key]=value;
    }//end for
 return true;
}
//-------------------------------------------------------------------------------------------------

bool IniParser::WriteToString(string& content)
{
 content="";
 map<string,map<string,string> >::iterator mi=items.begin();
 for(;mi!=items.end();mi++)
    {
     content+=mi->first+"\n";
     map<string,string>::iterator mii=mi->second.begin();
     for(;mii!=mi->second.end();mii++)
        {
         content+=mii->first+"="+mii->second+"\n";
        }
    }//end for
 return true;
}
//-------------------------------------------------------------------------------------------------
