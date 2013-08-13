#ifndef MAINDEFHPP
#define MAINDEFHPP
#include<string>
using namespace std;
//-------------------------------------------------------------------------------------------------
#define ULL unsigned long long
//-------------------------------------------------------------------------------------------------
const static string EXENAME="dmirror";
const static int ROLE_MAINBASE=1;
const static int ROLE_WATCHER=2;
const static int ROLE_SENDER=3;
//-------------------------------------------------------------------------------------------------
const static int STATE_NULL=0;
const static int STATE_TO_MASTER_WAIT_SLAVE_OK=1;
const static int STATE_MASTER=2;
const static int STATE_MASTER_TO_SLAVE1=3;
const static int STATE_MASTER_TO_SLAVE2=4;
const static int STATE_SLAVE=5;
//-------------------------------------------------------------------------------------------------
//HTTP/TCP/UNIX_SOCKET
const static int LISTEN_QUEUE_LENGTH=128;
const unsigned int SOCKET_READ_BUFFER_LENGTH=4096;

const string TELNET_END_TOKEN="\r\n";
const string NC_END_TOKEN="\n";

const static int HTTP_PROTOCOL=1;
const static int UNIX_SOCKET_PROTOCOL=2;

const static string HTTP_COMMAND_SET_MASTER="setmaster";
const static string HTTP_COMMAND_SET_SLAVE="setslave";
const static string HTTP_COMMAND_GET_STATE="getstate";
const static string HTTP_COMMAND_GET_STATUS="getstatus";
const static string HTTP_COMMAND_SHOW_CONF="showconf";
const static string HTTP_COMMAND_UPDATE_CONF="updateconf";

const static string UNIX_SOCKET_COMMAND_START_WATCHER="startwatcher";
const static string UNIX_SOCKET_COMMAND_STOP_WATCHER="stopwatcher";
const static string UNIX_SOCKET_COMMAND_START_SENDER1="startsender1";
const static string UNIX_SOCKET_COMMAND_STOP_SENDER="stopsender";
const static string UNIX_SOCKET_COMMAND_SET_SENDER_UPDATE_MODE_TRUE="setsenderupdatemodetrue";
const static string UNIX_SOCKET_COMMAND_SET_SENDER_UPDATE_MODE_FALSE="setsenderupdatemodefalse";
const static string UNIX_SOCKET_COMMAND_GET_SENDER_LAST_READ_TIME="getlastreadtime";
const static string UNIX_SOCKET_COMMAND_GET_WATCHER_STATUS="getwatcherstatus";
const static string UNIX_SOCKET_COMMAND_GET_SENDER_STATUS="getsenderstatus";
const static string UNIX_SOCKET_COMMAND_UPDATE_CONF="updateconf";

const static string MAINBASE_UNIX_SOCKET_FILENAME="mainbase.socket";
const static string WATCHER_UNIX_SOCKET_FILENAME="watcher.socket";
const static string SENDER_UNIX_SOCKET_FILENAME="sender.socket";

typedef bool (*COMMAND_FUNCTION)(const string& command,string& res);
//-------------------------------------------------------------------------------------------------
//TIMER
const static int MAINBASE_LOOP_TIME_INTERVAL_MS=1000;
const static int PROCESSES_MONITOR_TIME_INTERVAL_MS=3000;
const static int SENDER_LOOP_TIME_INTERVAL_MS=1000;
//-------------------------------------------------------------------------------------------------
//INOTIFY
const static int INOTIFY_READ_BUFFER_SIZE=16384;
//-------------------------------------------------------------------------------------------------
//RLOG
const static string RLOG_SURFIX=".rlog";
const static int MASTER_WAIT_RLOG_NO_CHANGE_TO_SLAVE_TIME=5;
const static int RLOG_BATCH_PROCESS_TIMEOUT=2;
//-------------------------------------------------------------------------------------------------
//SENDER
const static string STAT_FILE_NAME="stat.dat";
const static string RSYNC_PARAMS="-r --delete --delete-after --ignore-errors";
const static string RSYNC_LIST_FILE_NAME="sender_rsync.list";
const static string RSYNC_ERROR_FILE_NAME="sender_rsync.error";
//-------------------------------------------------------------------------------------------------
#endif
