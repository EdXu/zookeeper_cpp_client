/*
 * ZkClient.h
 *
 *      Created on: 2016��2��20��
 *      Author: ZengHui Bao (bao_z_h@163.com)
 */

#ifndef __ZK_CLIENT_H
#define __ZK_CLIENT_H


#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <string>
#include <poll.h>
#include <sys/epoll.h>
#include <map>
#include <vector>
#include <algorithm>
#include "muduo/base/Condition.h"
#include "muduo/base/Mutex.h"
#include "muduo/base/Logging.h"
#include "muduo/net/SocketsOps.h"
#include "ZkTimerQueue.h"
#include "zookeeper.h"
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>



using namespace muduo::net;
using namespace muduo;

namespace ZkCppClient
{

class ZkNetClient;
class ZkClient;
typedef boost::shared_ptr<ZkClient> ZkClientPtr;


namespace ZkUtil
{
    //common
	const int kThisEpollTimeMs = 10000;
	const int kNoneEvent = 0;
	const int kReadEvent = POLLIN | POLLPRI;
	const int kWriteEvent = POLLOUT;

	const int kMaxRetryDelay = 10*60;   //��λ: ��
	const int kInitRetryDelay = 5;      //��λ: ��

	const char* strerror_tl(int savedErrno);

	int setNonBlock(int fd, bool value);

	bool isReadEvent(int events);
	bool isWriteEvent(int events);

	void modifyEpollEvent(int operation, int epollfd, ZkNetClient* pClient, std::string printStr);

	void addEpollFd(int epollfd, ZkNetClient* pClient);

	void modEpollFd(int epollfd, ZkNetClient* pClient);

	void delEpollFd(int epollfd, ZkNetClient* pClient);

    void enableReading(ZkNetClient* pClient);
    void enableWriting(ZkNetClient* pClient);
    void disableWriting(ZkNetClient* pClient);
    void disableAll(ZkNetClient* pClient);

    int getSocketError(int sockfd);

    int createEventfd();


    //zookeeper client related
    const int32_t kInvalidDataVersion = -1;
    const int kMaxNodeValueLength = 32 * 1024;
    const int kMaxPathLength = 512;

    typedef boost::function<void (const ZkClientPtr& client, void* context)> SessionExpiredHandler;

    //�����Ļص�ԭ��///////////////////////////////////////////////////////////////////////////////////////////
    enum ZkErrorCode
    {
        kZKSucceed= 0, // �����ɹ�,���� ������
        kZKNotExist,  // �ڵ㲻����, ���� ��֧��㲻����
        kZKError,     // ����ʧ��
        kZKDeleted,   // �ڵ�ɾ��
        kZKExisted,   // �ڵ��Ѵ���
        kZKNotEmpty,   // �ڵ㺬���ӽڵ�
        kZKLostConnection   //��zookeeper server�Ͽ�����
    };
    /* errcode ���أ�
        kZKSucceed: ��ȡ�ɹ�, value ����ֵ��version ���İ汾��(֮��ɸ������ֵ����delete,setʱ������CAS����)
        kZKNotExist: ��㲻����, value Ϊ�մ���version ΪkInvalidDataVersion
        kZKError: ��������, value Ϊ�մ���version ΪkInvalidDataVersion
    */
    typedef boost::function<void (ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path,
                                  const std::string& value, int32_t version, void* context)> GetNodeHandler;

    /* errcode ���أ�
        kZKSucceed: ��ȡ�ɹ�, childNode���� �����ӽ��� �������path ���� ��֧·��
        kZKNotExist: ��㲻����, childNode Ϊ��
        kZKError: ��������, childNode Ϊ��
    */
    typedef boost::function<void (ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path,
                                  const std::vector<std::string>& childNodes, void* context)> GetChildrenHandler;

    /* errcode ���أ�
        kZKSucceed: ������
        kZKNotExist: ��㲻����
        kZKError: ��������
    */
    typedef boost::function<void (ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client,
                                  const std::string& path, void* context)> ExistHandler;

