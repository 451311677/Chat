#pragma once
#include <stdio.h>
#include<unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "ConnectInfo.hpp"
#include "Log.hpp"


//using namespace std;
struct MySelf
{
	std::string NickName_;
	std::string School_;
	std::string Passwd_;
	uint32_t UserId_;
};
class ChatClient
{
	public:
		ChatClient(std::string ip="127.0.0.1")
		{
			TcpSock_= -1;
			TcpPort_=TCP_PORT;
			ip_=ip;
			UdpSock_=-1;
			UdpPort_=UDP_PORT;
		}
		
		void InitClient()
		{
			//TcpSock_=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
			//if(TcpSock_<0)
			//{
			//	printf("create socket failed\n");
			//	exit(1);
			//}
			UdpSock_=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
			if(UdpSock_<0)
			{
				exit(1);
			}
			LOG(INFO,"create udp socket success")<<std::endl;
			
			
		}
		
		bool Connect2Server()
		{
			TcpSock_=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
			if(TcpSock_<0)
			{
				printf("create socket failed\n");
				exit(1);
			}
			
			
			struct sockaddr_in ser_addr;
			ser_addr.sin_family=AF_INET;
			ser_addr.sin_port=htons(TcpPort_);
			ser_addr.sin_addr.s_addr  = inet_addr(ip_.c_str());
			int ret = connect(TcpSock_,(struct sockaddr*)&ser_addr,sizeof(ser_addr));
			if(ret<0)
			{
				perror("connect err");
				return false;
				
			}
			
			return true;
		}
		//注册
		bool Register()
		{
			
			if(!Connect2Server())
			{
				return false;
			}
			
			//1.发送数据包标识
			char type=REGISTER;
			ssize_t sendsize = send(TcpSock_,&type,1,0);
			if(sendsize<0)
			{
				//log
				printf("Register sendsize<0\n");
				return false;
			}
			
			//2.发送注册包信息
			struct RegInfo ri;
			std::cout<<"please enter your nickname:";
			std::cin>>ri.NickName_;
			std::cout<<"please enter your school:";
			std::cin>>ri.School_;
			while(1)
			{
				std::cout<<"please enter your password:";
				std::string passwd_one;
				std::string passwd_two;
				std::cin>>passwd_one;
				std::cout<<"please enter your password again:";
				std::cin>>passwd_two;
				if(passwd_one==passwd_two)
				{
					strcpy(ri.Passwd_,passwd_one.c_str());
					break;
				}else
				{
					printf("The passwords don't match\n");
				}	
			}
			sendsize=send(TcpSock_,&ri,sizeof(ri),0);
			if(sendsize<0)
			{
				//log
				printf("send failed\n");
				return false;
			}
			//3.分析应答包信息，获取状态和用户id
			struct ReplyInfo resp;
			ssize_t recvsize = recv(TcpSock_,&resp,sizeof(resp),0);
			if(recvsize<0)
			{
				printf("recvsize < 0\n");
				return false;
			}
			else if(recvsize==0){
				printf("recvsize == 0");
				return false;
			}
			if(resp.Status_!=REGISTERED)
			{
				//注册失败
				printf("status : %d\n",resp.Status_);
				return false;
			}
			else if(resp.Status_==REGISTERED)
			{
				//注册成功
				//保存用户id
				me_.NickName_=ri.NickName_;
				me_.School_=ri.School_;
				me_.Passwd_=ri.Passwd_;
				me_.UserId_=resp.UserId_;
				
				std::cout<<"==>UserId: "<<me_.UserId_<<std::endl;
				std::cout<<"   nickname: "<<me_.NickName_<<std::endl;
				std::cout<<"   school: "<<me_.School_<<std::endl;
				std::cout<<"   password: "<<me_.Passwd_<<std::endl;
			}
			close(TcpSock_);
			return true;
		}
		
		//登录
		bool Login()
		{
			if(!Connect2Server())
			{
				return false;
			}
			//1.发送登录标识
			char type=LOGIN;
			ssize_t sendsize = send(TcpSock_,&type,1,0);
			if(sendsize<0)
			{
				//log
				return false;
			}
			//2.发送登录数据包
			struct LoginInfo li;
			std::cout<<"please enter your UserId:";
			std::cin>>li.UserId_;
			std::cout<<"please enter your password:";
			std::cin>>li.Passwd_;
			//li.UserId_=me_.UserId_;
			//strcpy(li.Passwd_,me_.Passwd_.c_str());
			
			sendsize=send(TcpSock_,&li,sizeof(li),0);
			if(sendsize<0)
			{
				return false;
			}
			
			
			//3.解析登录结果
			struct ReplyInfo resp;
			ssize_t recvsize = recv(TcpSock_,&resp,sizeof(resp),0);
			if(recvsize<0)
			{
				return false;
			}
			else if(recvsize==0)
			{
				return false;
			}
			if(resp.Status_!=LOGINED)
			{
				//log
				return false;
			}
			else if(resp.Status_==LOGINED)
			{
				printf("Login success\n");
			}
			return true;
			
			
		}
		bool LoginOut()
		{
			
			
			
			return true;
		}
		
		bool SendMsg(const std::string& msg)
		{
			struct sockaddr_in ser_addr;
			ser_addr.sin_family=AF_INET;
			ser_addr.sin_port=htons(UdpPort_);
			ser_addr.sin_addr.s_addr=inet_addr(ip_.c_str());
			
			ssize_t sendsize = sendto(UdpSock_,msg.c_str(),msg.size(),0,(struct sockaddr*)&ser_addr,sizeof(ser_addr));
			if(sendsize<0)
			{
				//考虑失败之后怎么办？补救措施
				return false;
			}
			
			return true;
		
		}
		bool RecvMsg(std::string* msg)
		{
			char buf[10240]={0};
			struct sockaddr_in ser_addr;
			socklen_t ser_addrlen=sizeof(ser_addr);
			
			ssize_t recvsize = recvfrom(UdpSock_,buf,sizeof(buf)-1,0,(struct sockaddr*)& ser_addr,&ser_addrlen);
			if(recvsize<0)
			{
				
				return false;
			}
			msg->assign(buf,recvsize);
			return true;
			
		}
		MySelf& GetMySelf()
		{
			return me_;
		}
		
		void PushOnlineUser(std::string& userinfo)
		{
			auto iter=Online_user.begin();
			for(;iter !=Online_user.end();iter++)
			{
				//如果已经在vector当中，则不添加了，直接返回
				if(*iter == userinfo)
				{
					return ;
				}
			}
			Online_user.push_back(userinfo);
			return ;
		}
		std::vector<std::string>& GetOnlineUser()
		{
			return Online_user;
		}
		
	
	private:
		//TCP的socket和端口
		int TcpSock_;
		int TcpPort_;
		//服务端的ip地址
		std::string ip_;
		
		//客户端自己的信息
		MySelf me_;
		
		//UDP的socket和端口
		int UdpSock_;
		int UdpPort_;
		
		//课下增加客户端的在线列表， 收到一个消息，就保存下[昵称-学校]
		std::vector<std::string> Online_user;
		
		
};


