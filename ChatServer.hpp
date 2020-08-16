#pragma once
//头文件
//套接字编程用到的头文件
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include <unistd.h>
#include <iostream>
#include <string>
#include <pthread.h>
#include <cstdlib>

#include "ConnectInfo.hpp"
#include "UserManager.hpp"
#include "MsgPool.hpp"
#include "Log.hpp"
#define THREAD_COUNT 2

class ClientConnectInfo
{
	public:
		ClientConnectInfo(int Sock,void* Server)
		{
			Sock_=Sock;
			Server_=Server;
		}
		
		int GetSock()
		{
			return Sock_;
		}
		
		void* GetServer(){
			return Server_;
		}
	private:
		int Sock_;	//保存服务端为客户端创建出来的套接字描述符
		void* Server_;	//保存ChatServer类实例化指针，因为在线程入口函数当中，没有this指针，在入口函数当中想要和用户管理模块打交道，就需要拿到this指针

};

class ChatServer
{
	public:
		ChatServer()
		{
			TcpSock_ = -1;
			TcpPort_ = TCP_PORT;
			//端口 ：uint16_t 0~65536
			UserMana_=NULL;
			UdpSock_=-1;
			UdpPort_=UDP_PORT;
			MsgPool_=NULL;
		}
		~ChatServer()
		{
			//释放
			if(UserMana_)
			{
				delete UserMana_;
				UserMana_=NULL;
			}
			if(MsgPool_)
			{
					delete MsgPool_;
					MsgPool_=NULL;
			}
		}
		void InitServer()
		{
			UserMana_ = new UserManager();
			if(!UserMana_)
			{
				LOG(ERROR,"create user manager failed");
				exit(1);
			}
			LOG(INFO,"create user manager success")<<std::endl;
			MsgPool_=new MsgPool();
			if(!MsgPool_)
			{
				exit(2);
			}
			//创建TCP套接字
			//TcpSock_=socket(AF_INET,SOCK_STREAM,0);
			TcpSock_=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
			if(TcpSock_<0)
			{
				//直接导致程序退出
				//打印日志
				printf("socket failed\n");
				exit(1);
			}
			//定义服务端侦听的地址信息
			// ip:
			// port
			struct sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_port = htons(TcpPort_);
			//(172.16.99.129)-->点分十进制的ip地址
			//1.点分十进制的ip地址转换为uint32_t
			//2.将主机字节序转换为网络字节序
			//0.0.0.0标识当前主机的所有网卡的ip地址
			addr.sin_addr.s_addr=inet_addr("0.0.0.0");
			int ret=bind(TcpSock_,(struct sockaddr*)&addr,sizeof(addr));
			if(ret<0)
			{
				printf("bind failed\n");
				exit(2);
			}
			ret=listen(TcpSock_,5);
			if(ret<0)
			{
				printf("listen failed\n");
				exit(3);
			}
			
			//初始化UDP
			UdpSock_ = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
			if(UdpSock_<0)
			{
				//log
				exit(4);
			}
			struct sockaddr_in udpaddr;
			udpaddr.sin_family=AF_INET;
			udpaddr.sin_port = htons(UDP_PORT);
			udpaddr.sin_addr.s_addr=inet_addr("0.0.0.0");
			ret = bind(UdpSock_,(struct sockaddr*)&udpaddr,sizeof(udpaddr));
			if(ret<0)
			{
				//log
				exit(5);
				
			}
		
		}
		int Start()
		{
			//创建生产者消费者线程
			//处理UDP线程
			
			for(int i=0;i<THREAD_COUNT;i++)
			{
				//创建消费线程
				pthread_t tid;
				int ret = pthread_create(&tid,NULL,ConsumeMsgStart,(void*)this);
				if(ret<0)
				{
					exit(1);
				}
				//创建生产线程
				ret = pthread_create(&tid,NULL,ProductMsgStart,(void*)this);
				if(ret<0)
				{
					exit(1);
				}
			}
			
			
			while(1)
			{
				
				struct sockaddr_in cli_addr;
				socklen_t cli_addrlen=sizeof(cli_addr);
				//当没有客户端连接服务器，已完成连接队列当中没有三次握手完成的连接
				//调用accept函数，会阻塞（调用accept函数，则阻塞的时候，改函数不会返回）
				int newfd =  accept(TcpSock_,(struct sockaddr*)&cli_addr,&cli_addrlen);
				if(newfd<0)
				{
					continue;
				}
				
				//组织线程入口函数需要的参数
				ClientConnectInfo* cc=new ClientConnectInfo(newfd,(void*)this);
				if(!cc){
					continue;
				}
				
				
				//创建线程
				pthread_t tid;
				int ret = pthread_create(&tid,NULL,LoginAndRegisterStart,(void*)cc);
				if(ret<0)
				{
					continue;
				}
				
				
			}
		}
	private:
		
		static void* ConsumeMsgStart(void* arg)
		{
			LOG(INFO,"ConsumeMsgStart")<<std::endl;
			pthread_detach(pthread_self());
			ChatServer* cs=(ChatServer*)arg;
			//1.从队列中（消息池）获取数据
			//2.根据在线用户群发消息
			while(1)
			{
				cs->BroadcastMsg();
			}
			return NULL;
		}
		
		static void* ProductMsgStart(void* arg)
		{
			LOG(INFO,"ProductMsgStart")<<std::endl;
			pthread_detach(pthread_self());
			ChatServer* cs=(ChatServer*)arg;
			//1.从网络上接收数据，然后放入消息池
			while(1)
			{
				//接收消息,UDP数据
				cs->RecvMsg();
			
			}
			return NULL;
		}
	
