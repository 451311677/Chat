#pragma once
#include <iostream>
#include<string>
#include<json/json.h>

#define REGISTER 0  //注册请求1字节传递'0'值
#define LOGIN 1		//登录请求1字节传递'1'值
#define LOGINOUT 2	//退出请求1字节传递'2'值

#define TCP_PORT 12138
#define UDP_PORT 15000
//注册包格式
struct RegInfo
{
	char NickName_[15];
	char School_[20];
	char Passwd_[20];
};
//登陆包格式
struct LoginInfo
{
	//用户ID
	uint32_t UserId_;
	char Passwd_[20];

};
//应答的信息
struct ReplyInfo
{
	//注册的时候返回给客户端
	uint32_t UserId_;
	//当前状态
	//标识注册是否成功 | 标识登录是否成功
	int Status_;
};

//为应答当前状态准备
enum UserStatus
{
	REGFAILED=0,	//注册失败
	REGISTERED,		//注册成功
	LOGINFAILED,	//登录失败
	LOGINED			//登录成功
};
class Message
{
	public:
		//{key:value,key:value.....}
		//反序列化
		void Deserialize(std::string Message)
		{
			Json::Reader reader;
			Json::Value val;
			reader.parse(Message,val,false);
			
			NickName_=val["NickName"].asString();
			School_ = val["School"].asString();
			Msg_ = val["Msg"].asString();
			UserId_=val["UserId"].asInt();
		}	
		void Serialize(std::string* msg)
		{
			Json::Value val;
			val["NickName"]=NickName_;
			val["School"]=School_;
			val["Msg"]=Msg_;
			val["UserId"]=UserId_;
			
			Json::FastWriter wri;
			*msg=wri.write(val);
		}
		uint32_t GetUserId()
		{
			return UserId_;
		}
	//private:
	public:
		std::string NickName_;
		std::string School_;
		std::string Msg_;
		uint32_t UserId_;
};