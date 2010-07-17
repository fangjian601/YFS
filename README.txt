/********************************************************
* 实验名称：Caching Locks(Lab5)
* 作者：方建
* 班级：计75
* 学号：2007011365
* 邮箱：fangjian601@gmail.com
* 修改日期：2010-07-17
********************************************************/
实验碰到的问题：
1、臭名昭著的“Distributed DeadLock”
	设计得不小心，来了一个Distributed DeadLock，这个deadlock大概是这么产生的：

	lock_server retry    -------------> lock_client retry
	     ^			RPC调用		   |
	     |					   | 需要获得同一个lock的mutex
	     |	需要同一个唤醒retry的mutex		   |
	     |					   V
	lock_server release  <------------- lock_client releaser	 
				RPC调用

	大概就这么产生了一个Distributed Deadlock，调试了很久才发现这个bug

	解决办法：在client端，多加了一个线程，专么来处理retry，这样的话，lock_client retry就
	不会依赖于lock_client releaser了，环被打破

2、相隔太近的两个信号，可能导致其中一个信号失效（也就是达不到唤醒的目的）	
	这个问题是这样的，举个例子，现在lock_server收到一个release请求，这个release请求需要导致
对retry函数的调用（如果这个锁上还有等待者的话），我们知道要唤醒retry函数的进程，需要获得retry
函数的mutex和向其发送信号量唤醒它，现在的问题是，如果前一个信号量进入了mutex区，发送了信号量，
然后退出mutex。这个时候下一个RPC调用马上接着进入这个mutex，也向retry发送一个信号量（为什么可以这样？
因为在线程调度延迟下，上一个信号可能还没有唤醒等待的retry，因此下一个RPC能够进入这个mutex区，并发送
信号），这样就有问题，上一个信号到了之后，会使得在mutex上等待的线程数为0，如果retry的任务比较长，那么
下一个信号到的时候由于mutex上的等待的线程为0，这样的话，这个信号等于就什么事情也没有做，导致的后果是
本来有一个retry要发给client的，结果给忽略了，这样就会造成死锁。
	解决办法：对于每个类似retry的独立线程，维护一个请求队列，每次在发送信号之前，把请求入队。每当线程
被唤醒的时候，应该处理完队列上的所有请求，这样就算有上面的情况，也不会丢掉该做的工作

实验设计思路
1、lock client端

1.1 数据结构

//存放本地锁信息的struct
struct lock_info_client{
	enum status{NONE = 0,FREE = 1,LOCKED = 2,ACQUIRING = 3,RELEASING = 4}; //五种状态
	lock_protocol::lockid_t id; //锁的标识，id
	status stat;                //锁的状态
	pthread_mutex_t lock_mutex; //锁的mutex变量，用来保证有多个线程操作锁信息时候的一致性
	pthread_cond_t lock_cond;   //锁的条件变量，用来唤醒等待锁的线程
	pthread_cond_t revoke_cond; //锁的revoke条件变量，当有revoke的时候，唤醒等待着的releaser线程
};

//lock_client数据结构
class lock_client_cache : public lock_client {
private:
	......
		
	pthread_mutex_t locks_mutex;				     //用来保证存放所有锁的map的一致性的mutex变量
	pthread_mutex_t releaser_mutex;				     //releaser线程的mutex变量
	pthread_cond_t releaser_cond;				     //唤醒releaser线程的条件变量
	pthread_mutex_t retryer_mutex;				     //retryer线程的mutex变量
	pthread_cond_t retryer_cond;				     //唤醒retryer线程的条件变量

	std::list<lock_protocol::lockid_t> revoke_list;		     //存放revoke请求的队列
	std::list<lock_protocol::lockid_t> retry_list;		     //存放retry请求的队列
	std::map<lock_protocol::lockid_t, lock_info_client*> locks;  //存放所有lock的map

	......
}

1.2 算法流程