		static void* LoginAndRegisterStart(void* arg)
		{
			ClientConnectInfo* cc=(ClientConnectInfo*)arg;
			ChatServer* cs=(ChatServer*)cc->GetServer();
			//1.区分到底是注册请求还是登录请求
			char Ques_type=-1;
			
			ssize_t recvsize = recv(cc->GetSock(),&Ques_type,1,0);
			if(recvsize<0)
			{
				//如果线程的入口函数执行结束，则该线程就退出了
				return NULL;
			}
			else if(recvsize==0)
			{
				//对端关闭掉连接
				return NULL;
			}
			//2.switch 判断（注册请求|登录请求）
			
			uint32_t UserId = -1;
			int UserStatus = -1;
			switch(Ques_type)
			{
				case REGISTER:
					//处理注册请求
					{
						UserStatus = cs->DealRegister(cc->GetSock(),&UserId);
						break;
					}
				case LOGIN:
					//处理登录请求
					{
						UserStatus = cs->DealLogin(cc->GetSock());
					
						break;
					}
				case LOGINOUT:
					//处理登出请求
					{
					
						break;
					}
				default:
					//处理异常情况
					{
						
					}
			}
			//3.回复应答
			struct ReplyInfo ri;
			ri.Status_=UserStatus;
			ri.UserId_=UserId;
			ssize_t sendsize = send(cc->GetSock(),&ri,sizeof(ri),0);
			if(sendsize<0)
			{
				//考虑发送失败是否要重新发送
				//log
			}
			//发送成功
			close(cc->GetSock());
			delete cc;
			return NULL;
		}
		//Sock为了从网络上接收注册请求
		//UserId:出参，为了返回给客户端的用户id
		int DealRegister(int Sock,uint32_t* UserId)
		{
			//1.能从网络上接收数据
			struct RegInfo ri;
			ssize_t recvsize = recv(Sock,&ri,sizeof(ri),0);
			if(recvsize<0)
			{
				return REGFAILED;
				
			}
			else if(recvsize==0)
			{
				//对方关闭连接，特殊处理
			}
			
			//注册用户
			//2.能够计算出一个独一无二的用户id
			int ret = UserMana_->Register(ri.NickName_,ri.School_,ri.Passwd_,UserId);
			
			if(ret<0)
			{
				return REGFAILED;
			}
			//.通过返回值告诉调用者，是否注册成功
			return REGISTERED;
		
		}
		int DealLogin(int Sock)
		{
				struct LoginInfo li;
				ssize_t recvsize = recv(Sock,&li,sizeof(li),0);
				if(recvsize<0)
				{
					return LOGINFAILED;
				}
				else if(recvsize==0)
				{
					//特殊处理
				}
				int ret = UserMana_->Login(li.UserId_,li.Passwd_);
				if(ret<0)
				{
					//登录失败
					return LOGINFAILED;
				}
				return LOGINED;


				
		}
		int DealLoginOut()
		{
		
		}
	private:
		void RecvMsg()
		{
			char buf[10240]={0};
			struct sockaddr_in cli_addr;
			socklen_t cli_addrlen=sizeof(struct sockaddr_in);
			
			ssize_t recvsize = recvfrom(UdpSock_,buf,sizeof(buf)-1,0,(struct sockaddr*)&cli_addr,&cli_addrlen);
			
			if(recvsize<0)
			{
				//接收失败
				LOG(ERROR,"RecvMsg recvsize<0")<<std::endl;
				return;
			}
			//LOG(INFO,"RecvMsg success")<<std::endl;
			std::string msg;
			msg.assign(buf,recvsize);
			
			Message json_msg;
			json_msg.Deserialize(msg);
			
			//std::cout<<json_msg.UserId_<<"-"<<json_msg.NickName_<<"#"<<json_msg.Msg_<<std::endl;
			
			
			//正常收到数据的逻辑
			//转换为Json字符串，获取到发送的消息是什么
			//1.NickName 2.School 3.msg数据 4.UserId
			//通过UserId来判断消息是否合法，
			//		4.1这个消息是否是已经登录的人发出来的
			//		4.2这个消息是否是发送过消息的人发出来的
			bool ret = UserMana_->IsLogin(json_msg.GetUserId(),cli_addr,cli_addrlen);
			if(!ret)
			{
				//直接不处理
				return ;
			}
			//如果是合法，将该条数据发送到消息池当中去。
			MsgPool_->PushMsg2Pool(msg);
			
			
		}
		void BroadcastMsg()
		{
			//1.获取发送的消息内容从消息池当中
			std::string msg;
			MsgPool_->PopMsgFromPool(&msg);
			
			//2.获取在线的用户列表，获取OnlineUserVec_
			std::vector<UserInfo> online_vec;
			UserMana_->GetOnlineUserVec(&online_vec);
			for(size_t i=0;i<online_vec.size();i++)
			{
				//调用发送接口发送出去
				SendMsg(msg,online_vec[i].GetCliAddrInfo(),online_vec[i].GetCliAddrLen());
			}
		}
		void SendMsg(const std::string& msg,const struct sockaddr_in& cli_addr,const socklen_t& len)
		{
			ssize_t sendsize = sendto(UdpSock_,msg.c_str(),msg.size(),0,(struct sockaddr*)&cli_addr,len);
			if(sendsize<0)
			{
				//是否需要将数据缓存下来，之后再推送一遍
			}
			//log成功，打印日志	
		}
		
	private:
		//  TCP协议
		//  TCP侦听套接字
		int TcpSock_;
		//	TCP端口
		int TcpPort_;
		//	用户管理模块
		UserManager* UserMana_;
		
		int UdpSock_;
		int UdpPort_;
		
		MsgPool* MsgPool_;
		
	
};






