#pragma
#include <iostream>
#include <queue>
#include <pthread.h>

#define MSG_POOL_SIZE 1024

class MsgPool
{
	public:
		MsgPool()
		{
			Capacity_=MSG_POOL_SIZE;
			pthread_mutex_init(&MsgQueLock_,NULL);
			pthread_cond_init(&SynProQue_,NULL);
			pthread_cond_init(&SynComQue_,NULL);
			
		}
		~MsgPool()
		{
			pthread_mutex_destroy(&MsgQueLock_);
			pthread_cond_destroy(&SynProQue_);
			pthread_cond_destroy(&SynComQue_);
		}
		//插入数据的接口
		void PushMsg2Pool(std::string& msg)
		{
			pthread_mutex_lock(&MsgQueLock_);
			while(MsgQue_.size()>=Capacity_)
			{
				pthread_cond_wait(&SynProQue_,&MsgQueLock_);
			}
			MsgQue_.push(msg);
			pthread_mutex_unlock(&MsgQueLock_);
			//通知消费者
			pthread_cond_signal(&SynComQue_);
		}
		//pop数据的接口
		void PopMsgFromPool(std::string* msg)
		{
			pthread_mutex_lock(&MsgQueLock_);
			while(MsgQue_.empty())
			{
				pthread_cond_wait(&SynComQue_,&MsgQueLock_);
			}
			*msg = MsgQue_.front();
			MsgQue_.pop();
			pthread_mutex_unlock(&MsgQueLock_);
			pthread_cond_signal(&SynProQue_);
		}
	
	private:
		std::queue<std::string> MsgQue_;
		//人为限制queue的容量
		size_t Capacity_;
		//互斥
		pthread_mutex_t MsgQueLock_;
		//同步
		//生产者对应的条件变量
		pthread_cond_t SynProQue_;
		//消费者对应的条件变量
		pthread_cond_t SynComQue_;
		
};