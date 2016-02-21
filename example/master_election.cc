/*
 * master_election.cc
 *
 *      Description: ����zookeeperʵ��masterѡ��
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
#include "muduo/base/Thread.h"
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

using namespace std;
using namespace ZkCppClient;

#define     ZOOKEEPER_SERVER_CONN_STRING        "127.0.0.1:2181,127.0.0.1:2182,127.0.0.1:2183"



class MasterElection : boost::noncopyable
{
public:
    typedef boost::function<void()> RunFunctor;

    //�������ӿ�
    static MasterElection& instance() {return muduo::Singleton<MasterElection>::instance();};

    //ע��masterFun �����Ƿ������ģ�
    bool init(const std::string& zkConnStr, RunFunctor masterFun)
    {
        if (isInit_ == false)
        {
            zkConnStr_ = zkConnStr;
            masterFun_ = masterFun;
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
            isInit_ = true;
        }

        return true;
    }

    bool start()
    {
        if (isInit_ == false)
            return false;

        //ע�� watcher
        if (zkClient_->regChildWatcher(parentPath_,
            boost::bind(&MasterElection::regChildWatcher_cb, this, _1, _2, _3, _4, _5), NULL) == false)
        {
            printf("\n regChildWatcher failed! path:%s\n", parentPath_.c_str());
            return false;
        }

        return retry();
    }

    bool isMaster() {return isInit_ == true && isMaster_ == true;};

public:
    MasterElection()
    {
        zkConnStr_ = "";
        isInit_ = false;
        isMaster_ = false;
    };

    ~MasterElection()
    {
        //ɾ�����
        ZkUtil::ZkErrorCode ec = zkClient_->deleteRecursive(childPath_);
        //�ͷ�zookeeper handle
        ZkClientManager::instance().destroyClient(zkClient_->getHandle());
        zkClient_.reset();
        isInit_ = false;
        isMaster_ = false;
    };

private:

    bool retry()
    {
        //���� ��·��
        bool isTemp = true;  //��ʱ���
        bool isSeq = false;
        std::string retPath;
        ZkUtil::ZkErrorCode ec = zkClient_->create(childPath_, "", isTemp, isSeq, retPath);
        if (ec == ZkUtil::kZKSucceed)     //�����ɹ���˵�� ������masterShip
        {
            isMaster_ = true;
            if (masterFun_)
                masterFun_();  //ע��masterFun �����Ƿ������ģ�
        }
        else if (ec == ZkUtil::kZKExisted)
        {
            isMaster_ = false;
        }
        else if (ec != ZkUtil::kZKExisted)
        {
            printf("create childPath failed! errCode:%d\n", ec);
            return false;
        }
        return true;
    }

    void regChildWatcher_cb(ZkUtil::ZkNotifyType type, const ZkClientPtr& client,
        const std::string& path, const std::vector<std::string>& childNodes, void* context)
    {
        assert(path == parentPath_);
        //�����û�� "binding" �ӽ�㣬˵�� mastership�Ѿ��ͷ��ˣ������¾���mastership
        if (std::find(childNodes.begin(), childNodes.end(), "binding") == childNodes.end())
        {
            retry();
        }
    }

private:
    std::string zkConnStr_;
    ZkClientPtr zkClient_;
    volatile bool isInit_;
    volatile bool isMaster_;
    RunFunctor masterFun_;
    const std::string parentPath_ = "/master_election";
    const std::string childPath_ = "/master_election/binding";
};


void masterFunCb()
{
    while(1)
    {
        printf("I'm master. Run in masterFunCb.\n");
        sleep(2);
    }
}

void masterFun()
{
    muduo::Thread* pThreadHandle = new muduo::Thread(boost::bind(&masterFunCb), "master_fun_thread");
    if (pThreadHandle != NULL)
    {
        pThreadHandle->start();
    }
}

int main()
{
    if (MasterElection::instance().init(ZOOKEEPER_SERVER_CONN_STRING, boost::bind(&masterFun)) == false)
    {
        printf("MasterElection init failed!\n");
        return 0;
    }

    MasterElection::instance().start();

    while(1)
    {
        printf("I'm %s. Run in main.\n", MasterElection::instance().isMaster() ? "Master" : "Slave");
        sleep(2);
    }

    return  0;
}

