/********************************************************
* 实验名称：MKDIR, REMOVE, and Locking(Lab4)
* 作者：方建
* 班级：计75
* 学号：2007011365
* 邮箱：fangjian601@gmail.com
* 修改日期：2010-07-09
********************************************************/
文件变更如下：
yfs_client.h
	1、增加lock_client成员变量lc，用来解决同步问题
	2、增加acquire函数声明，用来获得锁
	3、增加release函数声明，用来释放锁
yfs_client.cc	
	1、在yfs_client构造函数中增加对lock_client的初始化
	2、增加acquire函数定义
	3、增加release函数定义
fuse.cc
	1、修改getattr函数，不使用原来的getfile和getdir方法获得文件属性，用yfs->getattr替代
	2、修改fuseserver_getattr函数，增加锁的acquire和release
	3、修改fuseserver_read函数，增加锁的acquire和release
	4、修改fuseserver_write函数，增加锁的acquire和release
	5、修改createhelper函数，增加锁的acquire和release
	6、修改fuseserver_lookup函数，增加锁的acquire和release
	7、修改fuseserver_readdir函数，增加锁的acquire和release
	8、修改fuseserver_open函数，增加锁的acquire和release
	9、修改fuseserver_unlink函数，增加锁的acquire和release
	10、修改fuseserver_mkdir函数，增加锁的acquire和release
	