    /* errcode ���أ�
        kZKSucceed: �����ɹ�, value ����ֵ
        kZKNotExist: ��·�������ڣ�����ʧ�� value �մ�������Ҫ�ȴ�����·�����ٴ������
        kZKExisted: ����Ѵ��ڣ�����ʧ�� value �մ���
        kZKError: �������󣨴���ʧ�� value �մ���
		ע������� path ������ ������Ľ���������� ָ�������Ľ������
			����������� ˳���� �ڵ㣬�򷵻ص�·�� path �� ��ʵ�����Ľ���� ���в�ͬ(��ʱgetChildren����ȡ����ʵ�Ľ����)��
			������� ���ص�·�� path �� ��ʵ�����Ľ���� ����ͬ.
    */
    typedef boost::function<void (ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path,
                                  const std::string& value, void* context)> CreateHandler;
    /* errcode ���أ�
        kZKSucceed: set�ɹ�, version �������޸Ľ��İ汾��
        kZKNotExist: ��㲻����, version ΪkInvalidDataVersion
        kZKError: ��������, version ΪkInvalidDataVersion
    */
    typedef boost::function<void (ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path,
                                  int32_t version, void* context)> SetHandler;

    /* errcode ���أ�
        kZKSucceed: ɾ���ɹ�
        kZKNotExist: Ҫɾ���Ľڵ� ������
        kZKNotEmpty: Ҫɾ���Ľڵ� ���� �ӽڵ㣬��Ҫ��ɾ���ӽڵ�.
        kZKError: ��������
    */
    typedef boost::function<void (ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client,
                                  const std::string& path, void* context)> DeleteHandler;

    //Watcher�Ļص�ԭ��///////////////////////////////////////////////////////////////////////////////////////////
    enum ZkNotifyType
    {
        kNodeDelete = 0,    // �ڵ�ɾ��
        kNodeCreate,    // �ڵ㴴��
        kNodeChange,    // �ڵ�����ݱ��
        kGetNodeValueFailed_NodeNotExist, //�ڵ㴴�� �� ���ݱ��ʱ������zookeeper server��ȡ��������ʱ ����ѱ�ɾ��
        kGetNodeValueFailed,  //�ڵ㴴�� �� ���ݱ��ʱ������zookeeper server��ȡ��������ʱ ʧ��
        kChildChange,    // �ӽڵ�ı�������ӡ�ɾ���ӽڵ㣩
        kGetChildListFailed_ParentNotExist, //�ӽڵ�ı��ʱ������zookeeper server��ȡ�����ӽڵ��б�ʱ ������ѱ�ɾ��
        kGetChildListFailed,  //�ӽڵ�ı��ʱ������zookeeper server��ȡ�����ӽڵ��б�ʱ ʧ��
        kTypeError, //��������
    };
    /* type ���أ�
        kNodeDelete = 0,    // path: ע�������·�� value: �մ� version: kInvalidDataVersion
        kNodeCreate,    // path: ע�������·�� value: ������µ�ֵ version: ������µİ汾��
        kNodeChange,    // path: ע�������·�� value: ������µ�ֵ version: ������µİ汾��
        kGetNodeValue_NodeNotExist, //path: ע�������·�� value: �մ� version: kInvalidDataVersion
        kGetNodeValueFailed,  //path: ע�������·�� value: �մ� version: kInvalidDataVersion
        kTypeError, // path ע�������·�� value �մ� version kInvalidDataVersion
    */
    typedef boost::function<void (ZkUtil::ZkNotifyType type, const ZkClientPtr& client,
                                  const std::string& path, const std::string& value,
                                  int32_t version, void* context)> NodeChangeHandler;
    /* type ���أ�
        kChildChange,    // path: ע�������·��  childNodes: ���µ��ӽ���б�(ע:����������·���������ӽ��Ľ����)
        kGetChildListFailed_ParentNotExist�� // path: ע�������·��  childNodes: �ռ���
        kGetChildListFailed,  //path: ע�������·��  childNodes: �ռ���
        kTypeError, //path: ע�������·��  childNodes: �ռ���
    */
    typedef boost::function<void (ZkUtil::ZkNotifyType type, const ZkClientPtr& client,
                                  const std::string& path, const std::vector<std::string>& childNodes,
                                  void* context)> ChildChangeHandler;
};





//���лص���������Watcher��������һ�����ݽṹ��Ϊ������
//ͨ���Ǵ� Zookeeper�յ��ص���context* ��������ݽṹ���󣬴�ZKWatchContext��ȡ��Ҫ�õ��Ĳ������� �ص� �û�����ĺ���
struct ZkOperateAndWatchContext
{
	ZkOperateAndWatchContext(const std::string& path, void* context, ZkClientPtr zkclient);

