#pragma once

#include <stdio.h>
#include <unistd.h>
#include <ncurses.h>
#include <pthread.h>

#include "ChatClient.hpp"


class ChatWindow;
class Param
{
	public:
		Param(ChatWindow* win,int thread_num,ChatClient* chatcli)
		{
			win_=win;
			thread_num_=thread_num;
			chatcli_=chatcli;
		}
	
		ChatWindow* win_;
		int thread_num_;
		ChatClient* chatcli_;
};

class ChatWindow
{
	public:
		ChatWindow()
		{
			header_=NULL;
			output_=NULL;
			user_list_=NULL;
			input_=NULL;
			pthread_mutex_init(&lock_,NULL);
			
			//初始化屏幕，进入到curses模式，必须以endwin()函数结束
			initscr();
			//设置光标是否可见，0不可见，1可见，2完全可见
			curs_set(0);
			
		}
		~ChatWindow()
		{
			if(header_)
			{
				delwin(header_);
				header_=NULL;
			}
			if(output_)
			{
				delwin(output_);
				output_=NULL;
			}
			
			if(user_list_)
			{
				delwin(user_list_);
				user_list_=NULL;
			}
			if(input_)
			{
				delwin(input_);
				input_=NULL;
			}
			
			//endwin
			endwin();
			pthread_mutex_destroy(&lock_);
		}
		
		void Start(ChatClient* chatcli)
		{
			pthread_t tid;
			for(int i=0;i<4;i++)
			{
				Param* pm=new Param(this,i,chatcli);
				//传参：线程编号，当前chatwindow类的指针，chatclient类的指针
				int ret=pthread_create(&tid,NULL,DrawWindow,(void*)pm);
				if(ret<0)
				{
					exit(1);
				}
			}
		}
		static void* DrawWindow(void* arg)
		{
			Param* pm=(Param*)arg;
			ChatWindow* cw=pm->win_;
			int thread_num=pm->thread_num_;
			ChatClient* cc=pm->chatcli_;
			switch(thread_num)
			{
				case 0:
				{
					//header
					RunHeader(cw);
					break;
				}
				case 1:
				{
					//output
					RunOutput(cw,cc);
					break;
				}
				case 2:
				{
					//user_list
					RunUserList(cw,cc);
					break;
				}
				case 3:
				{
					//input
					RunInput(cw,cc);
					break;
				}
				default:
				{
					break;
				}
				
			}
			delete pm;
			return NULL;
			
		}
		void DrawHeader()
		{
			//计算当前方框的占用行数和列数
			int lines=LINES/5;
			int cols=COLS;
			
			//左上角
			int start_y = 0;
			int start_x = 0;
			header_=newwin(lines,cols,start_y,start_x);
			box(header_,0,0);
			//刷新窗口
			pthread_mutex_lock(&lock_);
			wrefresh(header_);
			pthread_mutex_unlock(&lock_);
		}
		//line:从哪一行开始展示
		//cols:从哪一列开始展示
		//打印字符串
		void PutStrToWin(WINDOW* cw,int line,int cols,std::string& msg)
		{
			mvwaddstr(cw,line,cols,msg.c_str());
			//刷新窗口
			pthread_mutex_lock(&lock_);
			wrefresh(cw);
			pthread_mutex_unlock(&lock_);
		}
		