acquire函数：根据当前lock的状态来判断处理方法
	(1)NONE：  表明本地没有这个lock的cache，需要从服务器获得，因此调用服务器的acquire函数，如果
			   收到OK，则成功获取，返回。如果收到RETRY，则把线程挂起wait，等待retry函数的调用
	(2)FREE：  直接返回即可
	(3)LOCKED：表明本地有其他线程占用，则将线程挂起wait，等待release函数的调用时候再被唤醒
	(4)RELEASING: 表明该锁正在等待被归还给服务器，因此也应该挂起wait，直到release函数调用时候再被唤醒
	(5)ACQUIRING: 表明该锁正在等待服务器的响应，因此也应该挂机wait，直到retry函数调用时候再被唤醒
release函数：倘若当前锁的状态为LOCKED，则唤醒等待这个锁其他线程；如果锁的状态位RELEASING，则把releaser
	    线程从这个锁的revoke_cond上唤醒
retry函数：收到服务器的retry请求，往retry_list里面加入这个请求，并向retryer发送唤醒信号
revoke函数：收到服务器的revoke请求，往revoke_list里面加入这个请求，并向releaser发送唤醒信号
releaser函数：当releaser线程被revoke函数唤醒，一个一个从队列顶端取出revoke请求，对于每个请求，首先判断，改请求
	     对应锁的状态，如果是LOCKED，则在锁的revoke_cond上wait，并把锁的状态改为RELEASING，等待被
	     release函数唤醒；如果是FREE，则调用服务器的release，将锁还给服务器，同时唤醒在这个锁等待队列
	     里面的一个线程，并将锁的状态置为NONE（这样做的目的是保证这些线程在将来能够得到锁）
retryer函数：当retryer线程被retry函数唤醒，一个一个从队列顶端取出retry请求，对于每个请求，向服务器发送acquire，
	    注意到这时候服务器必然返回OK，于是将该锁的状态置为FREE，并唤醒一个等待改锁的线程			   

2、lock server端

2.1 数据结构

//保存客户端信息的struct
struct client_info{
	std::string id;   //client端id
	std::string host; //域名
	int port;	  //端口号
};

//关于请求的struct
struct request{
	client_info* requester;			//该请求将会由哪个client执行
	lock_protocol::lockid_t request_lid;	//该请求所对应的锁的id
};

//保存锁信息的struct
struct lock_info_server{						
	enum status{FREE,LOCKED,REVOKING,RETRYING}; //锁的四个状态，REVOKING指该锁在等待某个client的revoke操作，RETRYING指该锁正在等待某个client的retry操作
	lock_protocol::lockid_t id;		    //锁的标识，id
	status stat;				    //锁的状态变量
	pthread_mutex_t lock_mutex;
	client_info* retry;			    //如果该锁状态位RETRYING，该变量表明，等待retry操作的那个客户端，其余时候该变量没有意义
	client_info* owner;			    //指该锁目前被哪个client所占用
	std::list<client_info*> waiters;	    //等待该锁的clients队列
};

//lock_server类
class lock_server_cache {
private:
	std::map<std::string, client_info*> clients;				//存放所有client信息的map
	std::map<lock_protocol::lockid_t, lock_info_server*> locks; //存放所有锁信息的map

	std::list<request> revoke_list;								//revoke请求队列
	std::list<request> retry_list;								//retry请求队列

	pthread_mutex_t revoker_mutex;								//revoker线程的mutex变量
	pthread_mutex_t retryer_mutex;								//retryer线程的mutex变量
	pthread_mutex_t locks_mutex;								//locks的mutex变量，用来保证数据一致性

	pthread_cond_t revoker_cond;								//revoker线程的条件变量
	pthread_cond_t retryer_cond;								//retryer线程的条件变量

	......
}										


2.2 算法描述

acquire函数：根据当前的状态进行判断
	(1)FREE：表明锁没有被任何client占用，则直接返回OK
	(2)LOCKED：表明锁被某个client占用，而且没有revoke过，因此这个时候应该往revoke_list里面加入一个revoke请求，
			   同时唤醒revoker线程。最后把请求这个锁的client加入等待改锁的waiters队列
	(3)REVOKING：表明锁被某个client占用，同时正在等待某个client的release，这时候只需将该client加入该锁的waiters队列
	(4)RETRYING：分两种情况，如果当前client的id和所请求锁的retry变量的id相同，则返回OK；如果不是同(3)
