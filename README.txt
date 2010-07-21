/********************************************************
* 实验名称：Paxos(Lab7)
* 作者：方建
* 班级：计75
* 学号：2007011365
* 邮箱：fangjian601@gmail.com
* 修改日期：2010-07-21
********************************************************/
文件变更如下：
rsm.cc
	1、增加commit_change函数定义
	2、增加joinreq函数定义
paxos.cc
	1、增加proposer成员函数prepare定义
	2、增加proposer成员函数accept定义
	3、增加proposer成员函数decide定义
	4、增加acceptor成员函数preparereq定义
	5、增加acceptor成员函数acceptreq定义
	6、增加proposer成员函数decidepreq定义
	7、修改proposer成员函数run定义
		
