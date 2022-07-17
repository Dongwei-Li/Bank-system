#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define TYPE_OPEN 11
#define TYPE_CLOSE 12
#define TYPE_SAVE 13   
#define TYPE_WITHDRAW 14
#define TYPE_QUERY 15
#define TYPE_TRANSFER 16//消息类型

static int g_reqid = -1;//请求消息队列ID初值
static int g_resid = -1;//响应消息队列ID初值

typedef struct tag_OpenRequest{//开户请求
	long   type;
	pid_t  pid;
	char   name[256];
	char   passwd[6];
	double balance;
}OPEN_REQUEST;

typedef struct tag_OpenRespond{//开户响应
	long type;
	char error[512];
	int  id;
}OPEN_RESPOND;

typedef struct tag_CloseRequest{//销户请求
	long   type;
	pid_t  pid;
	int    id;
	char   name[256];
	char   passwd[6];
}CLOSE_REQUEST;

typedef struct tag_CloseRespond{//销户响应
	long   type;
	char   error[512];
	double balance;
}CLOSE_RESPOND;

typedef struct tag_SaveRequest{//存款请求
	long   type;
	pid_t  pid;
	int    id;
	char   name[256];
	double money;
}SAVE_REQUEST;

typedef struct tag_SaveRespond{//存款响应
	long   type;
	char   error[512];
	double balance;
}SAVE_RESPOND;

typedef struct tag_WithdrawRequest{
	long   type;
	pid_t  pid;
	int    id;
	char   name[256];
	char   passwd[9];
	double money;
}WITHDRAW_REQUEST;

typedef struct tag_WithdrawRespond{
	long   type;
	char   error[512];
	double balance;
}WITHDRAW_RESPOND;

typedef struct tag_QueryRequest{
	long   type;
	pid_t  pid;
	int    id;
	char   name[256];
	char   passwd[9];
}QUERY_REQUEST;

typedef struct tag_QueryRespond{
	long   type;
	char   error[512];
	double balance;
}QUERY_RESPOND;

typedef struct tag_TransferRequest{
	long   type;
	pid_t  pid;
	int    id[2];
	char   name[2][256];
	char   passwd[9];
	double money;
}TRANSFER_REQUEST;

typedef struct tag_TransferRespond{
	long   type;
	char   error[512];
	double balance;
}TRANSFER_RESPOND;

void mmenu(void) 
{
	printf ("***欢迎来到中国银行***\n");
	printf ("      1. 开户         \n");
	printf ("      2. 销户         \n");
	printf ("      3. 存款         \n");
	printf ("      4. 取款         \n");
	printf ("      5. 查询         \n");
	printf ("      6. 转账         \n");
	printf ("******0. 退出*********\n");
	printf ("请选择你要操作的序号：");

	return;
}

int mquit(void)//退出银行系统
{
	printf ("欢迎下次使用！\n");
	return 0;
}

int mopen(void)//开户
{
	pid_t pid = getpid();//获取进程号
	OPEN_REQUEST req = {TYPE_OPEN, pid};//开户客户端：1类消息，进程号
	printf ("账户名：");
	scanf ("%s",req.name);
	printf ("密码：");
	scanf ("%s",req.passwd);
	printf ("存款金额：");
	scanf ("%lf", &req.balance);

	if (msgsnd(g_reqid,&req,sizeof(req)-sizeof(req.type),0) == -1)//向客户端消息队列写入内容 
	{
		perror ("msgsnd");
		return 0;
	}

	OPEN_RESPOND res;//开户服务端结构体

	if (msgrcv(g_resid,&res,sizeof(res)-sizeof(res.type),pid,0)== -1)//服务端消息队列获取内容 
	{
		perror ("msgrcv");
		return 0;
	}

	printf ("账号：%d\n", res.id);

	return 0;
}

int mclose(void)//销户 
{
	key_t KEY_REQUEST=ftok("/home",2);//客户端键值        C语言初始化一个全局变量或static变量时，只能用常量赋值，不能用变量赋值！
	key_t KEY_RESPOND=ftok("/home",3);//服务端键值
	
	pid_t pid = getpid ();//获取进程号
	CLOSE_REQUEST req = {TYPE_CLOSE, pid};

	printf ("账号：");
	scanf ("%d", &req.id);
	printf ("账户名：");
	scanf ("%s", req.name);
	printf ("密码：");
	scanf ("%s", req.passwd);

	if (msgsnd(g_reqid,&req,sizeof(req)-sizeof(req.type),0)== -1)//向消息队列发送销户请求 
	{
		perror ("msgsnd");
		return 0;
	}

	CLOSE_RESPOND res;

	if (msgrcv(g_resid,&res,sizeof(res)-sizeof(res.type),pid,0)== -1) 
	{
		perror ("msgrcv");
		return 0;
	}

	printf ("账户余额：%.2lf\n", res.balance);

	return 0;
}

