#pragma
#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<iostream>
#include<sys/time.h>
//等级：info/warning/error/fatal/debug
const char* Level[] = {
	"INFO",
	"WARNING",
	"ERROR",
	"FATAL",
	"DEBUG"
};
enum LogLevel
{
	INFO = 0,
	WARNING,
	ERROR,
	FATAL,
	DEBUG
};

class LogTime
{
	public:
		static void  GetTimeStamp(std::string* time_stamp)
		{
			time_t Systime; 
			time(&Systime);
			
			struct tm* ST=localtime(&Systime);
			char TimeNow[23]={'\0'};
			snprintf(TimeNow,sizeof(TimeNow)-1,"%04d-%02d-%02d %02d:%02d:%02d",ST->tm_year+1900,ST->tm_mon+1,ST->tm_mday,ST->tm_hour,ST->tm_min,ST->tm_sec);
			time_stamp->assign(TimeNow);
		}
};

//[时间 等级 文件 行号] 具体日志信息
inline std::ostream& Log(LogLevel lev,const char * file,int line,const std::string& log_msg)
{
	std::string log_level=Level[lev];
	std::string timestamp;
	
	LogTime::GetTimeStamp(&timestamp);
	std::cout <<"[ "<<timestamp<<" "<<log_level<<" "<<file<<" : "<<line<<" ] "<<log_msg;
	return std::cout;
}

#define LOG(lev,msg) Log(lev,__FILE__,__LINE__,msg)











