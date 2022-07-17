#include<stdio.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<signal.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<limits.h>
#include <sys/types.h>
#include <sys/ipc.h>

static int g_reqid=-1;//request 请求消息队列ID初值，初值为-1时，说明消息队列未启动
static int g_resid=-1;//respond 响应消息队列ID初值

typedef struct tag_Account{//存放账户信息的结构体
	int    id;//账号
	char   name[256];//账户名
	char   passwd[6];//账户密码
	double balance;//账户金额
}ACCOUNT;

typedef struct tag_Service{
	char srv_path[256];//存放六个服务端的路径
	pid_t srv_pid;//存放六个服务端的进程号
}SERVICE;//结构体重命名

static SERVICE g_srv[]={ //存放六个进程路径的数组。第一个为路径，第二个为进程号，当进程号（返回值）为-1时，说明进程未启动
	{"./open",		-1},//开户
	{"./close",		-1},//销户
	{"./save",		-1},//存款
	{"./withdraw",	-1},//取款
	{"./query",		-1},//查询
	{"./transfer",	-1}//转账
};
	
int init(void)//用msgget函数创建两个消息队列。一个用来存放客户端发来的请求；一个存放服务端回复客户端的消息
{
	key_t KEY_REQUEST=ftok("/home",2);//客户端键值        C语言初始化一个全局变量或static变量时，只能用常量赋值，不能用变量赋值！
	key_t KEY_RESPOND=ftok("/home",3);//服务端键值
	
	printf("服务器初始化...\n");
	if((g_reqid=msgget(KEY_REQUEST,IPC_CREAT|IPC_EXCL|0777))==-1)//如果消息队列不存在，则创建，返回消息队列的ID；如果消息对列存在，则返回-1
	{
		perror("msgget");//消息队列创建失败，返回-1
		return -1;
	}
	printf("创建请求消息队列成功！\n");
	if((g_resid=msgget(KEY_RESPOND,IPC_CREAT|IPC_EXCL|0777))==-1)
	{
		perror("msgget");
		return -1;
	}
	printf("创建响应消息队列成功！\n");
	
	return 0;
}

void deinit(void)//删除两个消息队列
{
	printf("服务器关闭....\n");
	if(msgctl(g_reqid,IPC_RMID,NULL)==-1)//删除消息队列，消息队列的ID，成功返回0,失败返回-1
		perror("msgctl");
	else
		printf("销毁请求消息队列成功！\n");
	if(msgctl(g_resid,IPC_RMID,NULL)==-1)
		perror("msgctl");
	else
		printf("销毁响应消息队列成功！\n");
}

int start(void)//运行六个业务的进程
{
	printf("启动业务服务...\n");                 
	size_t i;
	for(i=0;i<sizeof(g_srv)/sizeof(g_srv[0]);i++)//总数据类型的大小除以某一成员数据类型的大小，就是数组成员的个数，为6。循环6次，打开六个业务进程
	{
		if((g_srv[i].srv_pid=vfork())==-1)//vfork如果创建子进程失败，则返回-1,然后赋值给要打开进程的进程号，说明该进程打开失败
		{
			perror("vfork");
			return -1;
		}
		if(g_srv[i].srv_pid==0)//如果创建进程的返回值为0,说明子进程创建成功
		{
			if(execl(g_srv[i].srv_path,g_srv[i].srv_path,NULL)==-1)//让子进程重生
			{		perror("execl");
				return -1;
			}
			return 0;
		}
	}
	return 0;
}


int stop (void)//关闭六个业务进程
{
	printf ("停止业务服务....\n");            
	size_t i;
	for(i=0;i<sizeof(g_srv)/sizeof(g_srv[0]);i++)//获取服务进程的个数，即有六个服务进程
	{
		if(kill(g_srv[i].srv_pid,SIGINT)==-1)//给每个进程发送SIGINT信号，杀死进程，如果返回值为-1,则发送失败
		{
			perror("kill");
			return -1;
		}
	}
	for(;;)
	{
		if(wait(NULL)==-1)//等待六个子进程结束，帮子进程收尸，参数为进程结束的状态，如果返回值为-1,则等待失败
		{
			perror("wait");
			return -1;
		}
		break;//一直等所有的子进程运行结束，才退出
	}
	return 0;
}


void sigint(int signum)//参数为要捕捉的信号，当键入一个中断信号后，会被signal函数捕捉，然后调用sigint函数
{
    printf("%d\n", signum);
    stop();//调用stop函数，关闭六个进程

    exit(0);//进程退出，正常结束一个进程
}


int main(void)
{
	atexit(deinit);//钩子函数，退出清理函数，主函数即将结束时，调用deinit函数，删除两个消息队列
	signal(SIGINT,sigint);//捕捉一个信号并做处理。捕捉一个中断信号(2号信号，即ctrl+c,杀死进程)，调用sigint函数
	if(init()==-1)//如果返回值为-1,说明消息队列创建失败
		return -1;
	if(start()==-1)//如果返回值为-1,说明六个进程启动失败
		return -1;
	sleep(1);
	printf("按Enter键退出！\n");
	getchar();//按下enter键退出
	if(stop()==-1)//如果返回值为-1,说明关闭进程失败
		return -1;

	return 0;
}

