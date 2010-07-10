/********************************************************
* 实验名称：Reading, Writing and Sharing Files(Lab3)
* 作者：方建
* 班级：计75
* 学号：2007011365
* 邮箱：fangjian601@gmail.com
* 修改日期：2010-07-09
********************************************************/
文件变更如下：
extent_server.h
	1、增加成员函数putattr的声明，此函数用来设置attr属性
extent_server.cc
	1、增加成员函数putattr的定义
extent_client.h
	1、增加成员函数putattr的声明，此函数为一个新的rpc调用，用来设置文件的attr属性
extent_client.cc
	1、增加成员函数putattr的定义
extent_protocol.h
	1、增加枚举类rpc_numbers的成员putattr
extent_smain.c
	1、在main函数中注册putattr函数
yfs_client.h
	1、增加成员函数putattr的声明
	2、增加成员函数getattr的声明
yfs_client.cc
	1、增加成员函数putattr的定义
	2、增加成员函数getattr的定义
fuse.cc
	1、增加read函数的定义
	2、增加write函数的定义
	3、增加open函数的定义								
