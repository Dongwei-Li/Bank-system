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

#define TYPE_TRANSFER 16

typedef struct tag_Account{
	int    id;
	char   name[256];
	char   passwd[9];
	double balance;
}ACCOUNT;

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

int update (const ACCOUNT* acc) 
{
	char pathname[256];
	sprintf (pathname, "%d.acc", acc -> id);

	int fd = open (pathname, O_WRONLY);
	if (fd == -1) 
	{
		perror ("open");
		return -1;
	}

	if (write (fd, acc, sizeof (*acc)) == -1) 
	{
		perror ("write");
		return -1;
	}
	close (fd);
	return 0;
}

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
void sigint (int signum) //等待服务器发送信息，结束转账
{
	printf ("转账服务即将停止...\n");//提示转账进程要结束了
	exit (0);//杀死自己
}

int main (void) 
{
	key_t KEY_REQUEST=ftok("/home",2);//客户端键值
	key_t KEY_RESPOND=ftok("/home",3);//服务端键值
	
	if (signal (SIGINT, sigint) == SIG_ERR) //捕捉到2号信号后，执行sigint函数，结束转账进程
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

	printf ("转账服务：启动就绪。\n");

	while(1)//接收消息 
	{
		TRANSFER_REQUEST req;//转账客户端结构体

		if (msgrcv (reqid, &req, sizeof (req) - sizeof (req.type),TYPE_TRANSFER, 0) == -1) //接收客户端请求信息。TYPE_TRANSFER消息类型
		{
			perror ("msgrcv");
			continue;//本次接收不到，就跳过，继续接收
		}

		TRANSFER_RESPOND res = {req.pid, ""};//转账服务端结构体
		ACCOUNT acc[2];//账户结构体

		if (get (req.id[0], &acc[0]) == -1) //通过第一个账号获取第一个即自己的账户信息，否则就是无效账号
		{
			sprintf (res.error, "无效账号！");
			goto send_respond;
		}

		if (strcmp (req.name[0], acc[0].name)) //比较客户端的自己的用户名与服务端的用户名是否一致，否则就是无效用户名
		{
			sprintf (res.error, "无效户名！");
			goto send_respond;
		}

		if (strcmp (req.passwd, acc[0].passwd)) //比较密码信息是否一致，服务端保存的密码与用户输入的密码间比较
		{
			sprintf (res.error, "密码错误！");
			goto send_respond;
		}

		if (req.money > acc[0].balance) //比较要转账的金额与剩余金额，判断是否余额不足
		{
			sprintf (res.error, "余额不足！");
			goto send_respond;
		}

		if (get (req.id[1], &acc[1]) == -1) //通过第二个账号获取第二个即对方的账户信息，否则就是无效账号
		{
			sprintf (res.error, "无效对方账号！");
			goto send_respond;
		}

		if (strcmp (req.name[1], acc[1].name)) //比较客户端的对方的用户名与服务端的用户名是否一致，否则就是无效用户名
		{
			sprintf (res.error, "无效对方户名！");
			goto send_respond;
		}

		acc[0].balance -= req.money;//如果转账成功，则自己账号中的金额将减去转账金额

		if (update (&acc[0]) == -1) //更新自己的账号信息
		{
			sprintf (res.error, "更新账户失败！");
			goto send_respond;
		}

		acc[1].balance += req.money;//如果转账成功，对方账号的金额将增加

		if (update (&acc[1]) == -1) //更新对方账户的信息
		{
			sprintf (res.error, "更新对方账户失败！");
			goto send_respond;
		}

		res.balance = acc[0].balance;//将转账之后，自己剩余的金额显示在客户端

send_respond:

		if (msgsnd (resid, &res, sizeof (res) - sizeof (res.type),0) == -1) //向客户端发送报错信息
		{
			perror ("msgsnd");
			continue;//如果发送失败，则跳过，继续发送
		}
	}

	return 0;
}
