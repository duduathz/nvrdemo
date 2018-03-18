#include "http_manger.h"
#include "sql_manger.h"
#include "video_manger.h"
#include "sip_manger.h"
#include "nvr_device_mgr.h"
#include "Units.h"

nvr_http_server		*g_pnvr_http_server;
sql_manger			*g_sm;
video_manger		*g_vm;
sip_manger			*g_sipm;
nvr_device_mgr		*g_ndm;
bool				g_bnvr_run;

unsigned int		g_btrans_thread_num;

bool bRun()
{
	int fd;
	int lock_result;
	struct flock lock;
	char * pFileName = "/root/tmp.lck";
	fd = open(pFileName,O_RDWR|O_CREAT,0777);
	if(fd<0)
	{
		printf("Open file failed.\n");
		return true;
	}
	lock_result = lockf(fd,F_TEST,0);  //参数使用F_LOCK，则如果已经加锁，则阻塞到前一个进程释放锁为止，参数0表示对整个文件加锁
	//返回0表示未加锁或者被当前进程加锁；返回-1表示被其他进程加锁
	if(lock_result<0)
	{
		perror("Exec lockf function failed.1\n");
		return true;
	}

	lock_result = lockf(fd,F_LOCK,0);  //参数使用F_LOCK，则如果已经加锁，则阻塞到前一个进程释放锁为止，参数0表示对整个文件加锁
	if(lock_result<0)
	{
		perror("Exec lockf function failed.2\n");
		return true;
	}
	return false;
}
bool create_db(char *str)
{
	if(g_sm->get_file_block_num()==0)
	{	
		struct statfs fs;
		if(statfs(str,&fs)<0)
		{
			cout<<"存储信息获取失败 ,程序退出"<<endl;
			return false;
		}
		long long size = fs.f_bsize*fs.f_bfree;
		long long ExistFileNum = 0;
		DIR *video_dir;
		if((video_dir=opendir(str))==NULL)
		{
			cout<<"open data dir error"<<endl;
		}
		else
		{
			struct dirent *p_dirent;
			while ((p_dirent=readdir(video_dir))!=NULL)
			{
				ExistFileNum+=1;
			}
			closedir(video_dir);
		}
		if(ExistFileNum>=2)
		{
			ExistFileNum-=2;
		}
		size = size+ExistFileNum*FILE_BLOCK_SIZE;
		unsigned int num = size*0.9/FILE_BLOCK_SIZE;
		g_sm->fill_file_block(string(str),num,0);
	}
	return true;
}
int main(int argc,char *argv[])
{

	int ret, i, status;  
	pid_t pid;
	if(bRun())
	{
		cout<<" nvr was already running"<<endl;
		return 0;
	}
	if(argc>1)
	{
		if(strcmp(argv[1],"-d")==0)
		{
			daemon(0,0);
		}
		else
		{

		}
	}
	bool device_start = true;
	/*
	while(1)
		{
			pid= fork();
			if(pid==0)
			{*/
				g_btrans_thread_num = 0;
				cout<<"hello h3c"<<endl;
				if(eXosip_init()!=0)
				{
					cout<<"eXosip init failed"<<endl;
					return 0;
				}
				char	str[128];
				g_ndm = new nvr_device_mgr();
				g_ndm->updateinfo();
				cout<<"收集设备信息"<<endl;
				read_conf_value("mount_point",str,CONFIG_FILE);
				
			
				g_sm = new sql_manger();
				g_sm->init_db();
				create_db(str);
				g_sm->repair_db(time(NULL));
				cout<<"数据库初始化"<<endl;
				g_pnvr_http_server = new nvr_http_server();
				g_pnvr_http_server->load_config();
				g_pnvr_http_server->start_server();
				g_vm = new video_manger();
				//g_vm->add_video_stream(1,"h3c1","192.168.7.11",81,"admin","21232f297a57a5a743894a0e4a801fc3","0","34010000001130010001");
				//g_vm->add_video_stream(1,"h3c2","192.168.7.11",81,"admin","21232f297a57a5a743894a0e4a801fc3","1","34010000001130010002");
				//g_vm->add_video_stream(1,"h3c3","192.168.7.11",81,"admin","21232f297a57a5a743894a0e4a801fc3","2","34010000001130010003");
				//g_vm->add_video_stream(1,"h3c4","192.168.7.21",81,"admin","21232f297a57a5a743894a0e4a801fc3","0","34010000001130030001");
				//g_vm->add_video_stream(1,"h3c5","192.168.7.21",81,"admin","21232f297a57a5a743894a0e4a801fc3","1","34010000001130020002");
				//g_vm->add_video_stream(1,"h3c6","192.168.7.21",81,"admin","21232f297a57a5a743894a0e4a801fc3","2","34010000001130020003");
				//g_vm->add_video_stream(1,"h3c2","192.168.3.13",81,"admin","21232f297a57a5a743894a0e4a801fc3","1","34010000001310000002");
				
				cout<<"加载数据配置"<<endl;
				g_vm->load_video_stream();
				unsigned int dev_id = 1;
				
				//unsigned int dev_id = g_vm->add_video_stream_temp(1,"h3c001","192.168.3.13",81,"admin","21232f297a57a5a743894a0e4a801fc3","0","34010000002020000011");
				
				//if(g_vm->start_video_stream(dev_id))
				//{
					//g_vm->add_dc_dest(dev_id,"192.168.3.14",81,9001,"admin","21232f297a57a5a743894a0e4a801fc3","0");
					//g_vm->add_dc_dest(dev_id,"192.168.3.14",81,9003,"admin","21232f297a57a5a743894a0e4a801fc3","1");
					//g_vm->add_dc_dest(dev_id,"192.168.3.14",81,9005,"admin","21232f297a57a5a743894a0e4a801fc3","2");
					//g_vm->add_dc_dest(dev_id,"192.168.3.14",81,9007,"admin","21232f297a57a5a743894a0e4a801fc3","3");
					
					//g_vm->add_dest(dev_id,"192.168.0.163",9001);
					//g_vm->add_dest(dev_id,"192.168.0.163",9005);
					//g_vm->add_dest(dev_id,"192.168.0.163",9009);
				//}
				//dev_id = 2;
				//if(g_vm->start_video_stream(dev_id))
				//{
					//g_vm->add_dc_dest(dev_id,"192.168.3.14",81,9001,"admin","21232f297a57a5a743894a0e4a801fc3","0");
					//g_vm->add_dc_dest(dev_id,"192.168.3.14",81,9003,"admin","21232f297a57a5a743894a0e4a801fc3","1");
					//g_vm->add_dc_dest(dev_id,"192.168.3.14",81,9005,"admin","21232f297a57a5a743894a0e4a801fc3","2");
					//g_vm->add_dc_dest(dev_id,"192.168.3.14",81,9007,"admin","21232f297a57a5a743894a0e4a801fc3","3");
					//g_vm->add_dest(dev_id,"192.168.0.163",9001);
					//g_vm->add_dest(dev_id,"192.168.0.21",9001);
					//g_vm->add_dest(dev_id,"192.168.0.163",9009);
				//}
				
				//g_vm->add_video_data_stream(1,dev_id,"34010000001310000001",1377088783,1377089983,"192.168.0.163",9007,1,false);
				
				cout<<"加载数据配置完成"<<endl;
				
				g_sipm = new sip_manger();
				g_sipm->init_sip_manger();
				
				g_bnvr_run = true;
				while(g_bnvr_run)
				{
					sleep(1);
				}
				g_pnvr_http_server->stop_server();
				g_vm->stop_video();	
				/*
			}
			else
			{
					if (pid > 0)
					{ 
            pid = wait(&status); 
               
        	}
        	else
        	{
        		cout<<"fork error"<<endl;
        	}
			}
			if(device_start==false)
			{
				cout<<"nvr restart now!"<<endl;
	      int i = 30;
	      while(i)
	      {
	      	cout<<i<<" seconds waiting"<<endl;
	      	i--;
	      	sleep(1);
	      }
    	}
    	
    	device_start = false;
    }*/
			
}
int get_free_port()
{
	int port = 0;
	int fd = -1;
	socklen_t len= 0;
	port = -1;

#ifndef AF_IPV6
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(0);
	sin.sin_addr.s_addr = htonl(INADDR_ANY);

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(fd < 0){
		printf("socket() error:%s\n", strerror(errno));
		return -1;
	}

	if(bind(fd, (struct sockaddr *)&sin, sizeof(sin)) != 0)
	{
		printf("bind() error:%s\n", strerror(errno));
		close(fd);
		return -1;
	}

	len = sizeof(sin);
	if(getsockname(fd, (struct sockaddr *)&sin, &len) != 0)
	{
		printf("getsockname() error:%s\n", strerror(errno));
		close(fd);
		return -1;
	}

	port = sin.sin_port;
	if(fd != -1)
		close(fd);

#else
	struct sockaddr_in6 sin6;
	memset(&sin6, 0, sizeof(sin6));
	sin.sin_family = AF_INET6;
	sin.sin_port = htons(0);
	sin6.sin_addr.s_addr = htonl(IN6ADDR_ANY);

	fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

	if(fd < 0){
		printf("socket() error:%s\n", strerror(errno));
		return -1;
	}

	if(bind(fd, (struct sockaddr *)&sin6, sizeof(sin6)) != 0)
	{
		printf("bind() error:%s\n", strerror(errno));
		close(fd);
		return -1;
	}

	len = sizeof(sin6);
	if(getsockname(fd, (struct sockaddr *)&sin6, &len) != 0)
	{
		printf("getsockname() error:%s\n", strerror(errno));
		close(fd);
		return -1;
	}

	port = sin6.sin6_port;

	if(fd != -1)
		close(fd);

#endif
	return port;
}