release函数：将锁的状态置为FREE，如果该锁的waiters列表不为空，则取第一个client，给其发送retry，同时将锁的状态置为RETRYING，锁的retry域置为那个client
retryer函数：从retry_list中一个一个取出请求，对每个请求向相应的client发送retry请求
revoker函数：从revoke_list中一个一个取出请求，对每个请求向相应的client发送revoke请求	


文件变更如下：
lock_server_cache.h:
	1、增加struct client_info
	2、增加struct request
	3、增加struct lock_info_server
	4、在lock_server_cache中增加map类型的clients
	5、在lock_server_cache中增加map类型的locks
	6、在lock_server_cache中增加list类型的retry_list
	7、在lock_server_cache中增加list类型的revoke_list
	8、在lock_server_cache中增加pthread_mutex_t类型的locks_mutex
	9、在lock_server_cache中增加pthread_mutex_t类型的retryer_mutex
	10、在lock_server_cache中增加pthread_mutex_t类型的revoker_mutex
	11、在lock_server_cache中增加pthread_cond_t类型的retryer_cond
	12、在lock_server_cache中增加pthread_cond_t类型的revoker_cond
	13、增加get_lock私有成员函数
	14、增加acquire成员函数
	15、增加release成员函数
	16、增加subscribe成员函数

lock_server_cache.cc
	1、增加lock_info_server的构造函数定义
	2、增加client_info的构造函数定义
	3、增加静态函数retrythread
	4、修改lock_server_cache的构造函数定义
	5、增加retryer函数的定义
	6、增加revoker函数的定义
	7、增加revoke函数的定义
	8、增加retry函数的定义	
	9、增加stat函数的定义
	10、增加acquire函数的定义
	11、增加release函数的定义
	12、增加subscribe函数的定义
	13、增加get_lock函数的定义

lock_client_cache.h
	1、增加struct lock_info_client
	2、在lock_client_cache中增加pthread_mutex_t类型的locks_mutex
	3、在lock_client_cache中增加pthread_mutex_t类型的retryer_mutex
	4、在lock_client_cache中增加pthread_mutex_t类型的releaser_mutex
	5、在lock_client_cache中增加pthread_cond_t类型的retryer_cond
	6、在lock_client_cache中增加pthread_cond_t类型的releaser_cond	
	7、在lock_client_cache中增加list类型的retry_list
	8、在lock_client_cache中增加list类型的revoke_list
	9、在lock_client_cache中增加map类型的locks
	10、在lock_client_cache中增加私有成员函数rlsrpc_init
	11、在lock_client_cache中增加私有成员函数rlsrpc_reg
	12、在lock_client_cache中增加私有成员函数rlsrpc_subscribe
	13、在lock_client_cache中增加私有成员函数get_lock
	14、在lock_client_cache中增加成员函数retry
	15、在lock_client_cache中增加成员函数revoke
	16、在lock_client_cache中增加成员函数retryer

lock_client_cache.cc
	1、增加lock_info_client构造函数定义
	2、增加静态函数retrythread的定义
	3、修改lock_client_cache的构造函数定义
	4、增加rlsrpc_init函数定义
	5、增加rlsrpc_reg函数定义
	6、增加rlsrpc_subscribe函数定义
	7、增加get_lock函数定义
	8、增加releaser函数定义
	9、增加revoker函数定义
	10、增加revoke函数定义
	11、增加retry函数定义
	12、增加acquire函数定义
	13、增加release函数定义

yfs_client.h
	1、将lock_client类型变成lock_client_cache类型

yfs_client.cc
	1、修改yfs_client的构造函数

lock_smain.cc
	1、增加对lock_server_cache的RPC注册
	
fuse.cc
	1、修改createhelper函数			
