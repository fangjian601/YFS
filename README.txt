/********************************************************
* 实验名称：Replicated State Machine(Lab8)
* 作者：方建
* 班级：计75
* 学号：2007011365
* 邮箱：fangjian601@gmail.com
* 修改日期：2010-07-25
********************************************************/
文件变更：
rsm.cc
	1、修改recovery函数，由于recovery函数中增加对sync_with_backups和sync_with_slaves两个函数的使用，这个是用来进行slave和primary之间同步的函数，同时在recovery函数中增加对insync和inviewchange两个变量的维护。同时每次commit_change函数结束，都发出一个join_cond上的signal，用来唤醒等待在commit_change里的进程
	2、增加sync_with_backup函数的定义。这个函数的算法是：等待所有的slave进程同步数据，用一个变量nbackup和一个信号量sync_cond进行判断，每次primary进程会在sync_cond上等待某个slave进程的同步，当某个slave进程同步完成（对transferdone rpc调用）后，会产生一个signal，这个signal会唤醒在sync_cond等待的primary进程，被唤醒后primary需要判断nbackup是否等于所期望的值（即所有slave的数量）如果不是则继续wait，如果是则跳出。这里涉及到一个问题就是，如果某个slave不幸宕机，或者死掉，这样造成primary进程会一直等待下去。这里设计了一个办法，由于一旦有某个slave失去联系，config模块会自动调用paxos模块进行view重新构建，一旦新的view被构建出来，config模块会调用commit_change函数，因此我们只需要在commit_change时候判断是不是当前的进程还在sync，如果是则直接唤醒，结束上次过期的同步
	3、增加sync_with_primary函数的定义。这个函数是当前进程为slave的时候调用的，slave进程会通过调用statetransfer函数从primary同步数据，同时调用statetransferdone告诉服务器，同步已经完成。
	4、增加statetransferdone函数的定义。这个函数的功能是调用指定节点的statetransferdone RPC
	5、修改commit_change函数。进入commit_change函数时候，首先判断是否sync值位true，如果是表明当前进程正在进行旧view的同步操作，这时候应该给sync_cond发一个signal告知，放弃旧的同步，开始新的recovery，commit_change会在join_cond上等待旧的recovery通知其完成，然后继续。接下来，commit_change函数应该调用set_primary设置好primary，然后唤醒recovery线程。
	6、增加client_invoke定义。这个函数是client的invoke RPC调用，首先应该判断当前是不是正在更新view如果是则应该返回BUSY，如果当前进程不是primary那么，应该返回NOTPRIMARY，否则获得当前所有的slave进程的handle，像每个slave进程发出invoke RPC调用，只要其中有一个slave出错，则认为整个invoke出错，返回给client ERR，client会重试。（一般情况下这是由于slave进程死掉造成），当所有slave进程成功执行后再去执行primary进程上的相应操作，如果出错返回ERR
	7、增加invoke函数定义。这个函数是节点之间的invoke调用，一旦收到invoke请求，节点回去execute相应的request并返回
	8、增加transferdonereq函数定义
rms_client.cc
	1、增加primary_failure函数定义。此函数的功能是当client和primary通信出现错误的时候调用。一旦出错，client会通过调用members RPC来获得最新的view，同时根据最新的view来设置好primary
lock_server_cache.h
	1、修改client_info、lock_info_server、request三个struct，将里面的指针类型的成员变量变成对象的一个拷贝，同时为每个struct增加针对marshall和unmarshall的操作符重载
	2、增加map类型的rpcc_cache，这是对每个与client通信的rpcc的一个缓存，同时删除原来的在client_info中的成员变量cl。每次要给client发送RPC需要从这个map中取出相应的rpcc，如果没有这需要new一个出来。
	3、增加map类型的rpcc_status，这个是对个client的每个锁的每个request请求的一个存储，存储的是这个request执行前lock的状态，这样做的目的是可以很方便的得到某个动作执行之前锁的状态
	4、修改lock_server_cache，使其从rsm_state_transfer类继承，因此添加两个接口marshal_state和unmarshal_state
	5、增加一个mutex，clients_mutex用来保护存放clients信息的clients变量数据一致
	6、增加addrequest、get_rpcc、get_client、set_lock、set_client函数声明
	7、修改了acquire和release函数的声明，使其参数中包含了request id
lock_server_cache.cc
	1、增加client_info、lock_info_server、request三个struct里面对marshall和unmarshall的操作符重载的定义
	2、修改client_info的构造函数
	3、在lock_server_cache构造函数中增加clients_mutex的初始化
	4、修改revoker函数，只有当当前进程位primary时候才跟client通信，否则的话，直接清空revoke_list
	5、修改retryer函数，只有当当前进程位primary时候才跟client通信，否则的话，直接清空retry_list
	6、修改acquire函数，判断如果是primary进程，而且当前的request已经被执行过（从rpcc_status里面找，如果从在某个request，则说明执行过），这就说明系统出现了错误（primary死掉，某个slave变成了primary等情况），这样的话，将lock设为这个request执行之前的状态，重新做一遍。这样可以保证每个request不会少执行
	7、修改release函数，修改方法同acquire。
	8、增加addrequest函数定义，这个函数用来在revoke和retry的request list里面添加不重复的请求
	9、增加get_rpcc函数定义。这个函数用来得到与每个client对应的rpcc方法，如果没有则创建
	10、增加get_client函数定义。
	11、增加set_lock函数定义
	12、增加set_client函数定义
lock_client_cache.h
	1、增加last_request_id，用来给每个request附上id
	2、增加get_rid函数，获得最新request的id
	3、增加last_request_id的mutex
	4、添加rsm_client声明
lock_client_cache.cc
	1、修改releaser函数，注销掉对dorelease的调用（因为这部分没有涉及到对extent server容错性的处理），同时增加对于duplicate请求的处理
	2、修改retryer函数，增加对duplicate请求的处理
	3、给每个对server的acquire和release调用增加request id，这样需要修改acquire函数，releaser函数和retryer函数
	4、将每个RPC call换成rsm_client的call
	4、增加get_rid函数定义
lock_smain.cc
	1、将lock_server_cache的声明改为rsm的声明
