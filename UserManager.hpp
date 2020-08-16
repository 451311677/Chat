#pragma once
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>
#include <cstring>
#include <string>
#include <unordered_map>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <vector>

#define OFFLINE 	   		0
#define REGISTER_SUCCESS 	1
#define USERLOGINED 		2
#define ONLINE 				3

//描述单个用户的信息
class UserInfo
{
	public:
		UserInfo(const std::string& NickName,const std::string& School,const std::string& Passwd,uint32_t UserId)
		{
			NickName_=NickName;
			School_=School;
			Passwd_=Passwd;
			UserId_=UserId;
			UserStatus_=OFFLINE;
			memset(&CliAddr_, '0', sizeof(struct sockaddr_in));
			CliAddrLen_=-1;
		}
		~UserInfo(){
			
		}
		void setUserStatus(int Status)
		{
			UserStatus_=Status;
			
		}
		std::string& GetPasswd()
		{
			return Passwd_;
		}
		int GetUserStatus()
        {
            return UserStatus_;
        }


		
		void SetCliAddrInfo(const struct sockaddr_in& addr)
		{
			memcpy(&CliAddr_,&addr,sizeof(addr));
		}
		void SetCliAddrLen(const socklen_t& len)
		{
			CliAddrLen_=len;
		}
		struct sockaddr_in& GetCliAddrInfo()
		{
			return CliAddr_;
		}
		socklen_t& GetCliAddrLen()
		{
			return CliAddrLen_;
		}
	
	private:
		std::string NickName_;
		std::string School_;
		std::string Passwd_;
		//用户ID
		uint32_t UserId_;
		//当前用户的状态，描述当前用户是注册过，还是登陆过
		int UserStatus_;
	
		//保存客户端对应的UDP地址
		struct sockaddr_in CliAddr_;
		socklen_t CliAddrLen_;
	
};


class UserManager
{
	public:
		UserManager()
		{
			UserMap_.clear();
			
			//初始化互斥锁
			pthread_mutex_init(&MapLock_,NULL);
			PrepareUserId_ = 0;
		}
		~UserManager()
		{
			pthread_mutex_destroy(&MapLock_);
		}
		
		//处理注册
		int Register(const std::string& NickName,const std::string& School,const std::string& Passwd,uint32_t* UserId)
		{
			if(NickName.size()==0||School.size()==0||Passwd.size()==0)
			{
				return -1;
			}
			
			//加锁
			pthread_mutex_lock(&MapLock_);
			UserInfo user_info(NickName,School,Passwd,PrepareUserId_);
			
			//更改当前用户的状态
			user_info.setUserStatus(REGISTER_SUCCESS);
			
			
			//插入到map当中去
			UserMap_.insert(std::make_pair(PrepareUserId_,user_info));
			*UserId=PrepareUserId_;
			PrepareUserId_++;
			pthread_mutex_unlock(&MapLock_);
			return 0;
		}
		//处理登录
		int Login(const uint32_t& UserId,const std::string& Passwd)
		{
			if(Passwd.size()<0)
			{
				return -1;
			}
			
			int LoginStatus = -1;
			
			std::unordered_map<uint32_t, UserInfo>::iterator iter;
			//加锁
			pthread_mutex_lock(&MapLock_);
			iter = UserMap_.find(UserId);
			if(iter!=UserMap_.end())
			{
				//查找到了,对比密码
				//item->second===UserInfo
				if(Passwd==iter->second.GetPasswd())
				{
					//用户状态更改
					iter->second.setUserStatus(USERLOGINED);
					LoginStatus=0;
					
					
					
				}else{
					LoginStatus=-1;
				}
			}
			else
			{
				//没有该用户
				LoginStatus=-1;
			}
			//解锁
			pthread_mutex_unlock(&MapLock_);
			return LoginStatus;
			
		}
		bool IsLogin(uint32_t UserId,const struct sockaddr_in& cli_addr,const socklen_t& cli_addrlen)
		{
			//1.判断当前用户Id是否存在
			std::unordered_map<uint32_t,UserInfo>::iterator iter;
			pthread_mutex_lock(&MapLock_);
			iter=UserMap_.find(UserId);
			if(iter==UserMap_.end())
			{
				pthread_mutex_unlock(&MapLock_);
				//用户不存在
				return false;
			}
			//2.判断当前用户状态是否完成注册登录
			int UserStatus=iter->second.GetUserStatus();
			if(UserStatus==OFFLINE||UserStatus==REGISTER_SUCCESS)
			{
				pthread_mutex_unlock(&MapLock_);
				return false;
			}
			
			//3.判断当前用户是否是第一次发送消息
			if(UserStatus==ONLINE)
			{
				pthread_mutex_unlock(&MapLock_);
				return true;
			}
			//4.该用户是第一次发送数据，为了缓存客户端的地址信息
			
			if(UserStatus==USERLOGINED)
			{
				UserInfo us=iter->second;
				us.SetCliAddrInfo(cli_addr);
				us.SetCliAddrLen(cli_addrlen);
				//更改用户状态为ONLINE
				us.setUserStatus(ONLINE);
				
				//统计在线用户
				OnlineUserVec_.push_back(us);
				
			}
			pthread_mutex_unlock(&MapLock_);
			return true;
		}
		void GetOnlineUserVec(std::vector<UserInfo>* vec)
		{
			*vec = OnlineUserVec_;
		}
	private:
		//保存所用用户的信息--》注册过但还没有登录，注册过已经登录的
		std::unordered_map<uint32_t,UserInfo> UserMap_;
		pthread_mutex_t MapLock_;
		
		//预分配的UserId,相当于记录当前UserId分配到哪里
		uint32_t PrepareUserId_;
		
		std::vector<UserInfo> OnlineUserVec_;
};