	void* context_;
	std::string path_;
	ZkClientPtr zkclient_;

	ZkUtil::GetNodeHandler getnode_handler_;
	ZkUtil::GetChildrenHandler getchildren_handler_;
	ZkUtil::ExistHandler exist_handler_;
	ZkUtil::CreateHandler create_handler_;
	ZkUtil::SetHandler set_handler_;
	ZkUtil::DeleteHandler delete_handler_;
	ZkUtil::NodeChangeHandler node_notify_handler_;
	ZkUtil::ChildChangeHandler child_notify_handler_;
};

struct ContextInNodeWatcher
{
    ContextInNodeWatcher(const std::string&path,  ZkClientPtr zkclient, ZkUtil::NodeChangeHandler handler,
                         ZkUtil::ZkNotifyType type, void* context)
    {
        path_ = path;
        zkclient_ = zkclient;
        node_notify_handler_ = handler;
        notifyType_ = type;
        contextInOrignalWatcher_ = context;
    }

    std::string path_;
    ZkClientPtr zkclient_;
    ZkUtil::NodeChangeHandler node_notify_handler_;
    ZkUtil::ZkNotifyType notifyType_;
    void* contextInOrignalWatcher_;
};

struct ContextInChildWatcher
{
    ContextInChildWatcher(const std::string&path, ZkClientPtr zkclient, ZkUtil::ChildChangeHandler handler,
                         ZkUtil::ZkNotifyType type, void* context)
    {
        path_ = path;
        zkclient_ = zkclient;
        child_notify_handler = handler;
        notifyType_ = type;
        contextInOrignalWatcher_ = context;
    }

    std::string path_;
    ZkClientPtr zkclient_;
    ZkUtil::ChildChangeHandler child_notify_handler;
    ZkUtil::ZkNotifyType notifyType_;
    void* contextInOrignalWatcher_;
};

struct ContextInCreateParentAndNodes
{
    ContextInCreateParentAndNodes(const std::string& path, const std::string& value,
                                  ZkUtil::CreateHandler handler,void* context,
                                  bool isTemp, bool isSeq, ZkClientPtr zkclient)
    {
        path_ = path;
        value_ = value;
        create_handler_ = handler;
        context_ = context;
        isTemp_ = isTemp;
        isSequence_ = isSeq;
        zkclient_ = zkclient;
    }

    std::string path_;
    std::string value_;
    ZkUtil::CreateHandler create_handler_;
    void* context_;
    bool isTemp_;
    bool isSequence_;
    ZkClientPtr zkclient_;
};

struct ContextInDeleteRecursive
{
    ContextInDeleteRecursive(const std::string& path, ZkUtil::DeleteHandler handler, void* context,
                             int32_t version, ZkClientPtr zkclient)
    {
        path_ = path;
        delete_handler_ = handler;
        context_ = context;
        version_ = version;
        zkclient_ = zkclient;
    }

    std::string path_;
    ZkUtil::DeleteHandler delete_handler_;
    void* context_;
    int32_t version_;
    ZkClientPtr zkclient_;
};

struct ZkZooInitCbData
{
	ZkZooInitCbData(uint32_t handle)
	{
		handle_ = handle;
	}
	uint32_t handle_;
};






struct SessionClientId
{
    int64_t client_id;
    char passwd[16];

    SessionClientId()
    {
        memset(this, 0, sizeof(SessionClientId));
    }
};


struct ZkConnChannel : boost::noncopyable
{
public:
	ZkConnChannel(int epollfd, int fd, int events)
	{
		memset(this, 0, sizeof *this);
		fd_ = fd;
		events_ = events;
		epollfd_ = epollfd;
	}

	void update(ZkNetClient *client) {ZkUtil::modEpollFd(epollfd_, client);};

