/*
 * distribute_lock.cc
 *
 *      Description: ����zookeeperʵ�� �ֲ�ʽ������
 *      Created on: 2016��2��21��
 *      Author: ZengHui Bao (bao_z_h@163.com)
 */


#include "ZkClient.h"
#include "ZkClientManager.h"
#include <stdio.h>
#include <string>
#include <vector>
#include <assert.h>
#include <algorithm>
#include <unistd.h>
#include "muduo/base/Condition.h"
#include "muduo/base/Mutex.h"
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

using namespace std;
using namespace ZkCppClient;

#define     ZOOKEEPER_SERVER_CONN_STRING        "127.0.0.1:2181,127.0.0.1:2182,127.0.0.1:2183"

class DistriLock : boost::noncopyable
{
public:
    //�������ӿ�
    DistriLock() :
            mutex_(),
            condition_(mutex_)
    {
        zkConnStr_ = "";
        isInit_ = false;
        childNodeName_ = "";
    };

    bool init(const std::string& zkConnStr, const std::string& childNodeName)
    {
        if (isInit_ == false)
        {
            childNodeName_ = childNodeName;
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
            ZkUtil::ZkErrorCode ec = zkClient_->create(parentPath_, "", isTemp, isSeq, retPath);
            if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
            {
                printf("\n create parent path:%s failed! \n", parentPath_.c_str());
                return false;
            }

            //ע�� watcher
            if (zkClient_->regChildWatcher(parentPath_,
                boost::bind(&DistriLock::regChildWatcher_cb, this, _1, _2, _3, _4, _5), NULL) == false)
            {
                printf("\n regChildWatcher failed! path:%s\n", parentPath_.c_str());
                return false;
            }
            isInit_ = true;
        }

        return true;
    }

    //����false��˵����������
    bool lock()
    {
        if (isInit_ == false)
            return false;

        ZkUtil::ZkErrorCode ec;
        while(1)
        {
            ec = createChild();
            if (ec == ZkUtil::kZKExisted)
            {
                //û��������, �ȴ�
                mutex_.lock();
                condition_.wait();
                mutex_.unlock();
            }
            else
            {
                break;
            }
        }

        if (ec == ZkUtil::kZKSucceed)     //�����ɹ���˵�� ��������
        {
            return true;
        }
        else  //���������У����ִ���
        {
            printf("create childPath failed! errCode:%d\n", ec);
            return false;
        }
    }

    //����false��˵����������
    bool unlock()
    {
        //ɾ���ӽ��
        std::string childPath = parentPath_ + "/" + childNodeName_;
        ZkUtil::ZkErrorCode ec = zkClient_->deleteRecursive(childPath);
        if (ec == ZkUtil::kZKError)
        {
             return false;
        }
        return true;
    }

public:
    ~DistriLock()
    {
        //ɾ�����
        std::string childPath = parentPath_ + "/" + childNodeName_;
        ZkUtil::ZkErrorCode ec = zkClient_->deleteRecursive(childPath);
        //�ͷ�zookeeper handle
        ZkClientManager::instance().destroyClient(zkClient_->getHandle());
        zkClient_.reset();
        isInit_ = false;
    };

private:

    ZkUtil::ZkErrorCode createChild()
    {
        //���� ��·��
        bool isTemp = true;  //��ʱ���
        bool isSeq = false;
        std::string retPath;
        std::string childPath = parentPath_ + "/" + childNodeName_;
        return zkClient_->create(childPath, "", isTemp, isSeq, retPath);
    }

    void regChildWatcher_cb(ZkUtil::ZkNotifyType type, const ZkClientPtr& client,
        const std::string& path, const std::vector<std::string>& childNodes, void* context)
    {
        assert(path == parentPath_);
        //�����û�� childNodeName_ �ӽ�㣬˵�� ���Ѿ��ͷ��ˣ������¾���mastership
        if (std::find(childNodes.begin(), childNodes.end(), childNodeName_) == childNodes.end())
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
    const std::string parentPath_ = "/exclusive_lock";
    std::string childNodeName_;
    mutable MutexLock mutex_;
    Condition condition_;
};



int main()
{
    DistriLock distriLock;
    if (distriLock.init(ZOOKEEPER_SERVER_CONN_STRING, "lock") == false)
    {
        printf("init lock failed! \n");
        return 0;
    }

    distriLock.lock();
    int count = 20;
    while (count > 0)
    {
        printf ("get lock! count:%d\n", count);
        sleep(2);
        count--;
    }
    distriLock.unlock();
    printf("release lock\n");

    sleep(UINT_MAX);
    return  0;
}


