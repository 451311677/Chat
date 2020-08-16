#include "ChatClient.hpp"
#include "ChatWindow.hpp"
void menu()
{
	std::cout<<"---------------------------------"<<std::endl;
	std::cout<<"|   1.register        2.login   |"<<std::endl;
	std::cout<<"|                               |"<<std::endl;
	std::cout<<"|   3.logout          4.exit    |"<<std::endl;
	std::cout<<"---------------------------------"<<std::endl;
}

int main(int argc,char* argv[])
{
	
	
	if(argc!=2)
	{
		printf("./ChatClient [ser_ip]\n");
		exit(1);
	}
	ChatClient* cc=new ChatClient(argv[1]);
	if(!cc)
	{
		printf("out of memory\n");
		exit(2);
	}
	cc->InitClient();
	while(1)
	{
		menu();
		std::cout<<"please select service:";
		fflush(stdout);
		int Select=-1;
		std::cin>>Select;
		bool flag=false;
		switch(Select)
		{
			case 1:
			//注册
			{
				if(!cc->Register())
				{
					std::cout<<"Register failed! please try again"<<std::endl;
				}
				else{
					std::cout<<"Register success! please login"<<std::endl;
				}
				break;
			}
			
			case 2:
			//登录
			{
				if(!cc->Login())
				{
					std::cout<<"login failed! please try again"<<std::endl;
				}
				else
				{
					std::cout<<"login success! please chat"<<std::endl;
					ChatWindow* cw=new ChatWindow();
					cw->Start(cc);
					while(1){
						sleep(5);
					}
				}
				break;
			}
			case 3:
			//登出
			{
				if(!cc->LoginOut())
				{
					//出错
				}
				else{
						std::cout<<"loginout success!"<<std::endl;
				}
				
				break;
			}
			
			case 4:
			//退出
			{
				flag=true;
				break;
			}
			default:
			//报错
			{
				printf("\ninput number out of range\n");
			}
			
		}
		if(flag)
		{
			break;
		}
	}
	return 0;
}