	~ZkConnChannel() 
	{
		LOG_DEBUG << "[~ZkConnChannel] deleting fd:" << fd_;
		close(fd_);
	}

public:
	int fd_;
	int epollfd_;
	int events_;
};


class ZkNetClient  : boost::noncopyable
{
public:
	friend class ZkClientManager;
	friend class ZkTimerQueue;
	typedef boost::function<void()> ReadTimerCallback;

public:
	ZkNetClient(int epollfd, int threadId, int eventfd, std::string netName)
	{
#define  ZKCLIENT_LOOP_INDEX_INIT    0xFFFFFFFFFFFFFFFE
#define  FUNCTION_LOOP_INDEX_INIT    0xFFFFFFFFFFFFFFFF

		pConnChannel_ = new ZkConnChannel(epollfd, eventfd, ZkUtil::kNoneEvent);
		epollfd_ = epollfd;
		threadId_ = threadId;
		loopIndex_ = ZKCLIENT_LOOP_INDEX_INIT;
		loopIndexFunResetChannel_ = FUNCTION_LOOP_INDEX_INIT;
		loopIndexFunRetry_ = FUNCTION_LOOP_INDEX_INIT;
		netName_ = netName;
	}

	~ZkNetClient()
	{
		if (pConnChannel_)
		{
			delete pConnChannel_;
			pConnChannel_ = NULL;
		}
	}

	std::string getNetName() {return netName_;};
	ZkConnChannel* getChannel() {return pConnChannel_;};

	void handleRead();
	void handleEventFdRead(int eventfd);
	void handleTimerFdRead();
	void handleWrite();
	void setReadTimerCb(ReadTimerCallback cb);

private:
	ZkConnChannel *pConnChannel_;
	ReadTimerCallback timerReadCb_;

	std::string netName_;
	int threadId_;   //һ���̣߳�thread_id�������ж��sslClient����һ��sslclientֻ������һ���߳�
	int epollfd_;    //һ���߳�ʹ��һ��epollfd