		static void RunHeader(ChatWindow * cw)
		{
			//绘制窗口
			//展示欢迎语
			std::string msg="Welcome to our chat system";
			int y,x;
			size_t pos = 1;
			//flag==0,向右移动
			//flag==1,向左移动
			int flag=0;
			while(1)
			{
				cw->DrawHeader();
				//能够获取窗口的行和列
				getmaxyx(cw->header_,y,x);
				cw->PutStrToWin(cw->header_,y/2,pos,msg);
				//判断是否到达左边界,pos++
				if(pos<2)
				{
					flag=0;
				}
				else if(pos>x-msg.size()-2)
				{
					flag=1;
				}
				if(flag==0)
				{
					pos++;
				}
				else if(flag==1)
				{
					pos--;
				}
				sleep(1);
				//判断是否到达右边界,pos--
			}
		}
		void DrawOutput()
		{
			int lines=(LINES*3)/5;
			int cols=(COLS*3)/4;
			int start_y=LINES/5;
			int start_x=0;
			
			output_=newwin(lines,cols,start_y,start_x);
			box(output_,0,0);
			//刷新窗口
			pthread_mutex_lock(&lock_);
			wrefresh(output_);
			pthread_mutex_unlock(&lock_);
		}
		static void RunOutput(ChatWindow * cw,ChatClient* cc)
		{
			std::string recv_msg;
			Message msg;
			cw->DrawOutput();
			int line=1;
			int y,x;
			while(1)
			{
				getmaxyx(cw->output_,y,x);
				cc->RecvMsg(&recv_msg);
				msg.Deserialize(recv_msg);
				
				//昵称-学校#消息
				std::string show_msg;
				show_msg+=msg.NickName_;
				show_msg+="-";
				show_msg+=msg.School_;
				show_msg+="# ";
				show_msg+=msg.Msg_;
				if(line>y-2)
				{
					line=1;
					cw->DrawOutput();
				}
				cw->PutStrToWin(cw->output_,line,3,show_msg);
				line++;
				
				//更新在线用户列表
				std::string user_info;
				user_info+=msg.NickName_;
				user_info+="-";
				user_info+=msg.School_;
				
				cc->PushOnlineUser(user_info);
			}
		}
		void DrawUserList()
		{
			int lines=(LINES*3)/5;
			int cols=COLS/4;
			int start_y=LINES/5;
			int start_x=(COLS*3)/4;
			
			user_list_=newwin(lines,cols,start_y,start_x);
			box(user_list_,0,0);
			//刷新窗口
			pthread_mutex_lock(&lock_);
			wrefresh(user_list_);
			pthread_mutex_unlock(&lock_);	
		}
		static void RunUserList(ChatWindow * cw, ChatClient* cc)
		{
			int y,x;
			while(1)
			{
				cw->DrawUserList();
				
				//展示在线用户
				getmaxyx(cw->user_list_,y,x);
				std::vector<std::string> user_list = cc->GetOnlineUser();
				int line=1;
				for(auto& iter:user_list)
				{
					
					cw->PutStrToWin(cw->user_list_,line++,4,iter);
				}
				sleep(1);
			}
		}
		void DrawInput()
		{
			int lines=LINES/5;
			int cols=COLS;
			int start_y=(LINES*4)/5;
			int start_x=0;
			input_=newwin(lines,cols,start_y,start_x);
			box(input_,0,0);
			//刷新窗口
			pthread_mutex_lock(&lock_);
			wrefresh(input_);
			pthread_mutex_unlock(&lock_);
		}
		
		void GetStrFromWin(WINDOW* cw,std::string* msg)
		{
			char buf[10240] = {0};
			wgetnstr(cw,buf,sizeof(buf)-1);
			msg->assign(buf,strlen(buf));
		}
		
		static void RunInput(ChatWindow* cw,ChatClient* cc)
		{
			Message msg;
			msg.NickName_=(cc->GetMySelf()).NickName_;
			msg.School_=(cc->GetMySelf()).School_;
			msg.UserId_=(cc->GetMySelf()).UserId_;
			
			std::string tips="Please enter#";
			std::string enter_msg;
			std::string send_msg;
			
			while(1)
			{
				cw->DrawInput();
				cw->PutStrToWin(cw->input_,2,2,tips);
				cw->GetStrFromWin(cw->input_,&enter_msg);
				msg.Msg_=enter_msg;
				
				//序列化
				msg.Serialize(&send_msg);
				
				cc->SendMsg(send_msg);
			}
		}		
	
	private:
		WINDOW* header_;
		WINDOW* output_;
		WINDOW* user_list_;
		WINDOW* input_;
		
		pthread_mutex_t lock_;
	
};






