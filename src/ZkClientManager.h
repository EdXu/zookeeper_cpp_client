/*
 * ZkClientManager.h
 *
 *      Created on: 2016��2��20��
 *      Author: ZengHui Bao (bao_z_h@163.com)
 */

#ifndef __ZK_CLIENT_MANAGER_H
#define __ZK_CLIENT_MANAGER_H


#include <string>
#include <map>
#include <vector>
#include "muduo/base/Singleton.h"
#include "muduo/base/Thread.h"
#include "muduo/base/Mutex.h"
#include "muduo/base/Logging.h"
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "ZkTimerQueue.h"
#include "ZkClient.h"


using namespace muduo::net;
using namespace muduo;



namespace ZkCppClient
{

class ZkClientManager : boost::noncopyable
{
public:
	ZkClientManager();

	~ZkClientManager();

	ZkClientPtr __getZkClient(uint32_t handle);

	int getFirstThreadId()
	{
		if (zkThreads_.empty() == false && zkThreads_[0] != NULL)
		{
            return (zkThreads_[0])->tid();
		}
        return 0;
	}

	int getSecondThreadId()
	{
		if (zkThreads_.empty() == false && (zkThreads_.size() >= 2) && zkThreads_[1] != NULL)
		{
			return (zkThreads_[1])->tid();
		}
		return 0;
	}

public:
	//�������ӿ�
    //��Ҫ�ȵ�����ӿ� �� ��ʼ�� ��־��صĲ�������־�߳�
    static bool setLogConf(bool isDebug, const std::string& zkLogFilePath = "");

	static ZkClientManager& instance() {return muduo::Singleton<ZkClientManager>::instance();};

    //����һ��ZkClient, ��������ɹ����᷵�� ����Ӧ��handle; ����ʧ�ܣ�����0 (�̰߳�ȫ)
	//SessionClientIdͨ��ΪNULL, ����ΪNLL���� SessionClientId ���� �ѽ����ɹ���zkclient�� clientId.
    uint32_t createZkClient(const std::string& host, int timeout, SessionClientId *clientId = NULL,
                        ZkUtil::SessionExpiredHandler expired_handler = NULL, void* context = NULL);

    //����handle������ZkClientPtr����.����Ҳ��������ؿ�ָ���shared_ptr. (�̰߳�ȫ)
    ZkClientPtr getZkClient(uint32_t handle);

	void destroyClient(uint32_t handle);

private:
	void init();
	void LoopFun();
	void loop_once(int epollfd, int waitms, uint64_t loop_index);

private:

	std::vector<muduo::Thread*> zkThreads_;     //��Ҫ���ж�ʱ�� �� runInThread ע��ĺ���
	volatile bool isExit_;  /* atomic */        //һ��ȫ�ֵĿ��أ������߳��Ƿ�ֹͣ ����

	std::map<uint32_t, ZkClientPtr> totalZkClients_;  //map<handle, ZkClientPtr>
	muduo::MutexLock clientMutex_;

	std::vector<ZkNetClient*> zkNetClients_;

	uint32_t nextHandle_;
};


//����runInThread�Ļص�����
class CbFunManager : boost::noncopyable
{
public:
	typedef boost::function<void()> Functor;
	friend class ZkClientManager;

	//Ϊ�˼ӿ��ٶȣ�ÿ���̵߳�epollfd��eventfd�����ݣ�һ�ݴ��߳�˽�����ݣ�һ�ݴ�threadDatas_
	struct threadData
	{
		threadData()
		{
			epollfd_ = 0;
			eventfd_ = 0;
			callingPendingFunctors_ = false;
		}
		int epollfd_;    
		int eventfd_;
		volatile bool callingPendingFunctors_; /* atomic */
	};

public:
	CbFunManager();
	~CbFunManager();

	// singleton
	static CbFunManager& instance();

	void runInThread(int thread_id, const Functor& cb);

private:
	//����eventfd�������� ��eventfd���¼� ע�ᵽepoll��.
	int insertOrSetThreadData(int thread_id, int epollfd);

	bool isInCurrentThread(int thread_id);

	void queueInThreadFuns(int thread_id, const Functor& cb);

	void wakeup(int thread_id);

	void doPendingFunctors(int thread_id);

private:
	std::map<int, threadData> threadDatas_;   //<thread_id, threadData>
	muduo::MutexLock dataMutex_;

	std::map<int, std::vector<Functor> >* pendingFunctors_;  //<thread_id, cbfun_list>
	muduo::MutexLock funsMutex_;
};


class ZkTimerManager : boost::noncopyable
{
public:
	friend class ZkClientManager;

	ZkTimerManager()
	{
		timerQueues_ = new std::map<int, ZkTimerQueue*>();
		timerQueues_->clear();
	}

	~ZkTimerManager();

	// singleton
	static ZkTimerManager& instance();

	void runAt(int thread_id, const Timestamp& time, const TimerCallback& cb);

	//delay ��λΪ�룬������С��
	void runAfter(int thread_id, double delay, const TimerCallback& cb);

	//interval ��λΪ�룬������С��
	void runEvery(int thread_id, double interval, const TimerCallback& cb);

private:
	//����timerfd
	int insertTimeQueue(int thread_id, int epollfd);

public:
	//��Ϊ SslTimerQueue�е�timerfd��Ӧ���ظ�����������SslTimerQueue�ǲ��ɸ��Ƶģ����� ��ָ��
	std::map<int, ZkTimerQueue*>* timerQueues_;  
	muduo::MutexLock timeMutex_;
};

}

#endif