int msave(void)//存款 
{
	pid_t pid = getpid ();
	SAVE_REQUEST req = {TYPE_SAVE, pid};

	printf ("账号：");
	scanf ("%d", &req.id);
	printf ("账户名：");
	scanf ("%s", req.name);
	printf ("存款金额：");
	scanf ("%lf", &req.money);

	if (msgsnd(g_reqid,&req,sizeof(req)-sizeof(req.type),0)== -1) 
	{
		perror ("msgsnd");
		return 0;
	}

	SAVE_RESPOND res;

	if (msgrcv(g_resid,&res,sizeof(res)-sizeof(res.type),pid,0)== -1) 
	{
		perror ("msgrcv");
		return 0;
	}

	printf ("账户余额：%.2lf\n", res.balance);

	return 0;
}

int mwithdraw(void) //取款
{
	pid_t pid = getpid ();
	WITHDRAW_REQUEST req = {TYPE_WITHDRAW, pid};

	printf ("账号：");
	scanf ("%d", &req.id);
	printf ("账户名：");
	scanf ("%s", req.name);
	printf ("密码：");
	scanf ("%s", req.passwd);
	printf ("取款金额：");
	scanf ("%lf", &req.money);

	if (msgsnd (g_reqid, &req, sizeof (req) - sizeof (req.type),0) == -1) 
	{
		perror ("msgsnd");
		return 0;
	}

	WITHDRAW_RESPOND res;

	if (msgrcv (g_resid, &res, sizeof (res) - sizeof (res.type),pid, 0) == -1) 
	{
		perror ("msgrcv");
		return 0;
	}

	if (strlen (res.error)) 
	{
		printf ("%s\n", res.error);
		return 0;
	}

	printf ("账户余额：%.2lf\n", res.balance);

	return 0;
}

int mquery(void) //查询
{
	pid_t pid = getpid ();
	QUERY_REQUEST req = {TYPE_QUERY, pid};

	printf ("账号：");
	scanf ("%d", &req.id);
	printf ("账户名：");
	scanf ("%s", req.name);
	printf ("密码：");
	scanf ("%s", req.passwd);

	if (msgsnd (g_reqid, &req, sizeof (req) - sizeof (req.type),0) == -1) 
	{
		perror ("msgsnd");
		return 0;
	}

	QUERY_RESPOND res;

	if (msgrcv (g_resid, &res, sizeof (res) - sizeof (res.type),pid, 0) == -1) 
	{
		perror ("msgrcv");
		return 0;
	}

	if (strlen (res.error)) 
	{
		printf ("%s\n", res.error);
		return 0;
	}

	printf ("账户余额：%.2lf\n", res.balance);

	return 0;
}

int mtransfer (void) //转账
{
	pid_t pid = getpid ();
	TRANSFER_REQUEST req = {TYPE_TRANSFER, pid};

	printf ("己方账号：");
	scanf ("%d", &req.id[0]);
	printf ("己方账户名：");
	scanf ("%s", req.name[0]);
	printf ("己方密码：");
	scanf ("%s", req.passwd);
	printf ("转账金额：");
	scanf ("%lf", &req.money);
	printf ("对方账号：");
	scanf ("%d", &req.id[1]);
	printf ("对方账户名：");
	scanf ("%s", req.name[1]);

	if (msgsnd (g_reqid, &req, sizeof (req) - sizeof (req.type),0) == -1) 
	{
		perror ("msgsnd");
		return 0;
	}

	TRANSFER_RESPOND res;

	if (msgrcv (g_resid, &res, sizeof (res) - sizeof (res.type),pid, 0) == -1) 
	{
		perror ("msgrcv");
		return 0;
	}

	if (strlen (res.error)) 
	{
		printf ("%s\n", res.error);
		return 0;
	}

	printf ("己方账户余额：%.2lf\n", res.balance);

	return 0;
}

int main(void) 
{
	key_t KEY_REQUEST=ftok("/home",2);//客户端键值        C语言初始化一个全局变量或static变量时，只能用常量赋值，不能用变量赋值！
	key_t KEY_RESPOND=ftok("/home",3);//服务端键值
	
	
	if ((g_reqid = msgget (KEY_REQUEST, IPC_CREAT|0777)) == -1) 
	{
		perror ("msgget");
		return -1;
	}

	if ((g_resid = msgget (KEY_RESPOND, IPC_CREAT|0777)) == -1) 
	{
		perror ("msgget");
		return -1;
	}
	while(1)
	{
		mmenu();
		int a;
		scanf("%d",&a);
		switch(a)
		{
			case 1:mopen();break;
			case 2:mclose();break;
			case 3:msave();break;
			case 4:mwithdraw();break;
			case 5:mquery();break;
			case 6:mtransfer();break;
			case 0:mquit();return 0;
			default:printf("输入错误，请重新输入！\n");
		}
	}
	return 0;
}