	volatile uint64_t loopIndex_;
	volatile uint64_t loopIndexFunResetChannel_;
	volatile uint64_t loopIndexFunRetry_;
};



//����һ��session��״̬�����ṩ���������ӿڵ�ʵ��
//���ûص���ʱ��Ҫע�⣺�߳�����ԭ��Zookeeper C����߳������еģ����̰߳�ȫ��
class ZkClient : boost::noncopyable,
				 public boost::enable_shared_from_this<ZkClient>
{
public:
        struct NodeWatchData
        {
                NodeWatchData();
                NodeWatchData(const NodeWatchData& data);
                NodeWatchData& operator= (const NodeWatchData& data);

                std::string path_;
                ZkUtil::NodeChangeHandler handler_;
                void *context_;
                std::string value_;
                int32_t version_;
                bool isSupportAutoReg_;   //��watcher�������Ƿ�֧���Զ���ע��watcher
        };

        struct ChildWatchData
        {
                ChildWatchData();
                ChildWatchData(const ChildWatchData& data);
                ChildWatchData& operator= (const ChildWatchData& data);

                std::string path_;
                ZkUtil::ChildChangeHandler handler_;
                void *context_;
                std::vector<std::string> childList_;
                bool isSupportAutoReg_;   //��watcher�������Ƿ�֧���Զ���ע��watcher
        };

public:
        friend class ZkClientManager;

        ZkClient(uint32_t handle);
        bool init(const std::string& host, int timeout, SessionClientId *clientId = NULL,
              ZkUtil::SessionExpiredHandler expired_handler = NULL, void* context = NULL);

        ~ZkClient();

        muduo::MutexLock& getStateMutex() {return stateMutex_;};
        muduo::Condition& getStateCondition() {return stateCondition_;};

        int getSessStat();
        void setSessStat(int stat);

        int getSessTimeout();
        void setSessTimeout(int time);

        int64_t getSessDisconn();
        void setSessDisconn(int64_t disconn);

        void setNodeWatchData(const std::string& path, const NodeWatchData& data);
        bool getNodeWatchData(const std::string& path, NodeWatchData& retNodeWatchData);
        bool isShouldNotifyNodeWatch(const std::string& path);
        void getNodeWatchPaths(std::vector<std::string>& data);

        void getChildWatchPaths(std::vector<std::string>& data);
        void setChildWatchData(const std::string& path, const ChildWatchData& data);
        bool getChildWatchData(const std::string& path, ChildWatchData& retChildWatchData);
        bool isShouldNotifyChildWatch(const std::string& path);

		int getRetryDelay() {return retryDelay_;};
		void setRetryDelay(int delay) {retryDelay_ = delay;};

		bool isRetrying() {return isRetrying_;};
		void setIsRetrying(bool retrying) {isRetrying_ = retrying;};

		bool hasCallTimeoutFun() {return hasCallTimeoutFun_;};
		void setHasCallTimeoutFun(bool isCall) {hasCallTimeoutFun_ = isCall;};

		ZkUtil::SessionExpiredHandler& getExpireHandler() {return expiredHandler_;};
		void* getContext() {return userContext_;};

		void autoRegNodeWatcher(std::string path);
		void autoRegChildWatcher(std::string path);

		bool isInit() {return isInitialized_;};
		void setIsInit(bool isInited){isInitialized_ = isInited;};

public:
        //�������ӿ�
        uint32_t getHandle() {return handle_;};

        bool isSupportReconnect() {return isSupportReconnect_;};
        void setIsSupportReconnect(bool isReconn) {isSupportReconnect_ = isReconn;};

        bool isConnected() {return getSessStat() == ZOO_CONNECTED_STATE;};

        bool getClientId(SessionClientId& cliId);

        /* async operation api */
        // handle�� ��Ϊ��.
        // ����false������ʧ�ܣ�����true���п��ܳɹ���Ҫ���ݻص�handler���ص�rc����ȷ���Ƿ�ɹ���.
        bool getNode(const std::string& path, ZkUtil::GetNodeHandler handler, void* context);
        bool getChildren(const std::string& path, ZkUtil::GetChildrenHandler handler, void* context);
		//����: kZKSucceed, ������: kZKNotExist ��������kZKError
        bool isExist(const std::string& path, ZkUtil::ExistHandler handler, void* context);

        //�����������ͣ�Ĭ�ϳ־��ͷ�˳���ͣ�isTemp ��ʱ�ͣ�isSequence ˳���ͣ�
        //����Ȩ��acl: Ĭ���Ƕ��ɷ���
        bool create(const std::string& path, const std::string& value,
                ZkUtil::CreateHandler handler, void* context, bool isTemp = false, bool isSequence = false);
        bool createIfNeedCreateParents(const std::string& path, const std::string& value,
                ZkUtil::CreateHandler handler, void* context, bool isTemp = false, bool isSequence = false);

        //�������version����ָ���汾�Ľ��set���� ����CAS����������Ĭ�������ý������°汾��ֵ(version: -1)
        bool set(const std::string& path, const std::string& value, ZkUtil::SetHandler handler,
             void* context, int32_t version = -1);

        bool deleteNode(const std::string& path, ZkUtil::DeleteHandler handler, void* context, int32_t version = -1);
        bool deleteRecursive(const std::string& path, ZkUtil::DeleteHandler handler, void* context, int32_t version = -1);

        /* sync operation api */
        ZkUtil::ZkErrorCode getNode(const std::string& path, std::string& value, int32_t& version);
        ZkUtil::ZkErrorCode getChildren(const std::string& path, std::vector<std::string>& childNodes);

		//����: kZKSucceed, ������: kZKNotExist ��������kZKError
        ZkUtil::ZkErrorCode isExist(const std::string& path);
        //����������� ˳���� �ڵ㣬�򷵻ص�·��retPath �� ԭ·��path ���в�ͬ��������� retPath �� path����ͬ.
        ZkUtil::ZkErrorCode create(const std::string& path, const std::string& value,
                               bool isTemp /*= false*/, bool isSequence /*= false*/, std::string& retPath);
        //����ʱ�����·���ķ�֧��㲻���ڣ�����ȴ�����֧��㣬�ٴ���Ҷ�ӽ�㡣��ע����֧�������� �־��͵ģ�
        ZkUtil::ZkErrorCode createIfNeedCreateParents(const std::string& path, const std::string& value,
                               bool isTemp /*= false*/, bool isSequence /*= false*/, std::string& retPath);
        ZkUtil::ZkErrorCode set(const std::string& path, const std::string& value, int32_t version = -1);
        ZkUtil::ZkErrorCode deleteNode(const std::string& path, int32_t version = -1);
        ZkUtil::ZkErrorCode deleteRecursive(const std::string& path, int32_t version = -1);

        /* register watcher */
        //Ĭ��������ʽapi, �Ҵ���watcher�󣬻��Զ���ע��watcher.
		//ע�������¼�(�ڵ�ɾ�����ڵ㴴�����ڵ����ݱ��)��watcher.
		//ע���� path��� ������ʱ��Ҳ����ע��ɹ�.
        bool regNodeWatcher(const std::string& path, ZkUtil::NodeChangeHandler handler, void* context);
		//ע�� �ӽڵ�ı�������ӡ�ɾ���ӽڵ㣩�¼���watcher.
		//ע���� path��� ������ʱ����ע��ʧ�ܣ�����ע��ǰ�����ȴ��� path ���.
        bool regChildWatcher(const std::string& path, ZkUtil::ChildChangeHandler handler, void* context);

        //ȡ�� ��path��watcher.
        void cancelRegNodeWatcher(const std::string& path);
        void cancelRegChildWatcher(const std::string& path);

private:
        void setHandle(uint32_t handle) {handle_ = handle;};

        static void defaultSessionExpiredHandler(const ZkClientPtr& client, void* context);
        static void sessionWatcher(zhandle_t *zh, int type, int state, const char *path, void *watcher_ctx);
        int64_t getCurrentMs();

        static void checkSessionState(uint32_t handle);

        //�����ص�
        static void getNodeDataCompletion(int rc, const char* value, int value_len,
                                         const struct Stat* stat, const void* data);
        static void getChildrenStringCompletion(int rc, const struct String_vector* strings, const void* data);
        static void existCompletion(int rc, const struct Stat* stat, const void* data);
        static void createCompletion(int rc, const char* value, const void* data);
        static void setCompletion(int rc, const struct Stat* stat, const void* data);
        static void deleteCompletion(int rc, const void* data);
        static void existWatcher(zhandle_t* zh, int type, int state, const char* path, void* watcher_ctx);
        static void getNodeDataOnWatcher(int rc, const char* value, int value_len,
                                               const struct Stat* stat, const void* data);
        static void getChildrenWatcher(zhandle_t* zh, int type, int state,
                                                    const char* path,void* watcher_ctx);
        static void getChildDataOnWatcher(int rc, const struct String_vector* strings, const void* data);
        static void createIfNeedCreateParentsCompletion(int rc, const char* value, const void* data);
        static void deleteRecursiveCompletion(int rc, const void* data);

        bool reconnect();
		static void retry(uint32_t handle);
        static std::string getSessStatStr(int stat);
        void printClientInfo();
        ZkUtil::ZkErrorCode createPersistentDirNode(const std::string& path);
        bool createPersistentDir(const std::string& path);
        void postCreateParentAndNode(const ContextInCreateParentAndNodes* watch_ctx);
        void postDeleteRecursive(const ContextInDeleteRecursive* watch_ctx);
        static void regAllWatcher(uint32_t handle);

private:
        uint32_t handle_;    //��clientManager�е�handle

		volatile bool isInitialized_;
        volatile bool isSupportReconnect_;   //�Ƿ�֧������
		volatile int retryDelay_;
		volatile bool isRetrying_;
		volatile bool hasCallTimeoutFun_;

        std::string host_;
        SessionClientId *clientId_;
        zhandle_t* zhandle_;
        //	FILE* log_fp_;
		ZkUtil::SessionExpiredHandler expiredHandler_;
		void* userContext_;

        // ZK�Ự״̬
        volatile int sessionState_;
        muduo::MutexLock stateMutex_;  //���stateCondition_
        muduo::Condition stateCondition_;
        muduo::MutexLock sessStateMutex_;   //������ session_state_�Ļ������

        volatile int sessionTimeout_;    //session���ֵĳ�ʱʱ��
        muduo::MutexLock sessTimeoutMutex_;   //������ session_timeout_�Ļ������

        volatile int64_t sessionDisconnectMs_;   //session�����쳣��ʱ���
        muduo::MutexLock sessDisconnMutex_;   //������ session_disconnect_ms_�Ļ������

        std::map<std::string, NodeWatchData> nodeWatchDatas_;   //map<path, watchdata>
        muduo::MutexLock nodeWatchMutex_;
        std::map<std::string, ChildWatchData> childWatchDatas_; //map<path, watchdata>
        muduo::MutexLock childWatchMutex_;
};

}

#endif
