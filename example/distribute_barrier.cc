/*
 * distribute_barrier.cc
 *
 *      Description: ����zookeeperʵ�� �ֲ�ʽ����
 *      Created on: 2016��2��21��
 *      Author: ZengHui Bao (bao_z_h@163.com)
 */



#include "ZkClient.h"
#include "ZkClientManager.h"
#include <stdio.h>
#include <string>
#include<stdlib.h>
#include <vector>
#include <assert.h>
#include <algorithm>
#include <unistd.h>
#include <queue>
#include "muduo/base/Condition.h"
#include "muduo/base/Mutex.h"
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

using namespace std;
using namespace ZkCppClient;

#define     ZOOKEEPER_SERVER_CONN_STRING        "127.0.0.1:2181,127.0.0.1:2182,127.0.0.1:2183"


class DistriBarrier : boost::noncopyable
{
public:
    //�������ӿ�
    DistriBarrier() :
            mutex_(),
            condition_(mutex_)
    {
        zkConnStr_ = "";
        isInit_ = false;
        barrNum_ = 0;
    };

    bool init(const std::string& zkConnStr, int barrier_num)
    {
        if (isInit_ == false)
        {
            barrNum_ = barrier_num;
            zkConnStr_ = zkConnStr;
            //����zookeeper��־·��
            if (ZkClientManager::setLogConf(true, "./zk_log") == false)
            {
                printf("setLogConf failed!\n\n");
                return false;
            }

            //����һ��session
            uint32_t handle = ZkClientManager::instance().createZkClient(zkConnStr_, 30000, NULL, NULL, NULL);
            if (handle == 0)
            {
                printf("create session failed! connStr:%s\n", zkConnStr_.c_str());
                return false;
            }

            //ͨ��session handle����ȡZkClient
            zkClient_ = ZkClientManager::instance().getZkClient(handle);

            //���� ��·��
            bool isTemp = false;
            bool isSeq = false;
            std::string retPath;
            char value[48] = {0};
            snprintf(value, sizeof(value), "%d", barrNum_);
            ZkUtil::ZkErrorCode ec = zkClient_->create(parentPath_, value, isTemp, isSeq, retPath);
            if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
            {
                printf("\n create parent path:%s failed! \n", parentPath_.c_str());
                return false;
            }

            if (ec == ZkUtil::kZKExisted)
            {
                zkClient_->set(parentPath_, value);
            }

            //ע�� watcher
            if (zkClient_->regChildWatcher(parentPath_,
                boost::bind(&DistriBarrier::regChildWatcher_cb, this, _1, _2, _3, _4, _5), NULL) == false)
            {
                printf("\n regChildWatcher failed! path:%s\n", parentPath_.c_str());
                return false;
            }
            isInit_ = true;
        }

        return true;
    }

    //����false��˵����������
    bool wait()
    {
        if (isInit_ == false)
            return false;

        while(1)
        {
            //��ȡ�����ӽ��
            std::vector<std::string> childNodes;
            if (zkClient_->getChildren(parentPath_, childNodes) != ZkUtil::kZKSucceed)
            {
                printf("\n getChildren failed! path:%s \n", parentPath_.c_str());
                return false;
            }

            if (childNodes.size() >= barrNum_)
            {
                return true;
            }
            else  //����ӽ��������㣬��Ҫ�ȴ�
            {
                //û��������, �ȴ�
                mutex_.lock();
                condition_.wait();
                mutex_.unlock();
            }
        }
    }

    //����false��˵����������
    bool countdown()
    {
        //�����ӽ��
        std::string retPath;
        ZkUtil::ZkErrorCode ec = createChild(retPath);
        if (ec == ZkUtil::kZKError)
        {
             return false;
        }
        childPaths_.push(retPath);
        return true;
    }

public:
    ~DistriBarrier()
    {
        //ɾ�������ġ������ӽ��
        std::string childPath;
        while(childPaths_.empty() != true)
        {
            childPath = childPaths_.front();
            childPaths_.pop();
            ZkUtil::ZkErrorCode ec = zkClient_->deleteRecursive(childPath);
        }

        //�ͷ�zookeeper handle
        ZkClientManager::instance().destroyClient(zkClient_->getHandle());
        zkClient_.reset();
        isInit_ = false;
        barrNum_ = 0;
    };

private:

    ZkUtil::ZkErrorCode createChild(std::string& retPath)
    {
        //���� ��·��
        bool isTemp = true;  //��ʱ���
        bool isSeq = true;
        std::string childPath = parentPath_ + "/" + childNodeName_;
        return zkClient_->create(childPath, "", isTemp, isSeq, retPath);
    }

    void regChildWatcher_cb(ZkUtil::ZkNotifyType type, const ZkClientPtr& client,
        const std::string& path, const std::vector<std::string>& childNodes, void* context)
    {
        assert(path == parentPath_);
        if (childNodes.size() >= barrNum_)
        {
            mutex_.lock();
            condition_.notifyAll();
            mutex_.unlock();
        }
    }

private:
    std::string zkConnStr_;
    ZkClientPtr zkClient_;
    volatile bool isInit_;
    const std::string parentPath_ = "/queue_barrier";
    const std::string childNodeName_ = "barrier";
    queue<std::string> childPaths_;
    int barrNum_;
    mutable MutexLock mutex_;
    Condition condition_;
};


void threadFun(DistriBarrier *bar)
{
    sleep(30);
    for (int i = 0; i < 10; i++)
    {
        sleep(1);

        if(bar)
        {
            printf("[threadFun] countdown i = %d\n", i);
            bar->countdown();
        }
    }
}

int main()
{
    DistriBarrier bar;
    if (bar.init(ZOOKEEPER_SERVER_CONN_STRING, 10) == false)
    {
        printf("DistriBarrier failed! \n");
        return 0;
    }

    muduo::Thread* pThreadHandle = new muduo::Thread(boost::bind(&threadFun, &bar), "test_thread");
    if (pThreadHandle != NULL)
    {
        pThreadHandle->start();
    }

    printf("bar wait in mainthread\n");

    bar.wait();

    printf("through bar in mainthread\n");

    sleep(UINT_MAX);
    return  0;

}




