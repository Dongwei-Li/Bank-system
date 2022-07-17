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

#define TYPE_WITHDRAW 14

typedef struct tag_Account{//存放账户信息的结构体
	int    id;//账号
	char   name[256];//账户名
	char   passwd[6];//账户密码
	double balance;//账户金额
}ACCOUNT;

typedef struct tag_WithdrawRequest{//取款请求
	long   type;
	pid_t  pid;
	int    id;
	char   name[256];
	char   passwd[9];
	double money;
}WITHDRAW_REQUEST;

typedef struct tag_WithdrawRespond{//取款响应
	long   type;
	char   error[512];
	double balance;
}WITHDRAW_RESPOND;

int get (int id, ACCOUNT* acc) 
{
	char pathname[256];
	sprintf (pathname, "%d.acc", id);

	int fd = open (pathname, O_RDONLY);
	if (fd == -1) 
	{
		perror ("open");
		return -1;
	}

	if (read (fd, acc, sizeof (*acc)) == -1) 
	{
		perror ("read");
		return -1;
	}
	close (fd);
	return 0;
}

int update (const ACCOUNT* acc) //更新账户信息
{
	char pathname[256];//账户文件
	sprintf (pathname, "%d.acc", acc -> id);
	
	int fd = open (pathname, O_WRONLY);//只写的方式打开账户文件，获取文件流指针
	if (fd == -1) 
	{
		perror ("open");//打开文件失败
		return -1;
	}

	if (write (fd, acc, sizeof (*acc)) == -1) //向文件中写入内容
	{
		perror ("write");//写入内容失败
		return -1;
	}
	close (fd);//关闭文件
	return 0;
}

void sigint (int signum) //等待服务器发送信息，结束取款
{
	printf ("取款服务：即将停止...\n");//提示取款进程要结束了
	exit (0);//杀死自己
}

int main (void) 
{
	key_t KEY_REQUEST=ftok("/home",2);//客户端键值        C语言初始化一个全局变量或static变量时，只能用常量赋值，不能用变量赋值！
	key_t KEY_RESPOND=ftok("/home",3);//服务端键值
	
	if (signal (SIGINT, sigint) == SIG_ERR) //捕捉到2号信号后，执行sigint函数，结束取款进程
	{
		perror ("signal");//失败返回报错信息
		return -1;
	}

	int reqid = msgget (KEY_REQUEST, IPC_CREAT|0777);//获取请求消息队列的ID号
	if (reqid == -1) 
	{
		perror ("msgget");
		return -1;
	}

	int resid = msgget (KEY_RESPOND, IPC_CREAT|0777);//获取响应消息队列的ID号
	if (resid == -1) 
	{
		perror ("msgget");
		return -1;
	}

	printf ("取款服务：启动就绪。\n");

	while(1) //接收消息
	{
		WITHDRAW_REQUEST req;//取款客户端结构体

		if (msgrcv (reqid, &req, sizeof (req) - sizeof (req.type),TYPE_WITHDRAW, 0) == -1) //接收客户端请求信息。TYPE_CLOSE消息类型
		{
			perror ("msgrcv");
			continue;//本次接收不到，就跳过，继续接收
		}

		WITHDRAW_RESPOND res = {req.pid, ""};//响应取款结构体
		ACCOUNT acc;//账户结构体

		if (get (req.id, &acc) == -1) //通过ID号获取账户信息，否则就是无效账号
		{
			sprintf (res.error, "无效账号！");
			goto send_respond;
		}

		if (strcmp (req.name, acc.name)) //比较服务端的用户名与要销户的用户名是否一致，否则就是无效用户名
		{
			sprintf (res.error, "无效户名！");
			goto send_respond;
		}

		if (strcmp (req.passwd, acc.passwd)) //比较密码信息是否一致，服务端保存的密码与用户输入的密码间比较
		{
			sprintf (res.error, "密码错误！");
			goto send_respond;
		}

		if (req.money > acc.balance) //如果要取的金额大于账户中的剩余金额，则提示余额不足
		{
			sprintf (res.error, "余额不足！");
			goto send_respond;
		}

		acc.balance -= req.money;//账户中的金额减去要取走的金额

		if (update (&acc) == -1) //更新账户信息
		{
			sprintf (res.error, "更新账户失败！");
			goto send_respond;
		}

		res.balance = acc.balance;//取款成功后，将账户金额展示在客户端

send_respond:

		if (msgsnd (resid, &res, sizeof (res) - sizeof (res.type),0) == -1) //向客户端发送报错信息
		{
			perror ("msgsnd");//如果发送失败，则跳过，继续发送
			continue;
		}
	}

	return 0;
}
