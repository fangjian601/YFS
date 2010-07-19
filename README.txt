/********************************************************
* 实验名称：Caching Extents(Lab6)
* 作者：方建
* 班级：计75
* 学号：2007011365
* 邮箱：fangjian601@gmail.com
* 修改日期：2010-07-19
********************************************************/
文件变更如下：
extent_client.h
	1、添加cache_file类，此类的作用为本地的文件信息的cache
	class cache_file{
	public:
		std::string buffer;			//文件内容
		extent_protocol::attr attr;	//文件属性
		bool data_dirty;			//内容是否被改过
		bool meta_dirty;			//属性是否被改过
		bool data_cached;			//文件内容是否被cache了
		bool meta_cached;			//文件属性是否被cache了
		bool removed;				//这个文件cache是否被删除了
	};
	2、在extent_client中添加了map类型的cache_files和pthread_mutex_t类型的cache_mutex
	std::map<extent_protocol::extentid_t, cache_file*> cache_files; //保存本地文件缓存项
	pthread_mutex_t cache_mutex;									//防止每个client有多个线程时候的数据不一致
	3、添加成员函数flush，将数据提交至服务端

extent_client.cc
	1、添加三个cache_file的构造函数定义
	2、修改extent_client构造函数定义，增加cache_mutex的初始化
	3、修改put、get、putattr、getattr、remove、exist函数的定义，是他们能够满足cache的特性
	4、添加flush函数的定义

lock_client_cache.h
	1、添加lock_releaser类，此类的作用为在release锁的时候flush本地文件缓存
	class lock_releaser : public lock_release_user{
	private:
		extent_client* ec;									//指向extent_client对象的指针
	public:
		lock_releaser(extent_client* _ec);					//构造函数
		virtual ~lock_releaser();							//析构函数
		virtual void dorelease(lock_protocol::lockid_t);	//进行flush动作
	};
lock_client_cache.cc
	1、添加lock_releaser类的构造、析构、dorelease函数的定义
	2、修改releaser函数，在call服务器的release之前，先flush
yfs_client.cc
	1、修改yfs_client的构造，在构造lock_client_cache时候传入extent_client对象指针
fuse.cc
	1、修改readdir函数的一个bug，将锁的release放到最后
