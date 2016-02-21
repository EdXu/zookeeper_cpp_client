/*
 * test.cc
 *
 *      Description: ����ZkClient, ZkClientManager�ĸ����ӿ�
 *      Created on: 2016��2��20��
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
#include <muduo/base/CountDownLatch.h>



using namespace std;
using namespace ZkCppClient;

#define     ZOOKEEPER_SERVER_CONN_STRING        "127.0.0.1:2181,127.0.0.1:2182,127.0.0.1:2183"

uint32_t gZkClientHandle;


void printChild(const std::string& path, const std::vector<std::string>& childnodes)
{
	printf("\n\n-----------[printChild] ParentPath: %s, child size:%d-------------- \n", path.c_str(), childnodes.size());
    std::vector<std::string>::const_iterator iter = childnodes.begin();
    for(;iter != childnodes.end(); iter++)
    {
		printf("child name:%s\n", (*iter).c_str());
    }
	printf("-------------------------------------------------------------------- \n");
}

bool isEqualChildren(const std::vector<std::string>& left, const std::vector<std::string>& right)
{
	if (left.size() != right.size())
	{
		return false;
	}

	std::vector<std::string>::const_iterator iter = left.begin();
	for (; iter != left.end(); iter++)
	{
		if (find(right.begin(), right.end(), (*iter)) == right.end())
		{
			printf("[isEqualChildren] left vec elem: %s don't exist in right vec.\n", (*iter).c_str());
			return false;
		}
	}

	return true;
}

void printClientId(const SessionClientId& cliId)
{
	printf("\ncliendId:\n");
	printf("cliId.id:%d\n", cliId.client_id);
	printf("cliId.passwd:%s\n", cliId.passwd);
}

std::string getNotifyTypeStr(ZkUtil::ZkNotifyType type)
{
	if (ZkUtil::kNodeDelete == type)
	{
		return "ZkUtil::kNodeDelete";
	}
	else if (ZkUtil::kNodeCreate == type)
	{
		return "ZkUtil::kNodeCreate";
	}
	else if (ZkUtil::kNodeChange == type)
	{
		return "ZkUtil::kNodeChange";
	}
	else if (ZkUtil::kGetNodeValueFailed == type)
	{
		return "ZkUtil::kGetNodeValueFailed";
	}
	else if (ZkUtil::kChildChange == type)
	{
		return "ZkUtil::kChildChange";
	}
	else if (ZkUtil::kGetChildListFailed == type)
	{
		return "ZkUtil::kGetChildListFailed";
	}
	else if (ZkUtil::kTypeError == type)
	{
		return "ZkUtil::kTypeError";
	}
    else if (ZkUtil::kGetNodeValueFailed_NodeNotExist == type)
    {
        return "ZkUtil::kGetNodeValueFailed_NodeNotExist";
    }
    else if (ZkUtil::kGetChildListFailed_ParentNotExist == type)
    {
        return "ZkUtil::kGetChildListFailed_ParentNotExist";
    }
}








void getHandle_test(uint32_t original_handle, ZkClientPtr cli)
{
    assert(original_handle == cli->getHandle());

    //��һ�������ڵ�handle ��ȡzkclient��Ӧ����NULL
    uint32_t uninit_handle = 0xFFFFFFF0;
    assert(ZkClientManager::instance().getZkClient(uninit_handle).get() == NULL);
	printf("getHandle_test succeed!\n");
}

void getClientId_test()
{
	//����һ��session
	uint handle = ZkClientManager::instance().createZkClient(ZOOKEEPER_SERVER_CONN_STRING, 30000, NULL, NULL, NULL);
	if (handle == 0)
	{
		printf("[getClientId_test] create session failed! connStr:%s\n", ZOOKEEPER_SERVER_CONN_STRING);
		return;
	}

	//��ȡ��ǰsession��clientId
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(handle);
	SessionClientId cliId;
	if (cli->getClientId(cliId) == true)
	{
		printClientId(cliId);
	}

	//����֮ǰsession��clientId �� ����һ��session
	uint32_t handle_new = ZkClientManager::instance().createZkClient(ZOOKEEPER_SERVER_CONN_STRING, 90000, &cliId, NULL, NULL);
	if (handle_new == 0)
	{
		printf("[getClientId_test]create session failed! connStr:%s\n", ZOOKEEPER_SERVER_CONN_STRING);
		return;
	}
	ZkClientPtr cli_new = ZkClientManager::instance().getZkClient(handle_new);
	SessionClientId cliId_new;
	if (cli_new->getClientId(cliId_new) == true)
	{
		printClientId(cliId_new);
		assert(cliId.client_id == cliId_new.client_id);
		assert(strncmp(cliId.passwd, cliId_new.passwd, sizeof(cliId.passwd)) == 0);
	}
	printf("getClientId_test succeed!\n");
}

void sync_getNode_test()
{
#define  SYNC_GETNODE_PATH_NAME     "/sync_getNode_test"
#define  SYNC_GETNODE_PATH_VALUE     "sync_getNode_test_value"

	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec = cli->create(SYNC_GETNODE_PATH_NAME, SYNC_GETNODE_PATH_VALUE, false, false, retPath);
	if (ec == ZkUtil::kZKSucceed || ec == ZkUtil::kZKExisted)
	{
		std::string value = "";
		int32_t version;
		if (cli->getNode(SYNC_GETNODE_PATH_NAME, value, version) == ZkUtil::kZKSucceed)
		{
			printf("\n[sync_getNode_test] path:%s, value:%s, version:%d\n", SYNC_GETNODE_PATH_NAME, value.c_str(), version);
			assert(value == SYNC_GETNODE_PATH_VALUE);
		}
		else
		{
			printf("\n[sync_getNode_test] getNode:%s failed! sync_getNode_test failed!\n", SYNC_GETNODE_PATH_NAME);
			return;
		}
	}
	else
	{
		printf("\n[sync_getNode_test] create path:%s failed! sync_getNode_test failed!\n", SYNC_GETNODE_PATH_NAME);
		return;
	}
	printf("sync_getNode_test succeed!\n");
}

void sync_getChildren_test()
{
#define  SYNC_GETCHILDREN_BASE_NAME     "/sync_getChildren_test_base"
#define  SYNC_GETCHILDREN_CHILD_NAME    "sync_getChildren_test_child"
#define  SYNC_GETCHILDREN_CHILD_NUM      10

	std::vector<std::string> orignalChildList;
	std::vector<std::string> childNodes;

	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec = cli->create(SYNC_GETCHILDREN_BASE_NAME, "", false, false, retPath);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
	{
		printf("\n[sync_getChildren_test] create path:%s failed! \n", SYNC_GETCHILDREN_BASE_NAME);
		goto TAG_SYNC_GETCHILDREN_TEST_FAILED;
	}

	for (int i = 0; i < SYNC_GETCHILDREN_CHILD_NUM; i++)
	{
		char path[100] = {0};
		char childName[100] = {0};
		snprintf(childName, sizeof(childName), "%s_%d", SYNC_GETCHILDREN_CHILD_NAME, i);
		snprintf(path, sizeof(path), "%s/%s_%d", SYNC_GETCHILDREN_BASE_NAME, SYNC_GETCHILDREN_CHILD_NAME, i);
		orignalChildList.push_back(childName);

		ZkUtil::ZkErrorCode ec_chd = cli->create(path, "", false, false, retPath);
		if (ec_chd != ZkUtil::kZKSucceed && ec_chd != ZkUtil::kZKExisted)
		{
			printf("\n[sync_getChildren_test] create path:%s failed! \n", path);
			goto TAG_SYNC_GETCHILDREN_TEST_FAILED;
		}
	}

	if (cli->getChildren(SYNC_GETCHILDREN_BASE_NAME, childNodes) != ZkUtil::kZKSucceed)
	{
		printf("\n[sync_getChildren_test] getChildren failed! path:%s \n", SYNC_GETCHILDREN_BASE_NAME);
		goto TAG_SYNC_GETCHILDREN_TEST_FAILED;
	}

	printChild(SYNC_GETCHILDREN_BASE_NAME, childNodes);
	assert(isEqualChildren(orignalChildList, childNodes) == true);

	printf("sync_getChildren_test succeed!\n");
	return;

TAG_SYNC_GETCHILDREN_TEST_FAILED:
	printf("\n[sync_getChildren_test] failed! \n");
	return;
}

void sync_isExist_test()
{
#define  SYNC_ISEXIST_PATH_NAME     "/sync_isExist_test_base"

	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec_exist;

	ZkUtil::ZkErrorCode ec = cli->create(SYNC_ISEXIST_PATH_NAME, "", false, false, retPath);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
	{
		printf("\n[sync_isExist_test] create path:%s failed! \n", SYNC_ISEXIST_PATH_NAME);
		goto TAG_SYNC_ISEXIST_TEST_FAILED;
	}

	ec_exist = cli->isExist(SYNC_ISEXIST_PATH_NAME);
	assert(ec_exist == ZkUtil::kZKSucceed);

	printf("sync_isExist_test succeed!\n");
	return;

TAG_SYNC_ISEXIST_TEST_FAILED:
	printf("\n[sync_isExist_test] failed! \n");
	return;
}


void sync_create_test_1(std::string test_path)
{
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec;
	ZkUtil::ZkErrorCode ec_other;
	bool isTemp = false;
	bool isSeq = false;

	ec = cli->create(test_path, "", isTemp, isSeq, retPath);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
	{
		printf("\n[sync_create_test] create path:%s failed! \n", test_path.c_str());
		goto TAG_SYNC_CREATE_TEST_FAILED_1;
	}

	//���� ���ص�·�� ��ȷ
	printf("[not temp, not seq ] retPath:%s, original path:%s\n", retPath.c_str(), test_path.c_str());
	assert(retPath == test_path);

	//���� �����ɹ���
	ec_other = cli->isExist(test_path);
	assert(ec_other == ZkUtil::kZKSucceed);

	printf("sync_create_test_1 succeed!\n");
	return;

TAG_SYNC_CREATE_TEST_FAILED_1:
	printf("\n[sync_create_test_1] failed! \n");
	return;
}

void sync_create_test_2(std::string test_path)
{
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec;
	ZkUtil::ZkErrorCode ec_other;
	uint handle_temp;
	ZkClientPtr cli_temp;
	bool isTemp = true;
	bool isSeq = false;

	//����һ��session
	handle_temp = ZkClientManager::instance().createZkClient(ZOOKEEPER_SERVER_CONN_STRING, 30000, NULL, NULL, NULL);
	if (handle_temp == 0)
	{
		printf("[sync_create_test] create session failed! connStr:%s\n", ZOOKEEPER_SERVER_CONN_STRING);
		goto TAG_SYNC_CREATE_TEST_FAILED_2;
	}

	cli_temp = ZkClientManager::instance().getZkClient(handle_temp);
	ec = cli_temp->create(test_path, "", isTemp, isSeq, retPath);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
	{
		printf("\n[sync_create_test] create path:%s failed! \n", test_path.c_str());
		goto TAG_SYNC_CREATE_TEST_FAILED_2;
	}

	//���� ���ص�·�� ��ȷ
	assert(retPath == test_path);

	//���� �����ɹ���
	ec_other = cli_temp->isExist(test_path);
	assert(ec_other == ZkUtil::kZKSucceed);

	//���ٵ�ǰsession
	ZkClientManager::instance().destroyClient(handle_temp);
	cli_temp.reset();

	printf("distroy this zkclient, session Handle:%d\n", handle_temp);
	sleep(5);  //�ȴ�5�룬temp session ����

	ec_other = cli->isExist(test_path);
	assert(ec_other == ZkUtil::kZKNotExist);   //���� session���ٺ���ʱ��node Ӧ�Զ�ɾ��

	printf("sync_create_test_2 succeed!\n");
	return;

TAG_SYNC_CREATE_TEST_FAILED_2:
	printf("\n[sync_create_test_2] failed! \n");
	return;
}

void sync_create_test_3(std::string test_path)
{
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec;
	ZkUtil::ZkErrorCode ec_other;
	bool isTemp = false;
	bool isSeq = true;

	ec = cli->create(test_path, "", isTemp, isSeq, retPath);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
	{
		printf("\n[sync_create_test] create path:%s failed! \n", test_path.c_str());
		goto TAG_SYNC_CREATE_TEST_FAILED_3;
	}

	//���� ���ص�·�� ��ȷ
	printf("[not temp, seq] retPath:%s, original path:%s\n", retPath.c_str(), test_path.c_str());
	assert(retPath != test_path);

	//���� �����ɹ���
	ec_other = cli->isExist(retPath);
	assert(ec_other == ZkUtil::kZKSucceed);

	printf("sync_create_test_3 succeed!\n");
	return;

TAG_SYNC_CREATE_TEST_FAILED_3:
	printf("\n[sync_create_test_3] failed! \n");
	return;
}

void sync_create_test_4(std::string test_path)
{
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec;
	ZkUtil::ZkErrorCode ec_other;
	uint handle_temp;
	ZkClientPtr cli_temp;
	bool isTemp = true;
	bool isSeq = true;

	//����һ��session
	handle_temp = ZkClientManager::instance().createZkClient(ZOOKEEPER_SERVER_CONN_STRING, 30000, NULL, NULL, NULL);
	if (handle_temp == 0)
	{
		printf("[sync_create_test] create session failed! connStr:%s\n", ZOOKEEPER_SERVER_CONN_STRING);
		goto TAG_SYNC_CREATE_TEST_FAILED_4;
	}

	cli_temp = ZkClientManager::instance().getZkClient(handle_temp);
	ec = cli_temp->create(test_path, "", isTemp, isSeq, retPath);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
	{
		printf("\n[sync_create_test] create path:%s failed! \n", test_path.c_str());
		goto TAG_SYNC_CREATE_TEST_FAILED_4;
	}

	//���� ���ص�·�� ��ȷ
	printf("[temp, seq] retPath:%s, original path:%s\n", retPath.c_str(), test_path.c_str());
	assert(retPath != test_path);

	//���� �����ɹ���
	ec_other = cli_temp->isExist(retPath);
	assert(ec_other == ZkUtil::kZKSucceed);

	//���ٵ�ǰsession
	ZkClientManager::instance().destroyClient(handle_temp);
	cli_temp.reset();

	printf("distroy this zkclient, session Handle:%d\n", handle_temp);
	sleep(5);  //�ȴ�5�룬temp session ����

	ec_other = cli->isExist(retPath);
	assert(ec_other == ZkUtil::kZKNotExist);   //���� session���ٺ���ʱ��node Ӧ�Զ�ɾ��

	printf("sync_create_test_4 succeed!\n");
	return;

TAG_SYNC_CREATE_TEST_FAILED_4:
	printf("\n[sync_create_test_4] failed! \n");
	return;
}

void sync_create_test_5(std::string parentPath, std::string childPath)
{
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec;
	ZkUtil::ZkErrorCode ec_other;
	bool isTemp = false;
	bool isSeq = false;

	//���� ��������Ľ���У�����㻹δ������Ӧ����ʧ��
	ec = cli->create(childPath, "", isTemp, isSeq, retPath);
	assert(ec == ZkUtil::kZKNotExist);

	//�ȴ��������
	ec = cli->create(parentPath, "", isTemp, isSeq, retPath);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
	{
		printf("\n[sync_create_test] create path:%s failed! \n", parentPath.c_str());
		goto TAG_SYNC_CREATE_TEST_FAILED_5;
	}

	//���� ���ص�·�� ��ȷ
	printf("[not temp, not seq ] retPath:%s, original path:%s\n", retPath.c_str(), parentPath.c_str());
	assert(retPath == parentPath);

	//���� �����ɹ���
	ec_other = cli->isExist(parentPath);
	assert(ec_other == ZkUtil::kZKSucceed);

	//�ٴ����ӽ��
	ec = cli->create(childPath, "", false, false, retPath);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
	{
		printf("\n[sync_create_test] create path:%s failed! \n", childPath.c_str());
		goto TAG_SYNC_CREATE_TEST_FAILED_5;
	}

	//���� ���ص�·�� ��ȷ
	printf("[not temp, not seq ] retPath:%s, original path:%s\n", retPath.c_str(), childPath.c_str());
	assert(retPath == childPath);

	//���� �����ɹ���
	ec_other = cli->isExist(childPath);
	assert(ec_other == ZkUtil::kZKSucceed);

	printf("sync_create_test_5 succeed!\n");
	return;

TAG_SYNC_CREATE_TEST_FAILED_5:
	printf("\n[sync_create_test_5] failed! \n");
	return;
}

void sync_create_seq_no_test()
{
#define  SYNC_CREATE_SEQ_NO_PATH_NAME     "/sync_create_seq_no_test_node"

    ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
    std::string retPath = "";
    std::string firstRetPath = "";
    ZkUtil::ZkErrorCode ec;
    bool isTemp = true;
    bool isSeq = true;

    ec = cli->create(SYNC_CREATE_SEQ_NO_PATH_NAME, "", isTemp, isSeq, retPath);
    if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
    {
        printf("\n[sync_create_seq_no_test] create path:%s failed! \n", SYNC_CREATE_SEQ_NO_PATH_NAME);
        return;
    }
    printf("\n[sync_create_seq_no_test] retPath:%s \n", retPath.c_str());
    firstRetPath = retPath;

    ec = cli->create(SYNC_CREATE_SEQ_NO_PATH_NAME, "", isTemp, isSeq, retPath);
    if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
    {
        printf("\n[sync_create_seq_no_test] create path:%s failed! \n", SYNC_CREATE_SEQ_NO_PATH_NAME);
        return;
    }
    printf("\n[sync_create_seq_no_test] retPath:%s \n", retPath.c_str());

    ec = cli->create(SYNC_CREATE_SEQ_NO_PATH_NAME, "", isTemp, isSeq, retPath);
    if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
    {
        printf("\n[sync_create_seq_no_test] create path:%s failed! \n", SYNC_CREATE_SEQ_NO_PATH_NAME);
        return;
    }
    printf("\n[sync_create_seq_no_test] retPath:%s \n", retPath.c_str());

    ec = cli->create(SYNC_CREATE_SEQ_NO_PATH_NAME, "", isTemp, isSeq, retPath);
    if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
    {
        printf("\n[sync_create_seq_no_test] create path:%s failed! \n", SYNC_CREATE_SEQ_NO_PATH_NAME);
        return;
    }
    printf("\n[sync_create_seq_no_test] retPath:%s \n", retPath.c_str());

    //ɾ�����
    printf("\n[sync_create_seq_no_test] delete path:%s \n", firstRetPath.c_str());
    ec = cli->deleteRecursive(firstRetPath);
    assert(ec != ZkUtil::kZKError);

    ec = cli->create(SYNC_CREATE_SEQ_NO_PATH_NAME, "", isTemp, isSeq, retPath);
    if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
    {
        printf("\n[sync_create_seq_no_test] create path:%s failed! \n", SYNC_CREATE_SEQ_NO_PATH_NAME);
        return;
    }
    printf("\n[sync_create_seq_no_test] retPath:%s \n", retPath.c_str());
    firstRetPath = retPath;

    ec = cli->create(SYNC_CREATE_SEQ_NO_PATH_NAME, "", isTemp, isSeq, retPath);
    if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
    {
        printf("\n[sync_create_seq_no_test] create path:%s failed! \n", SYNC_CREATE_SEQ_NO_PATH_NAME);
        return;
    }
    printf("\n[sync_create_seq_no_test] retPath:%s \n", retPath.c_str());

    //ɾ�����
    printf("\n[sync_create_seq_no_test] delete path:%s \n", firstRetPath.c_str());
    ec = cli->deleteRecursive(firstRetPath);
    assert(ec != ZkUtil::kZKError);

    ec = cli->create(SYNC_CREATE_SEQ_NO_PATH_NAME, "", isTemp, isSeq, retPath);
    if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
    {
        printf("\n[sync_create_seq_no_test] create path:%s failed! \n", SYNC_CREATE_SEQ_NO_PATH_NAME);
        return;
    }
    printf("\n[sync_create_seq_no_test] retPath:%s \n", retPath.c_str());

    ec = cli->create(SYNC_CREATE_SEQ_NO_PATH_NAME, "", isTemp, isSeq, retPath);
    if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
    {
        printf("\n[sync_create_seq_no_test] create path:%s failed! \n", SYNC_CREATE_SEQ_NO_PATH_NAME);
        return;
    }
    printf("\n[sync_create_seq_no_test] retPath:%s \n", retPath.c_str());

}



void sync_create_test()
{
#define  SYNC_CREATE_PATH_NAME_1     "/sync_create_test_no_temp_no_seq"
#define  SYNC_CREATE_PATH_NAME_2     "/sync_create_test_temp_no_seq"
#define  SYNC_CREATE_PATH_NAME_3     "/sync_create_test_no_temp_seq"
#define  SYNC_CREATE_PATH_NAME_4     "/sync_create_test_temp_seq"
#define  SYNC_CREATE_PATH_NAME_5     "/sync_create_test_root"
#define  SYNC_CREATE_PATH_NAME_6     "/sync_create_test_root/sync_create_child"

	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec;

	//��ɾ�� 5 �����
	ec = cli->deleteNode(SYNC_CREATE_PATH_NAME_1);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKNotExist)
	{
		printf("\n[sync_create_test] delete path:%s failed! \n", SYNC_CREATE_PATH_NAME_1);
		goto TAG_SYNC_CREATE_TEST_FAILED;
	}

	ec = cli->deleteNode(SYNC_CREATE_PATH_NAME_2);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKNotExist)
	{
		printf("\n[sync_create_test] delete path:%s failed! \n", SYNC_CREATE_PATH_NAME_2);
		goto TAG_SYNC_CREATE_TEST_FAILED;
	}

	ec = cli->deleteNode(SYNC_CREATE_PATH_NAME_3);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKNotExist)
	{
		printf("\n[sync_create_test] delete path:%s failed! \n", SYNC_CREATE_PATH_NAME_3);
		goto TAG_SYNC_CREATE_TEST_FAILED;
	}

	ec = cli->deleteNode(SYNC_CREATE_PATH_NAME_4);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKNotExist)
	{
		printf("\n[sync_create_test] delete path:%s failed! \n", SYNC_CREATE_PATH_NAME_4);
		goto TAG_SYNC_CREATE_TEST_FAILED;
	}

	//Ӧ�� ɾ�� Ҷ�ӽ�㣬��ɾ�� ��֧���
	ec = cli->deleteNode(SYNC_CREATE_PATH_NAME_6);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKNotExist)
	{
		printf("\n[sync_create_test] delete path:%s failed! \n", SYNC_CREATE_PATH_NAME_6);
		goto TAG_SYNC_CREATE_TEST_FAILED;
	}

	ec = cli->deleteNode(SYNC_CREATE_PATH_NAME_5);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKNotExist)
	{
		printf("\n[sync_create_test] delete path:%s failed! \n", SYNC_CREATE_PATH_NAME_5);
		goto TAG_SYNC_CREATE_TEST_FAILED;
	}

	//����not temp, not seq �����//////////////////////////////
	sync_create_test_1(SYNC_CREATE_PATH_NAME_1);

	//����temp, not seq �����////////////////////////////////
	sync_create_test_2(SYNC_CREATE_PATH_NAME_2);

	//����not temp, seq �����////////////////////////////////
	sync_create_test_3(SYNC_CREATE_PATH_NAME_3);

	//����temp, seq �����////////////////////////////////
	sync_create_test_4(SYNC_CREATE_PATH_NAME_4);

	//���� ��������Ľ���У�����㻹δ������Ӧ����ʧ��
	sync_create_test_5(SYNC_CREATE_PATH_NAME_5, SYNC_CREATE_PATH_NAME_6);

	printf("sync_create_test succeed!\n");
	return;

TAG_SYNC_CREATE_TEST_FAILED:
	printf("\n[sync_create_test] failed! \n");
	return;
}

void sync_createIfNeedCreateParents_test()
{
#define  SYNC_CREATEIFNEEDCREATEPARENT_PATH_NAME_1     "/sync_createIfNeedCreateParents_test_parent"
#define  SYNC_CREATEIFNEEDCREATEPARENT_PATH_NAME_2     "/sync_createIfNeedCreateParents_test_parent/sync_createIfNeedCreateParents_test_child"

	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec;
	ZkUtil::ZkErrorCode ec_other;
	bool isTemp = false;
	bool isSeq = false;

	//Ӧ�� ɾ�� Ҷ�ӽ�㣬��ɾ�� ��֧���
	ec = cli->deleteNode(SYNC_CREATEIFNEEDCREATEPARENT_PATH_NAME_2);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKNotExist)
	{
		printf("\n[sync_createIfNeedCreateParents_test] delete path:%s failed! \n", SYNC_CREATEIFNEEDCREATEPARENT_PATH_NAME_2);
		goto TAG_SYNC_CREATEIFNEEDCREATEPARENT_TEST_FAILED;
	}

	ec = cli->deleteNode(SYNC_CREATEIFNEEDCREATEPARENT_PATH_NAME_1);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKNotExist)
	{
		printf("\n[sync_createIfNeedCreateParents_test] delete path:%s failed! \n", SYNC_CREATEIFNEEDCREATEPARENT_PATH_NAME_1);
		goto TAG_SYNC_CREATEIFNEEDCREATEPARENT_TEST_FAILED;
	}

	//ֱ�Ӵ���Ҷ�ӽ��
	ec = cli->createIfNeedCreateParents(SYNC_CREATEIFNEEDCREATEPARENT_PATH_NAME_2, "", isTemp, isSeq, retPath);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
	{
		printf("\n[sync_createIfNeedCreateParents_test] create path:%s failed! \n", SYNC_CREATEIFNEEDCREATEPARENT_PATH_NAME_2);
		goto TAG_SYNC_CREATEIFNEEDCREATEPARENT_TEST_FAILED;
	}

	//���� ���ص�·�� ��ȷ
	printf("retPath:%s, original path:%s\n", retPath.c_str(), SYNC_CREATEIFNEEDCREATEPARENT_PATH_NAME_2);
	assert(retPath == SYNC_CREATEIFNEEDCREATEPARENT_PATH_NAME_2);

	//���� �����ɹ���
	ec_other = cli->isExist(SYNC_CREATEIFNEEDCREATEPARENT_PATH_NAME_2);
	assert(ec_other == ZkUtil::kZKSucceed);

	printf("sync_createIfNeedCreateParents_test succeed!\n");
	return;

TAG_SYNC_CREATEIFNEEDCREATEPARENT_TEST_FAILED:
	printf("\n[sync_createIfNeedCreateParents_test] failed! \n");
	return;
}

void sync_set_test()
{
#define  SYNC_SET_PATH_NAME     "/sync_set_test"

	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec;
	ZkUtil::ZkErrorCode ec_other;
	bool isTemp = false;
	bool isSeq = false;
	std::string value1 = "";
	int32_t version1;
	std::string value2 = "";
	int32_t version2;
	std::string value3 = "";
	int32_t version3;

	//�������Խ��
	ec = cli->create(SYNC_SET_PATH_NAME, "", isTemp, isSeq, retPath);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
	{
		printf("\n[sync_set_test] create path:%s failed! \n", SYNC_SET_PATH_NAME);
		goto TAG_SYNC_SET_TEST_FAILED;
	}

	//��ȡ���Խ���ֵ���汾��
	if (cli->getNode(SYNC_SET_PATH_NAME, value1, version1) != ZkUtil::kZKSucceed)
	{
		printf("\n[sync_set_test] getNode:%s failed! sync_getNode_test failed!\n", SYNC_GETNODE_PATH_NAME);
		goto TAG_SYNC_SET_TEST_FAILED;
	}

	//���ý��� �汾�Ų��ԣ�Ӧ����ʧ��
	ec_other = cli->set(SYNC_SET_PATH_NAME, "test_value_1", version1 + 1);
	assert(ec_other == ZkUtil::kZKError);


	//���ý��� �汾����ȷ��Ӧ���سɹ�
	ec_other = cli->set(SYNC_SET_PATH_NAME, "test_value_2", version1);
	assert(ec_other == ZkUtil::kZKSucceed);

	//��ȡ���Խ���ֵ���汾��
	if (cli->getNode(SYNC_SET_PATH_NAME, value2, version2) != ZkUtil::kZKSucceed)
	{
		printf("\n[sync_set_test] getNode:%s failed! sync_getNode_test failed!\n", SYNC_SET_PATH_NAME);
		goto TAG_SYNC_SET_TEST_FAILED;
	}
	assert(value2 == "test_value_2");


	//������汾�ţ���Ĭ�� ��������İ汾��
	ec_other = cli->set(SYNC_SET_PATH_NAME, "test_value_3");
	assert(ec_other == ZkUtil::kZKSucceed);

	//��ȡ���Խ���ֵ���汾��
	if (cli->getNode(SYNC_SET_PATH_NAME, value3, version3) != ZkUtil::kZKSucceed)
	{
		printf("\n[sync_set_test] getNode:%s failed! sync_getNode_test failed!\n", SYNC_GETNODE_PATH_NAME);
		goto TAG_SYNC_SET_TEST_FAILED;
	}
	assert(value3 == "test_value_3");

	printf("sync_set_test succeed!\n");
	return;
TAG_SYNC_SET_TEST_FAILED:
	printf("\n[sync_set_test] failed! \n");
	return;
}

void sync_deleteNode_test()
{
#define  SYNC_DELETENODE_PATH_NAME_1     "/sync_deleteNode_test_parent"
#define  SYNC_DELETENODE_PATH_NAME_2     "/sync_deleteNode_test_parent/sync_deleteNode_test_child"
#define  SYNC_DELETENODE_PATH_NAME_3     "/sync_deleteNode_test_node"

	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec;
	ZkUtil::ZkErrorCode ec_other;
	bool isTemp = false;
	bool isSeq = false;
	std::string value = "";
	int32_t version = 0;

	//ֱ�Ӵ���Ҷ�ӽ��
	ec = cli->createIfNeedCreateParents(SYNC_DELETENODE_PATH_NAME_2, "", isTemp, isSeq, retPath);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
	{
		printf("\n[sync_deleteNode_test] create path:%s failed! \n", SYNC_DELETENODE_PATH_NAME_2);
		goto TAG_SYNC_DELETENODE_TEST_FAILED;
	}

	ec = cli->createIfNeedCreateParents(SYNC_DELETENODE_PATH_NAME_3, "", isTemp, isSeq, retPath);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
	{
		printf("\n[sync_deleteNode_test] create path:%s failed! \n", SYNC_DELETENODE_PATH_NAME_3);
		goto TAG_SYNC_DELETENODE_TEST_FAILED;
	}


	//��ɾ���Ľ���� �����ӽ�㣬��ɾ��ʧ��
	ec_other = cli->deleteNode(SYNC_DELETENODE_PATH_NAME_1);
	assert(ec_other == ZkUtil::kZKNotEmpty);


	//��ɾ���ӽ�㣬��ɾ����֧���
	ec_other = cli->deleteNode(SYNC_DELETENODE_PATH_NAME_2);
	assert(ec_other == ZkUtil::kZKSucceed);

	ec_other = cli->deleteNode(SYNC_DELETENODE_PATH_NAME_1);
	assert(ec_other == ZkUtil::kZKSucceed);

	//���� ɾ���ɹ���
	ec_other = cli->isExist(SYNC_DELETENODE_PATH_NAME_1);
	assert(ec_other == ZkUtil::kZKNotExist);

	ec_other = cli->isExist(SYNC_DELETENODE_PATH_NAME_2);
	assert(ec_other == ZkUtil::kZKNotExist);



	//��ȡ���Խ���ֵ���汾��
	if (cli->getNode(SYNC_DELETENODE_PATH_NAME_3, value, version) != ZkUtil::kZKSucceed)
	{
		printf("\n[sync_deleteNode_test] getNode:%s failed! sync_getNode_test failed!\n", SYNC_DELETENODE_PATH_NAME_3);
		goto TAG_SYNC_DELETENODE_TEST_FAILED;
	}

	//��ɾ���Ľ�� �İ汾�Ų��ԣ��򷵻�ʧ��
	ec_other = cli->deleteNode(SYNC_DELETENODE_PATH_NAME_3, version + 1);
	assert(ec_other == ZkUtil::kZKError);

	//��ɾ���Ľ�� �İ汾����ȷ����ɾ���ɹ�
	ec_other = cli->deleteNode(SYNC_DELETENODE_PATH_NAME_3, version);
	assert(ec_other == ZkUtil::kZKSucceed);

	//���� ɾ���ɹ���
	ec_other = cli->isExist(SYNC_DELETENODE_PATH_NAME_3);
	assert(ec_other == ZkUtil::kZKNotExist);


	printf("sync_deleteNode_test succeed!\n");
	return;

TAG_SYNC_DELETENODE_TEST_FAILED:
	printf("\n[sync_deleteNode_test] failed! \n");
	return;
}

void sync_deleteRecursive_test()
{
#define  SYNC_DELETERECURISIVE_PATH_NAME_1     "/sync_deleteRecursive_test_parent"
#define  SYNC_DELETERECURISIVE_PATH_NAME_2     "/sync_deleteRecursive_test_parent/sync_deleteRecursive_test_child"

	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec;
	ZkUtil::ZkErrorCode ec_other;
	bool isTemp = false;
	bool isSeq = false;
	std::string value = "";
	int32_t version = 0;

	//ֱ�Ӵ���Ҷ�ӽ��
	ec = cli->createIfNeedCreateParents(SYNC_DELETERECURISIVE_PATH_NAME_2, "", isTemp, isSeq, retPath);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
	{
		printf("\n[sync_deleteNode_test] create path:%s failed! \n", SYNC_DELETERECURISIVE_PATH_NAME_2);
		goto TAG_SYNC_DELETERECRUSIVE_TEST_FAILED;
	}

	//��ɾ���Ľ�� �İ汾����ȷ����ɾ���ɹ�
	ec_other = cli->deleteRecursive(SYNC_DELETERECURISIVE_PATH_NAME_1);
	assert(ec_other == ZkUtil::kZKSucceed);

	//���� ɾ���ɹ���
	ec_other = cli->isExist(SYNC_DELETERECURISIVE_PATH_NAME_1);
	assert(ec_other == ZkUtil::kZKNotExist);

	ec_other = cli->isExist(SYNC_DELETERECURISIVE_PATH_NAME_2);
	assert(ec_other == ZkUtil::kZKNotExist);

	printf("sync_deleteRecursive_test succeed!\n");
	return;

TAG_SYNC_DELETERECRUSIVE_TEST_FAILED:
	printf("\n[sync_deleteRecursive_test] failed! \n");
	return;
}

void asyn_getNode_test_cb(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path,
	const std::string& value, int32_t version, void* context);

void asyn_getNode_test()
{
#define  ASYNC_GETNODE_PATH_NAME     "/async_getNode_test"
#define  ASYNC_GETNODE_PATH_VALUE     "async_getNode_test_value"

	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec = cli->create(ASYNC_GETNODE_PATH_NAME, ASYNC_GETNODE_PATH_VALUE, false, false, retPath);
	if (ec == ZkUtil::kZKSucceed || ec == ZkUtil::kZKExisted)
	{
		if (cli->getNode(ASYNC_GETNODE_PATH_NAME, boost::bind(&asyn_getNode_test_cb, _1, _2, _3, _4,_5, _6), NULL) == false)
		{
			printf("\n[asyn_getNode_test] getNode:%s failed! async_getNode_test failed!\n", ASYNC_GETNODE_PATH_NAME);
			return;
		}
	}
	else
	{
		printf("\n[asyn_getNode_test] create path:%s failed! async_getNode_test failed!\n", ASYNC_GETNODE_PATH_NAME);
		return;
	}
}

void asyn_getNode_test_cb(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path,
		const std::string& value, int32_t version, void* context)
{
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	assert(cli == client);

	if (errcode == ZkUtil::kZKSucceed)
	{
		printf("\n[asyn_getNode_test_cb] path:%s, value:%s, version:%d\n", path.c_str(), value.c_str(), version);
		assert(path == ASYNC_GETNODE_PATH_NAME);
		assert(value == ASYNC_GETNODE_PATH_VALUE);
		printf("asyn_getNode_test succeed!\n");
	}
	else
	{
		printf("asyn getNode failed, path:%s\n", ASYNC_GETNODE_PATH_NAME);
	}
}

void asyn_getChildren_test_cb(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, 
								const std::vector<std::string>& childNodes, void* context);

void asyn_getChildren_test()
{
#define  ASYNC_GETCHILDREN_BASE_NAME     "/async_getChildren_test_base"
#define  ASYNC_GETCHILDREN_CHILD_NAME    "async_getChildren_test_child"
#define  ASYNC_GETCHILDREN_CHILD_NUM      10

	std::vector<std::string> *orignalChildList = new std::vector<std::string>;
	std::vector<std::string> childNodes;

	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec = cli->create(ASYNC_GETCHILDREN_BASE_NAME, "", false, false, retPath);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
	{
		printf("\n[async_getChildren_test] create path:%s failed! \n", ASYNC_GETCHILDREN_BASE_NAME);
		goto TAG_ASYNC_GETCHILDREN_TEST_FAILED;
	}

	for (int i = 0; i < ASYNC_GETCHILDREN_CHILD_NUM; i++)
	{
		char path[100] = {0};
		char childName[100] = {0};
		snprintf(childName, sizeof(childName), "%s_%d", ASYNC_GETCHILDREN_CHILD_NAME, i);
		snprintf(path, sizeof(path), "%s/%s_%d", ASYNC_GETCHILDREN_BASE_NAME, ASYNC_GETCHILDREN_CHILD_NAME, i);
		orignalChildList->push_back(childName);

		ZkUtil::ZkErrorCode ec_chd = cli->create(path, "", false, false, retPath);
		if (ec_chd != ZkUtil::kZKSucceed && ec_chd != ZkUtil::kZKExisted)
		{
			printf("\n[async_getChildren_test] create path:%s failed! \n", path);
			goto TAG_ASYNC_GETCHILDREN_TEST_FAILED;
		}
	}

	if (cli->getChildren(ASYNC_GETCHILDREN_BASE_NAME, boost::bind(&asyn_getChildren_test_cb, _1, _2, _3, _4,_5), orignalChildList) == false)
	{
		delete orignalChildList;
		orignalChildList = NULL;

		printf("\n[async_getChildren_test] getChildren failed! path:%s \n", ASYNC_GETCHILDREN_BASE_NAME);
		goto TAG_ASYNC_GETCHILDREN_TEST_FAILED;
	}

	return;

TAG_ASYNC_GETCHILDREN_TEST_FAILED:
	printf("\n[async_getChildren_test] failed! \n");
	return;
}

void asyn_getChildren_test_cb(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path,
	const std::vector<std::string>& childNodes, void* context)
{
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	assert(cli == client);

	assert(context != NULL);
	std::vector<std::string> *orignalChildList = (std::vector<std::string> *)context;

	if (errcode == ZkUtil::kZKSucceed)
	{
		printChild(path, childNodes);

		assert(path == ASYNC_GETCHILDREN_BASE_NAME);
		assert(isEqualChildren(*orignalChildList, childNodes) == true);

		delete orignalChildList;
		orignalChildList = NULL;
		printf("async_getChildren_test succeed!\n");
	}
	else
	{
		printf("asyn get child failed.\n");
	}
}


void asyn_isExist_test_cb(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, void* context);

void asyn_isExist_test()
{
#define  ASYNC_ISEXIST_PATH_NAME     "/async_isExist_test_base"

	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec_exist;

	ZkUtil::ZkErrorCode ec = cli->create(ASYNC_ISEXIST_PATH_NAME, "", false, false, retPath);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
	{
		printf("\n[async_isExist_test] create path:%s failed! \n", ASYNC_ISEXIST_PATH_NAME);
		goto TAG_ASYNC_ISEXIST_TEST_FAILED;
	}

	if (cli->isExist(ASYNC_ISEXIST_PATH_NAME, boost::bind(&asyn_isExist_test_cb, _1, _2, _3, _4), NULL) == false)
	{
		printf("\n[asyn_isExist_test] isExist failed! path:%s \n", ASYNC_ISEXIST_PATH_NAME);
		goto TAG_ASYNC_ISEXIST_TEST_FAILED;
	}

	return;

TAG_ASYNC_ISEXIST_TEST_FAILED:
	printf("\n[async_isExist_test] failed! \n");
	return;
}

void asyn_isExist_test_cb(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, void* context)
{
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	assert(cli == client);
	assert(errcode == ZkUtil::kZKSucceed);
	assert(path == ASYNC_ISEXIST_PATH_NAME);

	printf("asyn_isExist_test_cb succeed!\n");
}







void async_create_test_1_cb(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, const std::string& value, void* context);

void async_create_test_1(std::string test_path)
{
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	ZkUtil::ZkErrorCode ec;
	bool isTemp = false;
	bool isSeq = false;
	std::string *orignalPath = new std::string(test_path);
	if (cli->create(test_path, "", boost::bind(&async_create_test_1_cb, _1, _2, _3, _4, _5), orignalPath, isTemp, isSeq) == false)
	{
		delete orignalPath;
		orignalPath = NULL;
		printf("\n[async_create_test_1] create path failed! path:%s \n", test_path.c_str());
		return;
	}
}

void async_create_test_1_cb(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, const std::string& value, void* context)
{
	assert(errcode == ZkUtil::kZKSucceed);
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	assert(cli == client);

	assert(context != NULL);
	std::string* orignalPath = (std::string*) context;
	assert(path == (*orignalPath));

	//���� �����ɹ���
	ZkUtil::ZkErrorCode ec_other;
	ec_other = cli->isExist(path);
	assert(ec_other == ZkUtil::kZKSucceed);

	delete orignalPath;
	orignalPath = NULL;
	printf("async_create_test_1_cb succeed!\n");
}




CountDownLatch *gpCreateTest2CountDown = NULL;

void async_create_test_2_cb(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, const std::string& value, void* context);

void async_create_test_2(std::string test_path)
{
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec;
	ZkUtil::ZkErrorCode ec_other;
	uint handle_temp;
	ZkClientPtr cli_temp;
	std::string *orignalPath = NULL;
	bool isTemp = true;
	bool isSeq = false;

	//����һ��session
	handle_temp = ZkClientManager::instance().createZkClient(ZOOKEEPER_SERVER_CONN_STRING, 30000, NULL, NULL, NULL);
	if (handle_temp == 0)
	{
		printf("[async_create_test] create session failed! connStr:%s\n", ZOOKEEPER_SERVER_CONN_STRING);
		goto TAG_ASYNC_CREATE_TEST_FAILED_2;
	}

	cli_temp = ZkClientManager::instance().getZkClient(handle_temp);

	gpCreateTest2CountDown = new muduo::CountDownLatch(1);
	orignalPath = new std::string(test_path);
	if (cli_temp->create(test_path, "", boost::bind(&async_create_test_2_cb, _1, _2, _3, _4, _5), orignalPath, isTemp, isSeq) == false)
	{
		delete orignalPath;
		orignalPath = NULL;
		delete gpCreateTest2CountDown;
		gpCreateTest2CountDown = NULL;
		printf("\n[async_create_test_2] create path failed! path:%s \n", test_path.c_str());
		return;
	}

	gpCreateTest2CountDown->wait();

	//���ٵ�ǰsession
	ZkClientManager::instance().destroyClient(handle_temp);
	cli_temp.reset();

	printf("distroy this zkclient, session Handle:%d. please wait...\n", handle_temp);
	sleep(5);  //�ȴ�5�룬temp session ����

	ec_other = cli->isExist(test_path);
	printf("ec:%d, path:%s \n", ec_other, test_path.c_str());

	assert(ec_other == ZkUtil::kZKNotExist);   //���� session���ٺ���ʱ��node Ӧ�Զ�ɾ��

	delete gpCreateTest2CountDown;
	gpCreateTest2CountDown = NULL;

	printf("async_create_test_2 succeed!\n");

	return;

TAG_ASYNC_CREATE_TEST_FAILED_2:
	printf("\n[async_create_test_2] failed! \n");
	return;
}

void async_create_test_2_cb(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, const std::string& value, void* context)
{
	assert(errcode == ZkUtil::kZKSucceed);
	assert(context != NULL);
	std::string* orignalPath = (std::string*) context;
	assert(path == (*orignalPath));

	//���� �����ɹ���
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	ZkUtil::ZkErrorCode ec_other;
	ec_other = cli->isExist(path);
	assert(ec_other == ZkUtil::kZKSucceed);

	delete orignalPath;
	orignalPath = NULL;

	if (gpCreateTest2CountDown)
	{
		gpCreateTest2CountDown->countDown();
	}
}



void async_create_test_3_cb(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, const std::string& value, void* context);

void async_create_test_3(std::string test_path)
{

	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec;
	ZkUtil::ZkErrorCode ec_other;
	bool isTemp = false;
	bool isSeq = true;

	std::string *orignalPath = new std::string(test_path);
	if (cli->create(test_path, "", boost::bind(&async_create_test_3_cb, _1, _2, _3, _4, _5), orignalPath, isTemp, isSeq) == false)
	{
		delete orignalPath;
		orignalPath = NULL;
		printf("\n[async_create_test_2] create path failed! path:%s \n", test_path.c_str());
		return;
	}
}

void async_create_test_3_cb(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, const std::string& value, void* context)
{
	assert(errcode == ZkUtil::kZKSucceed);
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	assert(cli == client);

	assert(context != NULL);
	std::string* orignalPath = (std::string*) context;
	printf("seqRetPath:%s, originalPath:%s\n", path.c_str(), orignalPath->c_str());
	//assert(path != (*orignalPath));
	assert(path == (*orignalPath));

	//���� �����ɹ���   ��Ϊ���ﷵ�صĲ��� ������� ��յĽ���������Բ���ֱ����path���жϣ���Ӧ���Ȼ�ȡ��ʵ�Ľ���������ж��Ƿ����
	//ZkUtil::ZkErrorCode ec_other = cli->isExist(path);
	//assert(ec_other == ZkUtil::kZKSucceed);

	delete orignalPath;
	orignalPath = NULL;

	printf("async_create_test_3_cb succeed!\n");
	return;
}


struct CreateTest4Context
{
	std::string originalPath_;
	std::string retPath_;
};

CountDownLatch *gpCreateTest4CountDown = NULL;

void async_create_test_4_cb(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, const std::string& value, void* context);

void async_create_test_4(std::string test_path)
{

	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec;
	ZkUtil::ZkErrorCode ec_other;
	uint handle_temp;
	ZkClientPtr cli_temp;
	bool isTemp = true;
	bool isSeq = true;
	CreateTest4Context *context;

	//����һ��session
	handle_temp = ZkClientManager::instance().createZkClient(ZOOKEEPER_SERVER_CONN_STRING, 30000, NULL, NULL, NULL);
	if (handle_temp == 0)
	{
		printf("[async_create_test_4] create session failed! connStr:%s\n", ZOOKEEPER_SERVER_CONN_STRING);
		goto TAG_ASYNC_CREATE_TEST_FAILED_4;
	}

	cli_temp = ZkClientManager::instance().getZkClient(handle_temp);

	gpCreateTest4CountDown = new muduo::CountDownLatch(1);
	context = new CreateTest4Context();
	context->originalPath_ = test_path;
	if (cli_temp->create(test_path, "", boost::bind(&async_create_test_4_cb, _1, _2, _3, _4, _5), context, isTemp, isSeq) == false)
	{
		delete context;
		context = NULL;
		delete gpCreateTest4CountDown;
		gpCreateTest4CountDown = NULL;
		printf("\n[async_create_test_4] create path failed! path:%s \n", test_path.c_str());
		return;
	}

	gpCreateTest4CountDown->wait();

	//���ٵ�ǰsession
	ZkClientManager::instance().destroyClient(handle_temp);
	cli_temp.reset();

	printf("distroy this zkclient, session Handle:%d. please wait...\n", handle_temp);
	sleep(5);  //�ȴ�5�룬temp session ����

	ec_other = cli->isExist(context->retPath_);
	assert(ec_other == ZkUtil::kZKNotExist);   //���� session���ٺ���ʱ��node Ӧ�Զ�ɾ��

	delete gpCreateTest4CountDown;
	gpCreateTest4CountDown = NULL;

	delete context;
	context = NULL;

	printf("async_create_test_4 succeed!\n");

	return;

TAG_ASYNC_CREATE_TEST_FAILED_4:
	printf("\n[async_create_test_4] failed! \n");
	return;
}

void async_create_test_4_cb(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, const std::string& value, void* context)
{
	assert(errcode == ZkUtil::kZKSucceed);
	assert(context != NULL);
	CreateTest4Context* testContext = (CreateTest4Context*) context;
	//assert(path != testContext->originalPath_);
	assert(path == testContext->originalPath_);
	testContext->retPath_ = path;

	//���� �����ɹ���   ��Ϊ���ﷵ�صĲ��� ������� ��յĽ���������Բ���ֱ����path���жϣ���Ӧ���Ȼ�ȡ��ʵ�Ľ���������ж��Ƿ����
	//ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	//ZkUtil::ZkErrorCode ec_other;
	//ec_other = cli->isExist(path);
	//assert(ec_other == ZkUtil::kZKSucceed);

	if (gpCreateTest4CountDown)
	{
		gpCreateTest4CountDown->countDown();
	}
}


CountDownLatch *gpCreateTest5CountDown = NULL;

void async_create_test_5_cb_1(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, const std::string& value, void* context);
void async_create_test_5_cb_2(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, const std::string& value, void* context);
void async_create_test_5_cb_3(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, const std::string& value, void* context);

void async_create_test_5(std::string parentPath, std::string childPath)
{
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec;
	ZkUtil::ZkErrorCode ec_other;
	bool isTemp = false;
	bool isSeq = false;
	std::string* originalParentPath;
	std::string* originalChildPath;

	//���� ��������Ľ���У�����㻹δ������Ӧ����ʧ��
	if (cli->create(childPath, "", boost::bind(&async_create_test_5_cb_1, _1, _2, _3, _4, _5), NULL, isTemp, isSeq) == false)
	{
		printf("\n[async_create_test_5] create path failed! path:%s \n", childPath.c_str());
		return;
	}

	//�ȴ��������
	gpCreateTest5CountDown = new muduo::CountDownLatch(1);
	originalParentPath = new std::string(parentPath);
	if (cli->create(parentPath, "", boost::bind(&async_create_test_5_cb_2, _1, _2, _3, _4, _5), originalParentPath, isTemp, isSeq) == false)
	{
		delete originalParentPath;
		originalParentPath = NULL;
		delete gpCreateTest5CountDown;
		gpCreateTest5CountDown = NULL;
		printf("\n[async_create_test_5] create path failed! path:%s \n", parentPath.c_str());
		return;
	}

	gpCreateTest5CountDown->wait();

	//�ٴ����ӽ��
	originalChildPath = new std::string(childPath);
	if (cli->create(childPath, "", boost::bind(&async_create_test_5_cb_3, _1, _2, _3, _4, _5), originalChildPath, isTemp, isSeq) == false)
	{
		delete originalChildPath;
		originalChildPath = NULL;
		printf("\n[async_create_test_5] create path failed! path:%s \n", childPath.c_str());
		return;
	}

	delete gpCreateTest5CountDown;
	gpCreateTest5CountDown = NULL;

	return;
}

void async_create_test_5_cb_1(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, const std::string& value, void* context)
{
	assert(errcode == ZkUtil::kZKNotExist);
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	assert(cli == client);
}

void async_create_test_5_cb_2(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, const std::string& value, void* context)
{
	assert(errcode == ZkUtil::kZKSucceed);
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	assert(cli == client);

	assert(context != NULL);
	std::string* orignalParentPath = (std::string*) context;
	assert(path == (*orignalParentPath));

	//���� �����ɹ���
	ZkUtil::ZkErrorCode ec_other = cli->isExist(path);
	assert(ec_other == ZkUtil::kZKSucceed);

	delete orignalParentPath;
	orignalParentPath = NULL;

	if (gpCreateTest5CountDown)
	{
		gpCreateTest5CountDown->countDown();
	}
}

void async_create_test_5_cb_3(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, const std::string& value, void* context)
{
	assert(errcode == ZkUtil::kZKSucceed);
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	assert(cli == client);

	assert(context != NULL);
	std::string* orignalChildPath = (std::string*) context;
	assert(path == (*orignalChildPath));

	//���� �����ɹ���
	ZkUtil::ZkErrorCode ec_other = cli->isExist(path);
	assert(ec_other == ZkUtil::kZKSucceed);

	delete orignalChildPath;
	orignalChildPath = NULL;

	printf("async_create_test_5_cb_3 succeed!\n");
}



void asyn_create_test()
{
#define  ASYNC_CREATE_PATH_NAME_1     "/async_create_test_no_temp_no_seq"
#define  ASYNC_CREATE_PATH_NAME_2     "/async_create_test_temp_no_seq"
#define  ASYNC_CREATE_PATH_NAME_3     "/async_create_test_no_temp_seq"
#define  ASYNC_CREATE_PATH_NAME_4     "/async_create_test_temp_seq"
#define  ASYNC_CREATE_PATH_NAME_5     "/async_create_test_root"
#define  ASYNC_CREATE_PATH_NAME_6     "/async_create_test_root/async_create_child"

	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec;

	//��ɾ�� 5 �����
	ec = cli->deleteNode(ASYNC_CREATE_PATH_NAME_1);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKNotExist)
	{
		printf("\n[async_create_test] delete path:%s failed! \n", ASYNC_CREATE_PATH_NAME_1);
		goto TAG_ASYNC_CREATE_TEST_FAILED;
	}

	ec = cli->deleteNode(ASYNC_CREATE_PATH_NAME_2);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKNotExist)
	{
		printf("\n[async_create_test] delete path:%s failed! \n", ASYNC_CREATE_PATH_NAME_2);
		goto TAG_ASYNC_CREATE_TEST_FAILED;
	}

	ec = cli->deleteNode(ASYNC_CREATE_PATH_NAME_3);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKNotExist)
	{
		printf("\n[async_create_test] delete path:%s failed! \n", ASYNC_CREATE_PATH_NAME_3);
		goto TAG_ASYNC_CREATE_TEST_FAILED;
	}

	ec = cli->deleteNode(ASYNC_CREATE_PATH_NAME_4);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKNotExist)
	{
		printf("\n[async_create_test] delete path:%s failed! \n", ASYNC_CREATE_PATH_NAME_4);
		goto TAG_ASYNC_CREATE_TEST_FAILED;
	}

	//Ӧ�� ɾ�� Ҷ�ӽ�㣬��ɾ�� ��֧���
	ec = cli->deleteNode(ASYNC_CREATE_PATH_NAME_6);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKNotExist)
	{
		printf("\n[async_create_test] delete path:%s failed! \n", ASYNC_CREATE_PATH_NAME_6);
		goto TAG_ASYNC_CREATE_TEST_FAILED;
	}

	ec = cli->deleteNode(ASYNC_CREATE_PATH_NAME_5);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKNotExist)
	{
		printf("\n[async_create_test] delete path:%s failed! \n", ASYNC_CREATE_PATH_NAME_5);
		goto TAG_ASYNC_CREATE_TEST_FAILED;
	}

	//����not temp, not seq �����//////////////////////////////
	async_create_test_1(ASYNC_CREATE_PATH_NAME_1);

	//����temp, not seq �����////////////////////////////////
	async_create_test_2(ASYNC_CREATE_PATH_NAME_2);

	//����not temp, seq �����////////////////////////////////
	async_create_test_3(ASYNC_CREATE_PATH_NAME_3);

	//����temp, seq �����////////////////////////////////
	async_create_test_4(ASYNC_CREATE_PATH_NAME_4);

	//���� ��������Ľ���У�����㻹δ������Ӧ����ʧ��
	async_create_test_5(ASYNC_CREATE_PATH_NAME_5, ASYNC_CREATE_PATH_NAME_6);

	printf("async_create_test succeed!\n");
	return;

TAG_ASYNC_CREATE_TEST_FAILED:
	printf("\n[async_create_test] failed! \n");
	return;
}



void asyn_createIfNeedCreateParents_test_cb(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, const std::string& value, void* context);

void asyn_createIfNeedCreateParents_test()
{
#define  ASYNC_CREATEIFNEEDCREATEPARENT_PATH_NAME_1     "/async_createIfNeedCreateParents_test_parent"
#define  ASYNC_CREATEIFNEEDCREATEPARENT_PATH_NAME_2     "/async_createIfNeedCreateParents_test_parent/async_createIfNeedCreateParents_test_child"

	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec;
	ZkUtil::ZkErrorCode ec_other;
	bool isTemp = false;
	bool isSeq = false;
	std::string *orignalPath = NULL;

	//Ӧ�� ɾ�� Ҷ�ӽ�㣬��ɾ�� ��֧���
	ec = cli->deleteNode(ASYNC_CREATEIFNEEDCREATEPARENT_PATH_NAME_2);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKNotExist)
	{
		printf("\n[async_createIfNeedCreateParents_test] delete path:%s failed! \n", ASYNC_CREATEIFNEEDCREATEPARENT_PATH_NAME_2);
		goto TAG_ASYNC_CREATEIFNEEDCREATEPARENT_TEST_FAILED;
	}

	ec = cli->deleteNode(ASYNC_CREATEIFNEEDCREATEPARENT_PATH_NAME_1);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKNotExist)
	{
		printf("\n[async_createIfNeedCreateParents_test] delete path:%s failed! \n", ASYNC_CREATEIFNEEDCREATEPARENT_PATH_NAME_1);
		goto TAG_ASYNC_CREATEIFNEEDCREATEPARENT_TEST_FAILED;
	}

	//ֱ�Ӵ���Ҷ�ӽ��
	orignalPath = new std::string(ASYNC_CREATEIFNEEDCREATEPARENT_PATH_NAME_2);
	if (cli->createIfNeedCreateParents(ASYNC_CREATEIFNEEDCREATEPARENT_PATH_NAME_2, "", 
					boost::bind(&asyn_createIfNeedCreateParents_test_cb, _1, _2, _3, _4, _5), 
					orignalPath, isTemp, isSeq) == false)
	{
		delete orignalPath;
		orignalPath = NULL;
		printf("\n[asyn_createIfNeedCreateParents_test] create path failed! path:%s \n", ASYNC_CREATEIFNEEDCREATEPARENT_PATH_NAME_2);
		return;
	}

	return;

TAG_ASYNC_CREATEIFNEEDCREATEPARENT_TEST_FAILED:
	printf("\n[async_createIfNeedCreateParents_test] failed! \n");
	return;
}

void asyn_createIfNeedCreateParents_test_cb(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, const std::string& value, void* context)
{
	assert(errcode == ZkUtil::kZKSucceed);
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	assert(cli == client);

	assert(context != NULL);
	std::string* orignalChildPath = (std::string*) context;
	assert(path == (*orignalChildPath));

	//���� �����ɹ���
	ZkUtil::ZkErrorCode ec_other = cli->isExist(path);
	assert(ec_other == ZkUtil::kZKSucceed);

	delete orignalChildPath;
	orignalChildPath = NULL;

	printf("asyn_createIfNeedCreateParents_test_cb succeed!\n");
}




CountDownLatch *gpSetTestCountDown_1 = NULL;
CountDownLatch *gpSetTestCountDown_2 = NULL;

void asyn_set_test_cb_1(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, int32_t version, void* context);
void asyn_set_test_cb_2(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, int32_t version, void* context);
void asyn_set_test_cb_3(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, int32_t version, void* context);

void asyn_set_test()
{
#define  ASYNC_SET_PATH_NAME     "/async_set_test"

	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	ZkUtil::ZkErrorCode ec;
	ZkUtil::ZkErrorCode ec_other;
	std::string retPath;
	bool isTemp = false;
	bool isSeq = false;
	std::string value1 = "";
	int32_t version1;
	std::string value2 = "";
	int32_t version2;
	std::string value3 = "";
	int32_t version3;
	std::string *setPath = NULL;
	std::string *setPath_sec = NULL;
	std::string *setPath_third = NULL;

	//�������Խ��
	ec = cli->create(ASYNC_SET_PATH_NAME, "", isTemp, isSeq, retPath);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
	{
		printf("\n[async_set_test] create path:%s failed! \n", ASYNC_SET_PATH_NAME);
		goto TAG_ASYNC_SET_TEST_FAILED;
	}

	//��ȡ���Խ���ֵ���汾��
	if (cli->getNode(ASYNC_SET_PATH_NAME, value1, version1) != ZkUtil::kZKSucceed)
	{
		printf("\n[async_set_test] getNode:%s failed! sync_getNode_test failed!\n", ASYNC_GETNODE_PATH_NAME);
		goto TAG_ASYNC_SET_TEST_FAILED;
	}

	//���ý��� �汾�Ų��ԣ�Ӧ����ʧ��
	setPath = new std::string(ASYNC_SET_PATH_NAME);
	if (cli->set(ASYNC_SET_PATH_NAME, "test_value_1", boost::bind(&asyn_set_test_cb_1, _1, _2, _3, _4, _5), setPath, version1 + 1) == false)
	{
		delete setPath;
		setPath = NULL;
		printf("\n[asyn_set_test] set path failed! path:%s \n", ASYNC_SET_PATH_NAME);
		goto TAG_ASYNC_SET_TEST_FAILED;
	}

	//���ý��� �汾����ȷ��Ӧ���سɹ�
	gpSetTestCountDown_1 = new CountDownLatch(1);
	setPath_sec = new std::string(ASYNC_SET_PATH_NAME);
	if (cli->set(ASYNC_SET_PATH_NAME, "test_value_2", boost::bind(&asyn_set_test_cb_2, _1, _2, _3, _4, _5), setPath_sec, version1) == false)
	{
		delete setPath_sec;
		setPath_sec = NULL;
		delete gpSetTestCountDown_1;
		gpSetTestCountDown_1 = NULL;
		printf("\n[asyn_set_test] set path failed! path:%s \n", ASYNC_SET_PATH_NAME);
		goto TAG_ASYNC_SET_TEST_FAILED;
	}

	gpSetTestCountDown_1->wait();

	//��ȡ���Խ���ֵ���汾��
	if (cli->getNode(ASYNC_SET_PATH_NAME, value2, version2) != ZkUtil::kZKSucceed)
	{
		printf("\n[async_set_test] getNode:%s failed! sync_getNode_test failed!\n", ASYNC_SET_PATH_NAME);
		goto TAG_ASYNC_SET_TEST_FAILED;
	}
	assert(value2 == "test_value_2");


	//������汾�ţ���Ĭ�� ��������İ汾��
	gpSetTestCountDown_2 = new CountDownLatch(1);
	setPath_third = new std::string(ASYNC_SET_PATH_NAME);
	if (cli->set(ASYNC_SET_PATH_NAME, "test_value_3", boost::bind(&asyn_set_test_cb_3, _1, _2, _3, _4, _5), setPath_third) == false)
	{
		delete setPath_third;
		setPath_third = NULL;
		delete gpSetTestCountDown_2;
		gpSetTestCountDown_2 = NULL;
		printf("\n[asyn_set_test] set path failed! path:%s \n", ASYNC_SET_PATH_NAME);
		goto TAG_ASYNC_SET_TEST_FAILED;
	}

	gpSetTestCountDown_2->wait();

	//��ȡ���Խ���ֵ���汾��
	if (cli->getNode(ASYNC_SET_PATH_NAME, value3, version3) != ZkUtil::kZKSucceed)
	{
		printf("\n[sync_set_test] getNode:%s failed! sync_getNode_test failed!\n", ASYNC_SET_PATH_NAME);
		goto TAG_ASYNC_SET_TEST_FAILED;
	}
	assert(value3 == "test_value_3");


	delete gpSetTestCountDown_1;
	gpSetTestCountDown_1 = NULL;

	delete gpSetTestCountDown_2;
	gpSetTestCountDown_2 = NULL;

	printf("async_set_test succeed!\n");
	return;
TAG_ASYNC_SET_TEST_FAILED:
	printf("\n[async_set_test] failed! \n");
	return;
}

void asyn_set_test_cb_1(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, int32_t version, void* context)
{
	assert(errcode == ZkUtil::kZKError);
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	assert(cli == client);

	assert(context != NULL);
	std::string* orignalChildPath = (std::string*) context;
	assert(path == (*orignalChildPath));

	assert(version == ZkUtil::kInvalidDataVersion);

	delete orignalChildPath;
	orignalChildPath = NULL;
}

void asyn_set_test_cb_2(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, int32_t version, void* context)
{
	assert(errcode == ZkUtil::kZKSucceed);
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	assert(cli == client);

	assert(context != NULL);
	std::string* orignalChildPath = (std::string*) context;
	assert(path == (*orignalChildPath));

	delete orignalChildPath;
	orignalChildPath = NULL;

	if (gpSetTestCountDown_1)
	{
		gpSetTestCountDown_1->countDown();
	}
}

void asyn_set_test_cb_3(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, int32_t version, void* context)
{
	assert(errcode == ZkUtil::kZKSucceed);
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	assert(cli == client);

	assert(context != NULL);
	std::string* orignalChildPath = (std::string*) context;
	assert(path == (*orignalChildPath));

	delete orignalChildPath;
	orignalChildPath = NULL;

	if (gpSetTestCountDown_2)
	{
		gpSetTestCountDown_2->countDown();
	}
}

CountDownLatch *gpDeleteNodeTestCountDown_1 = NULL;
CountDownLatch *gpDeleteNodeTestCountDown_2 = NULL;
CountDownLatch *gpDeleteNodeTestCountDown_3 = NULL;

void asyn_deleteNode_test_1(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, void* context);
void asyn_deleteNode_test_2(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, void* context);
void asyn_deleteNode_test_3(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, void* context);
void asyn_deleteNode_test_4(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, void* context);
void asyn_deleteNode_test_5(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, void* context);

void asyn_deleteNode_test()
{
#define  ASYNC_DELETENODE_PATH_NAME_1     "/async_deleteNode_test_parent"
#define  ASYNC_DELETENODE_PATH_NAME_2     "/async_deleteNode_test_parent/async_deleteNode_test_child"
#define  ASYNC_DELETENODE_PATH_NAME_3     "/async_deleteNode_test_node"

	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec;
	ZkUtil::ZkErrorCode ec_other;
	bool isTemp = false;
	bool isSeq = false;
	std::string value = "";
	int32_t version = 0;
	std::string *oriPath1 = NULL;
	std::string *oriPath2 = NULL;
	std::string *oriPath3 = NULL;
	std::string *oriPath4 = NULL;
	std::string *oriPath5 = NULL;

	//ֱ�Ӵ���Ҷ�ӽ��
	ec = cli->createIfNeedCreateParents(ASYNC_DELETENODE_PATH_NAME_2, "", isTemp, isSeq, retPath);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
	{
		printf("\n[async_deleteNode_test] create path:%s failed! \n", ASYNC_DELETENODE_PATH_NAME_2);
		goto TAG_ASYNC_DELETENODE_TEST_FAILED;
	}

	ec = cli->createIfNeedCreateParents(ASYNC_DELETENODE_PATH_NAME_3, "", isTemp, isSeq, retPath);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
	{
		printf("\n[async_deleteNode_test] create path:%s failed! \n", ASYNC_DELETENODE_PATH_NAME_3);
		goto TAG_ASYNC_DELETENODE_TEST_FAILED;
	}

	//��ɾ���Ľ���� �����ӽ�㣬��ɾ��ʧ��
	oriPath1 = new std::string(ASYNC_DELETENODE_PATH_NAME_1);
	if (cli->deleteNode(ASYNC_DELETENODE_PATH_NAME_1, boost::bind(&asyn_deleteNode_test_1, _1, _2, _3, _4), oriPath1) == false)
	{
		delete oriPath1;
		oriPath1 = NULL;
		printf("\n[asyn_deleteNode_test] delete path failed! path:%s \n", ASYNC_DELETENODE_PATH_NAME_1);
		goto TAG_ASYNC_DELETENODE_TEST_FAILED;
	}

	//��ɾ���ӽ�㣬��ɾ����֧���
	gpDeleteNodeTestCountDown_1 = new CountDownLatch(1);
	oriPath2 = new std::string(ASYNC_DELETENODE_PATH_NAME_2);
	if (cli->deleteNode(ASYNC_DELETENODE_PATH_NAME_2, boost::bind(&asyn_deleteNode_test_2, _1, _2, _3, _4), oriPath2) == false)
	{
		delete oriPath2;
		oriPath2 = NULL;
		delete gpDeleteNodeTestCountDown_1;
		gpDeleteNodeTestCountDown_1 = NULL;
		printf("\n[asyn_deleteNode_test] delete path failed! path:%s \n", ASYNC_DELETENODE_PATH_NAME_2);
		goto TAG_ASYNC_DELETENODE_TEST_FAILED;
	}

	gpDeleteNodeTestCountDown_1->wait();

	gpDeleteNodeTestCountDown_2 = new CountDownLatch(1);
	oriPath3 = new std::string(ASYNC_DELETENODE_PATH_NAME_1);
	if (cli->deleteNode(ASYNC_DELETENODE_PATH_NAME_1, boost::bind(&asyn_deleteNode_test_3, _1, _2, _3, _4), oriPath3) == false)
	{
		delete oriPath3;
		oriPath3 = NULL;
		delete gpDeleteNodeTestCountDown_2;
		gpDeleteNodeTestCountDown_2 = NULL;
		printf("\n[asyn_deleteNode_test] delete path failed! path:%s \n", ASYNC_DELETENODE_PATH_NAME_1);
		goto TAG_ASYNC_DELETENODE_TEST_FAILED;
	}

	gpDeleteNodeTestCountDown_2->wait();

	//���� ɾ���ɹ���
	ec_other = cli->isExist(ASYNC_DELETENODE_PATH_NAME_1);
	assert(ec_other == ZkUtil::kZKNotExist);

	ec_other = cli->isExist(ASYNC_DELETENODE_PATH_NAME_2);
	assert(ec_other == ZkUtil::kZKNotExist);


	//��ȡ���Խ���ֵ���汾��
	if (cli->getNode(ASYNC_DELETENODE_PATH_NAME_3, value, version) != ZkUtil::kZKSucceed)
	{
		printf("\n[async_deleteNode_test] getNode:%s failed! sync_getNode_test failed!\n", ASYNC_DELETENODE_PATH_NAME_3);
		goto TAG_ASYNC_DELETENODE_TEST_FAILED;
	}

	//��ɾ���Ľ�� �İ汾�Ų��ԣ��򷵻�ʧ��
	oriPath4 = new std::string(ASYNC_DELETENODE_PATH_NAME_3);
	if (cli->deleteNode(ASYNC_DELETENODE_PATH_NAME_3, boost::bind(&asyn_deleteNode_test_4, _1, _2, _3, _4), oriPath4, version + 1) == false)
	{
		delete oriPath4;
		oriPath4 = NULL;
		printf("\n[asyn_deleteNode_test] delete path failed! path:%s \n", ASYNC_DELETENODE_PATH_NAME_3);
		goto TAG_ASYNC_DELETENODE_TEST_FAILED;
	}

	//��ɾ���Ľ�� �İ汾����ȷ����ɾ���ɹ�
	gpDeleteNodeTestCountDown_3 = new CountDownLatch(1);
	oriPath5 = new std::string(ASYNC_DELETENODE_PATH_NAME_3);
	if (cli->deleteNode(ASYNC_DELETENODE_PATH_NAME_3, boost::bind(&asyn_deleteNode_test_5, _1, _2, _3, _4), oriPath5, version) == false)
	{
		delete oriPath5;
		oriPath5 = NULL;
		delete gpDeleteNodeTestCountDown_3;
		gpDeleteNodeTestCountDown_3 = NULL;
		printf("\n[asyn_deleteNode_test] delete path failed! path:%s \n", ASYNC_DELETENODE_PATH_NAME_3);
		goto TAG_ASYNC_DELETENODE_TEST_FAILED;
	}

	gpDeleteNodeTestCountDown_3->wait();

	//���� ɾ���ɹ���
	ec_other = cli->isExist(SYNC_DELETENODE_PATH_NAME_3);
	assert(ec_other == ZkUtil::kZKNotExist);


	delete gpDeleteNodeTestCountDown_1;
	gpDeleteNodeTestCountDown_1 = NULL;
	delete gpDeleteNodeTestCountDown_2;
	gpDeleteNodeTestCountDown_2 = NULL;
	delete gpDeleteNodeTestCountDown_3;
	gpDeleteNodeTestCountDown_3 = NULL;
	printf("async_deleteNode_test succeed!\n");
	return;

TAG_ASYNC_DELETENODE_TEST_FAILED:
	printf("\n[async_deleteNode_test] failed! \n");
	return;
}

void asyn_deleteNode_test_1(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, void* context)
{
	assert(errcode == ZkUtil::kZKNotEmpty);
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	assert(cli == client);

	assert(context != NULL);
	std::string* orignalChildPath = (std::string*) context;
	assert(path == (*orignalChildPath));

	delete orignalChildPath;
	orignalChildPath = NULL;
}

void asyn_deleteNode_test_2(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, void* context)
{
	assert(errcode == ZkUtil::kZKSucceed);
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	assert(cli == client);

	assert(context != NULL);
	std::string* orignalChildPath = (std::string*) context;
	assert(path == (*orignalChildPath));

	delete orignalChildPath;
	orignalChildPath = NULL;

	if (gpDeleteNodeTestCountDown_1)
	{
		gpDeleteNodeTestCountDown_1->countDown();
	}
}

void asyn_deleteNode_test_3(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, void* context)
{
	assert(errcode == ZkUtil::kZKSucceed);
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	assert(cli == client);

	assert(context != NULL);
	std::string* orignalChildPath = (std::string*) context;
	assert(path == (*orignalChildPath));

	delete orignalChildPath;
	orignalChildPath = NULL;

	if (gpDeleteNodeTestCountDown_2)
	{
		gpDeleteNodeTestCountDown_2->countDown();
	}
}

void asyn_deleteNode_test_4(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, void* context)
{
	assert(errcode == ZkUtil::kZKError);
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	assert(cli == client);

	assert(context != NULL);
	std::string* orignalChildPath = (std::string*) context;
	assert(path == (*orignalChildPath));

	delete orignalChildPath;
	orignalChildPath = NULL;
}

void asyn_deleteNode_test_5(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, void* context)
{
	assert(errcode == ZkUtil::kZKSucceed);
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	assert(cli == client);

	assert(context != NULL);
	std::string* orignalChildPath = (std::string*) context;
	assert(path == (*orignalChildPath));

	delete orignalChildPath;
	orignalChildPath = NULL;

	if (gpDeleteNodeTestCountDown_3)
	{
		gpDeleteNodeTestCountDown_3->countDown();
	}
}



CountDownLatch *gpDeleteRecursiveTestCountDown = NULL;

void asyn_deleteRecursive_test(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, void* context);

void asyn_deleteRecursive_test()
{
#define  ASYNC_DELETERECURISIVE_PATH_NAME_1     "/async_deleteRecursive_test_parent"
#define  ASYNC_DELETERECURISIVE_PATH_NAME_2     "/async_deleteRecursive_test_parent/async_deleteRecursive_test_child"

	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec;
	ZkUtil::ZkErrorCode ec_other;
	bool isTemp = false;
	bool isSeq = false;
	std::string value = "";
	int32_t version = 0;
	std::string *oriPath = NULL;

	//ֱ�Ӵ���Ҷ�ӽ��
	ec = cli->createIfNeedCreateParents(ASYNC_DELETERECURISIVE_PATH_NAME_2, "", isTemp, isSeq, retPath);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
	{
		printf("\n[async_deleteNode_test] create path:%s failed! \n", ASYNC_DELETERECURISIVE_PATH_NAME_2);
		goto TAG_ASYNC_DELETERECRUSIVE_TEST_FAILED;
	}

	//��ɾ���Ľ�� �İ汾����ȷ����ɾ���ɹ�
	gpDeleteRecursiveTestCountDown = new CountDownLatch(1);
	oriPath = new std::string(ASYNC_DELETERECURISIVE_PATH_NAME_1);
	if (cli->deleteRecursive(ASYNC_DELETERECURISIVE_PATH_NAME_1, boost::bind(&asyn_deleteRecursive_test, _1, _2, _3, _4), oriPath) == false)
	{
		delete oriPath;
		oriPath = NULL;
		delete gpDeleteRecursiveTestCountDown;
		gpDeleteRecursiveTestCountDown = NULL;
		printf("\n[asyn_deleteNode_test] delete path failed! path:%s \n", ASYNC_DELETERECURISIVE_PATH_NAME_1);
		goto TAG_ASYNC_DELETERECRUSIVE_TEST_FAILED;
	}

	gpDeleteRecursiveTestCountDown->wait();

	//���� ɾ���ɹ���
	ec_other = cli->isExist(ASYNC_DELETERECURISIVE_PATH_NAME_1);
	assert(ec_other == ZkUtil::kZKNotExist);

	ec_other = cli->isExist(ASYNC_DELETERECURISIVE_PATH_NAME_2);
	assert(ec_other == ZkUtil::kZKNotExist);

	delete gpDeleteRecursiveTestCountDown;
	gpDeleteRecursiveTestCountDown = NULL;

	printf("async_deleteRecursive_test succeed!\n");
	return;

TAG_ASYNC_DELETERECRUSIVE_TEST_FAILED:
	printf("\n[async_deleteRecursive_test] failed! \n");
	return;
}

void asyn_deleteRecursive_test(ZkUtil::ZkErrorCode errcode, const ZkClientPtr& client, const std::string& path, void* context)
{
	assert(errcode == ZkUtil::kZKSucceed);
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	assert(cli == client);

	assert(context != NULL);
	std::string* orignalChildPath = (std::string*) context;
	assert(path == (*orignalChildPath));

	delete orignalChildPath;
	orignalChildPath = NULL;

	if (gpDeleteRecursiveTestCountDown)
	{
		gpDeleteRecursiveTestCountDown->countDown();
	}
}



void regNodeWatcher_cancelRegNodeWatcher_test_cb(ZkUtil::ZkNotifyType type, const ZkClientPtr& client,
	const std::string& path, const std::string& value, int32_t version, void* context);

void regNodeWatcher_cancelRegNodeWatcher_test()
{
#define  NODEWATCHER_PATH_NAME     "/regNodeWatcher_test_node"
	ZkUtil::ZkErrorCode ec;
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);

	//ɾ�����
	ec = cli->deleteNode(NODEWATCHER_PATH_NAME);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKNotExist)
	{
		printf("\n[regNodeWatcher_cancelRegNodeWatcher_test] delete path:%s failed! ec:%d \n", NODEWATCHER_PATH_NAME,ec);
		return;
	}

	//ע��watcher
	std::string *oriPath = new std::string(NODEWATCHER_PATH_NAME);
	if (cli->regNodeWatcher(NODEWATCHER_PATH_NAME, 
							boost::bind(&regNodeWatcher_cancelRegNodeWatcher_test_cb, _1, _2, _3, _4, _5, _6), oriPath) == false)
	{
		printf("\n[regNodeWatcher_cancelRegNodeWatcher_test] regNodeWatcher failed! path:%s\n", NODEWATCHER_PATH_NAME);
		return;
	}

	printf("[regNodeWatcher_cancelRegNodeWatcher_test] create node.\n");
	bool isTemp = false;
	bool isSeq = false;
	std::string retPath;
	ec = cli->create(NODEWATCHER_PATH_NAME, "test_value_create", isTemp, isSeq, retPath);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
	{
		printf("\n[regNodeWatcher_cancelRegNodeWatcher_test] create path:%s failed! \n", NODEWATCHER_PATH_NAME);
		return;
	}

	sleep(10);

	printf("[regNodeWatcher_cancelRegNodeWatcher_test] set node.\n");
	ec = cli->set(NODEWATCHER_PATH_NAME, "test_value_set");
	if (ec != ZkUtil::kZKSucceed)
	{
		printf("\n[regNodeWatcher_cancelRegNodeWatcher_test] set path:%s failed! \n", NODEWATCHER_PATH_NAME);
		return;
	}

	sleep(10);

	printf("[regNodeWatcher_cancelRegNodeWatcher_test] delete node.\n");
	ec = cli->deleteNode(NODEWATCHER_PATH_NAME);
	if (ec != ZkUtil::kZKSucceed)
	{
		printf("\n[regNodeWatcher_cancelRegNodeWatcher_test] delete path:%s failed! \n", NODEWATCHER_PATH_NAME);
		return;
	}

	printf("cancel watcher... path:%s\n", NODEWATCHER_PATH_NAME);
	sleep(5);

	//ȡ��ע��
	cli->cancelRegNodeWatcher(NODEWATCHER_PATH_NAME);

	sleep(5);

	//�Ͳ���֪ͨ��
	printf("[regNodeWatcher_cancelRegNodeWatcher_test] create node, don't watcher.\n");
	ec = cli->create(NODEWATCHER_PATH_NAME, "test_value_create_after", isTemp, isSeq, retPath);
	if (ec != ZkUtil::kZKSucceed && ec != ZkUtil::kZKExisted)
	{
		printf("\n[regNodeWatcher_cancelRegNodeWatcher_test] create path:%s failed! \n", NODEWATCHER_PATH_NAME);
		return;
	}
}


void regNodeWatcher_cancelRegNodeWatcher_test_cb(ZkUtil::ZkNotifyType type, const ZkClientPtr& client,
	const std::string& path, const std::string& value, int32_t version, void* context)
{
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	assert(client == cli);

	assert(context != NULL);
	std::string *oriPath = (std::string*) context;
	assert(path == (*oriPath));

	printf("[regNodeWatcher_cancelRegNodeWatcher_test_cb] notifyType:%d,%s, value:%s, version:%d \n",
			type, getNotifyTypeStr(type).c_str(), value.c_str(), version);
}






void regChildWatcher_cancelRegChildWatcher_test_cb(ZkUtil::ZkNotifyType type, const ZkClientPtr& client,
	const std::string& path, const std::vector<std::string>& childNodes, void* context);

void regChildWatcher_cancelRegChildWatcher_test()
{
#define  REGCHILDWATCH_PARENT_NAME     "/regChildWatcher_parent"
#define  REGCHILDWATCH_CHILD_NAME    "regChildWatcher_child"
#define  REGCHILDWATCH_CHILD_NUM      3

	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);

	//ɾ�����
	ZkUtil::ZkErrorCode ec_other = cli->deleteRecursive(REGCHILDWATCH_PARENT_NAME);
	assert(ec_other == ZkUtil::kZKSucceed);

	//��������û�д�����ע��watcher ��ʱ��ע��ʧ��
	std::string *oriPath = new std::string(REGCHILDWATCH_PARENT_NAME);
	if (cli->regChildWatcher(REGCHILDWATCH_PARENT_NAME, 
		boost::bind(&regChildWatcher_cancelRegChildWatcher_test_cb, _1, _2, _3, _4, _5), oriPath) == false)
	{
		printf("\n[regNodeWatcher_cancelRegNodeWatcher_test] regChildWatcher failed! path:%s\n", REGCHILDWATCH_PARENT_NAME);
	}

	printf("create parent node, don't call watcher...\n");
	sleep(5);

	//���������
	std::string retPath = "";
	ZkUtil::ZkErrorCode ec = cli->create(REGCHILDWATCH_PARENT_NAME, "", false, false, retPath);
	if (ec != ZkUtil::kZKSucceed)
	{
		printf("\n[regNodeWatcher_cancelRegNodeWatcher_test] create path:%s failed! \n", REGCHILDWATCH_PARENT_NAME);
		return;
	}

	//ע��watcher ��λ�ɹ�
	if (cli->regChildWatcher(REGCHILDWATCH_PARENT_NAME, 
		boost::bind(&regChildWatcher_cancelRegChildWatcher_test_cb, _1, _2, _3, _4, _5), oriPath) == false)
	{
		printf("\n[regNodeWatcher_cancelRegNodeWatcher_test] regChildWatcher failed! path:%s\n", REGCHILDWATCH_PARENT_NAME);
	}
	else
	{
		printf("\n[regNodeWatcher_cancelRegNodeWatcher_test] regChildWatcher succeed! path:%s\n", REGCHILDWATCH_PARENT_NAME);
	}

	//���� �ӽ��
	std::vector<std::string> childNodes;
	for (int i = 0; i < REGCHILDWATCH_CHILD_NUM; i++)
	{
		char path[100] = {0};
		char childName[100] = {0};
		snprintf(path, sizeof(path), "%s/%s_%d", REGCHILDWATCH_PARENT_NAME, REGCHILDWATCH_CHILD_NAME, i);

		printf("create child node(i=%d), call watcher...\n", i);
		sleep(5);

		ZkUtil::ZkErrorCode ec_chd = cli->create(path, "", false, false, retPath);
		if (ec_chd != ZkUtil::kZKSucceed)
		{
			printf("\n[regNodeWatcher_cancelRegNodeWatcher_test] create path:%s failed! \n", path);
			return;
		}
	}

	printf("delete child node(i=%d), call watcher...\n", 0);
	sleep(5);

	//ɾ���ӽ��
	char path[100] = {0};
	char childName[100] = {0};
	snprintf(path, sizeof(path), "%s/%s_%d", REGCHILDWATCH_PARENT_NAME, REGCHILDWATCH_CHILD_NAME, 0);
	ZkUtil::ZkErrorCode ec_chd = cli->deleteNode(path);
	if (ec_chd != ZkUtil::kZKSucceed && ec_chd != ZkUtil::kZKNotExist)
	{
		printf("\n[regNodeWatcher_cancelRegNodeWatcher_test] create path:%s failed! \n", path);
		return;
	}

//    //ɾ�� ����� => ��ʱ��ص������ kGetChildListFailed_ParentNotExist��
//	  //zkclient����ע��watcher, �� �������ɾ���� ��ע��ʧ�ܡ�zkclient���ٹ�5���Ӻ� ������ע��.
//    ec_other = cli->deleteRecursive(REGCHILDWATCH_PARENT_NAME);
//    assert(ec_other == ZkUtil::kZKSucceed);

	printf("set child node(i=%d) value, don't call watcher...\n", 1);
	sleep(5);
	//�޸��ӽ���ֵ������֪ͨwatcher
	snprintf(path, sizeof(path), "%s/%s_%d", REGCHILDWATCH_PARENT_NAME, REGCHILDWATCH_CHILD_NAME, 1);
	ec_chd = cli->set(path, "child_change_value");
	if (ec_chd != ZkUtil::kZKSucceed)
	{
		printf("\n[regNodeWatcher_cancelRegNodeWatcher_test] set path:%s failed! \n", path);
		return;
	}

	printf("cancel child watcher... path:%s\n", REGCHILDWATCH_PARENT_NAME);
	sleep(5);

	//ȡ��ע��
	cli->cancelRegChildWatcher(REGCHILDWATCH_PARENT_NAME);

	sleep(5);

	//ɾ���ӽ�� �Ͳ���֪ͨ��
	printf("delete child node(i=%d), don't call watcher...\n", 2);
	snprintf(path, sizeof(path), "%s/%s_%d", REGCHILDWATCH_PARENT_NAME, REGCHILDWATCH_CHILD_NAME, 2);
	ec_chd = cli->deleteNode(path);
	if (ec_chd != ZkUtil::kZKSucceed && ec_chd != ZkUtil::kZKNotExist)
	{
		printf("\n[regNodeWatcher_cancelRegNodeWatcher_test] create path:%s failed! \n", path);
		return;
	}

	printf("create child node(i=%d), don't call watcher...\n", 5);
	sleep(5);

	//�����ӽ�� �Ͳ���֪ͨ��
	snprintf(path, sizeof(path), "%s/%s_%d", REGCHILDWATCH_PARENT_NAME, REGCHILDWATCH_CHILD_NAME, 5);
	ec_chd = cli->create(path, "", false, false, retPath);
	if (ec_chd != ZkUtil::kZKSucceed)
	{
		printf("\n[regNodeWatcher_cancelRegNodeWatcher_test] create path:%s failed! \n", path);
		return;
	}
}

void regChildWatcher_cancelRegChildWatcher_test_cb(ZkUtil::ZkNotifyType type, const ZkClientPtr& client,
	const std::string& path, const std::vector<std::string>& childNodes, void* context)
{
	ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
	assert(client == cli);

	assert(context != NULL);
	std::string *oriPath = (std::string*) context;
	assert(path == (*oriPath));


	printf("[regChildWatcher_cancelRegChildWatcher_test_cb] notifyType:%d,%s, parentPath:%s \n",
		type, getNotifyTypeStr(type).c_str(), path.c_str());

	printChild(path, childNodes);
}


int main()
{
    //����zookeeper��־·��
    if (ZkClientManager::setLogConf(true, "./zk_log") == false)
    {
		printf("setLogConf failed!\n\n");
    }

    //����һ��session
    uint32_t handle = ZkClientManager::instance().createZkClient(ZOOKEEPER_SERVER_CONN_STRING, 30000, NULL, NULL, NULL);
    if (handle == 0)
    {
		printf("create session failed! connStr:%s\n", ZOOKEEPER_SERVER_CONN_STRING);
		return 0;
    }

	gZkClientHandle = handle;

    //ͨ��session handle����ȡZkClient
    ZkClientPtr cli = ZkClientManager::instance().getZkClient(gZkClientHandle);
//	getHandle_test(gZkClientHandle, cli);
//
//	getClientId_test();
//
//	sync_getNode_test();
//
//	sync_getChildren_test();
//
//	sync_isExist_test();
//
//	sync_create_test();
//
//	sync_createIfNeedCreateParents_test();
//
//	sync_set_test();
//
//	sync_deleteNode_test();
//
//	sync_deleteRecursive_test();
//
//	asyn_getNode_test();
//
//	asyn_getChildren_test();
//
//	asyn_isExist_test();
//
//	asyn_create_test();
//
//	asyn_createIfNeedCreateParents_test();
//
//	asyn_set_test();
//
//	asyn_deleteNode_test();
//
//	asyn_deleteRecursive_test();
//
//	regNodeWatcher_cancelRegNodeWatcher_test();
//
//	regChildWatcher_cancelRegChildWatcher_test();

	sync_create_seq_no_test();
	sleep(UINT_MAX);

    return  0;
}








