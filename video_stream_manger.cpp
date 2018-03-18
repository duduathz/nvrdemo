
#include "video_stream_manger.h"
#include "nvr_device_mgr.h"
#include "Units.h"
extern sql_manger			*g_sm;
extern nvr_device_mgr		*g_ndm;

video_stream_manger::video_stream_manger(void)
{
	m_pec_device = NULL;
	m_benable = false;
	m_last_time_recv_data = 0;
	m_brun = false;
	m_buffer = (char *)malloc(CACHE_SIZE);
	m_pwriter = NULL;
	m_bstart_record = false;
	m_bstart_shot = false;
	m_fp_shot = NULL;
	m_pbq = NULL;
	sem_init(&m_sem,0,0);
	m_pub_block = NULL;
	m_pwriter = NULL;
	
	m_preal_transfer = NULL;
	pthread_rwlock_init(&m_tranfer_map_rw_lock,NULL);
	pthread_rwlock_init(&m_tranfer_map_unuse_rw_lock,NULL);
	
}

video_stream_manger::~video_stream_manger(void)
{
	free(m_buffer);
	m_buffer = NULL;
	sem_destroy(&m_sem);
	pthread_rwlock_destroy(&m_tranfer_map_rw_lock);
	pthread_rwlock_destroy(&m_tranfer_map_unuse_rw_lock);
}
int video_stream_manger::create_by_cmd(unsigned int device_type,string device_name,string ip,unsigned int port,string username,string password,string channel,string sip_id)
{
	//web调用 
	m_device_name = device_name;
	m_device_addr = ip;
	m_device_status = 0;
	m_device_type = device_type;
	m_device_port = port;
	m_device_user = username;
	m_device_pass = password;
	m_device_channel = channel;
	m_device_sip_id = sip_id;
	m_video_id = g_sm->add_device_sql(device_type,device_name,ip,port,username,password,channel,sip_id);
	if(m_video_id>=0)
	{
		m_pec_device = new device_manger();
		m_pub_block = new zmq_block_pub();
		//m_pub_block->init_zmq_block_pub(m_video_id);
		//m_preal_transfer = new data_transfer();
		//m_preal_transfer->init_transfer_for_realvideo2(m_pub_block,sip_id);
		m_pwriter = new data_writer();
		//m_pwriter->init_writer(m_video_id);
		m_device_video_plan = "000167";
		load_video_plan();
	}

	return m_video_id;
}
int video_stream_manger::create_by_sql(unsigned int device_id,unsigned int device_type,string device_name,string ip,unsigned int port,string username,string password,string channel,string video_plan,string sip_id)
{
	//加载数据库时调用
	m_video_id = device_id;
	m_device_name = device_name;
	m_device_addr = ip;
	m_device_status = 0;
	m_device_type = device_type;
	m_device_port = port;
	m_device_user = username;
	m_device_pass = password;
	m_device_channel = channel;
	m_device_sip_id = sip_id;

	m_pec_device = new device_manger();
	m_pub_block = new zmq_block_pub();
	
	//m_preal_transfer = new data_transfer();
	//m_preal_transfer->init_transfer_for_realvideo2(m_pub_block,m_device_sip_id);
	m_pwriter = new data_writer();
	
	m_device_video_plan=video_plan;
	load_video_plan();
	return 0;
}
int video_stream_manger::create_by_temp(unsigned int device_id,unsigned int device_type,string device_name,string ip,unsigned int port,string username,string password,string channel,string sip_id)
{
	//进中转时的sip调用
	m_video_id = device_id;
	m_device_name = device_name;
	m_device_addr = ip;
	m_device_status = 0;
	m_device_type = device_type;
	m_device_port = port;
	m_device_user = username;
	m_device_pass = password;
	m_device_channel = channel;
	m_device_sip_id = sip_id;
	m_pec_device = new device_manger();
	m_pub_block = new zmq_block_pub();
	//m_pub_block->init_zmq_block_pub(m_video_id);
	//m_preal_transfer = new data_transfer();
	//m_preal_transfer->init_transfer_for_realvideo2(m_pub_block,sip_id);
	//m_pwriter = new data_writer();
	//m_pwriter->init_writer(m_video_id);
	m_device_video_plan="";
	load_video_plan();
	return device_id;
}
int video_stream_manger::update_by_cmd(unsigned int device_id,unsigned int device_type,string device_name,string ip,unsigned int port,string username,string password,string channel)
{
	g_sm->update_device_sql(device_id,device_type,device_name,ip,port,username,password,channel);
	return 0;
}
int video_stream_manger::update_plan_by_cmd(unsigned int device_id,string plan,unsigned int maxday)
{
	g_sm->update_device_plan_sql(device_id,plan,maxday);
	m_device_video_plan = plan;
	load_video_plan();
	return 0;
}
int video_stream_manger::start_stream()
{
	if(m_brun)
	{
		m_device_status = 1;
		cout<<"设备已经在运行，跳过启动步骤"<<endl;
		return 0;
	}
	cout<<"登录编码器"<<m_device_addr<<" 本地地址:"<<g_ndm->ip_addr<<endl;
	if(m_pec_device->device_login(m_device_user,m_device_pass,m_device_addr,m_device_port))
	{
		int port = get_free_port();
		if(port>0)
		{
			m_recv_port = port;
			m_localip=g_ndm->ip_addr;
			m_pec_device->start_video_stream(m_localip,port,m_device_channel);
			struct sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			//addr.sin_addr.s_addr = inet_addr(localip.c_str());
			m_udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
			if (bind(m_udp_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
			{
				return -1;
			}

			//if(m_preal_transfer->start_transfer())
			{
				m_device_status = 1;
				m_brun = true;
				m_benable = true;
				pthread_create(&m_recv_data_pthread,NULL,recv_data_thread,this);
				g_sm->start_device_sql(m_video_id);
				cout<<"登录编码器"<<m_device_addr<<"成功"<<endl;
				if(m_pub_block)
				{
					m_pub_block->init_zmq_block_pub(m_video_id);
				}
				if(m_pwriter)
				{
					m_pwriter->init_writer(m_video_id);
				}
				return 0;
			}
			return 1;
		}
		return 2;
	}
	cout<<"登录编码器"<<m_device_addr<<"失败"<<endl;
	return 3;

}
int video_stream_manger::stop_stream()
{
	stop_recv_stream();
	pthread_rwlock_wrlock(&m_tranfer_map_rw_lock);
	transfer_map::iterator item = m_transfer_map.begin();
	for(;item!=m_transfer_map.end();)
	{
		item->second->stop_transfer();
		delete item->second;
		m_transfer_map.erase(item++);
	}
	pthread_rwlock_unlock(&m_tranfer_map_rw_lock);
	return 0;
}
int	video_stream_manger::stop_recv_stream()
{
	if(m_brun)
	{
		m_brun = false;
		stop_writer();
		if(m_recv_data_pthread)
		{	
			int pthread_kill_err = pthread_kill(m_recv_data_pthread,0);
			if(pthread_kill_err!=ESRCH&&pthread_kill_err!=EINVAL)
			{
				pthread_join(m_recv_data_pthread,NULL);
			}
		}
		close(m_udp_sock);
		if(m_pec_device)
			m_pec_device->device_logout();
		m_benable = false;		
	}
}
int	video_stream_manger::start_recv_stream()
{
	if(m_brun)
	{
		cout<<"设备已经在运行，跳过启动步骤"<<endl;
		return 0;
	}
	cout<<"重新登录编码器"<<m_device_addr<<" 本地地址:"<<g_ndm->ip_addr<<endl;
	if(m_pec_device->device_login(m_device_user,m_device_pass,m_device_addr,m_device_port))
	{
		int port = get_free_port();
		if(port>0)
		{
			m_recv_port = port;
			m_localip=g_ndm->ip_addr;
			m_pec_device->start_video_stream(m_localip,port,m_device_channel);
			struct sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			//addr.sin_addr.s_addr = inet_addr(localip.c_str());
			m_udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
			if (bind(m_udp_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
			{
				return -1;
			}
			{
				m_brun = true;
				m_benable = true;
				pthread_create(&m_recv_data_pthread,NULL,recv_data_thread,this);
				g_sm->start_device_sql(m_video_id);
				cout<<"重新登录编码器"<<m_device_addr<<"成功"<<endl;
				if(m_pub_block)
				{
					m_pub_block->init_zmq_block_pub(m_video_id);
				}
				if(m_pwriter)
				{
					m_pwriter->init_writer(m_video_id);
				}
				return 0;
			}
			return 1;
		}
		return 2;
	}
	cout<<"登录编码器"<<m_device_addr<<"失败"<<endl;
	return 3;
}
int	video_stream_manger::start_writer()
{
	if(m_benable)
	{
		if(m_pwriter)
		{
			m_pwriter->start_thread2(m_pub_block);
		}
	}
	return 0;
}
int	video_stream_manger::stop_writer()
{

	if(m_pwriter)
	{
		m_pwriter->stop_thead();
	}
	return 0;
}
int video_stream_manger::update_time()
{
	if(m_transfer_map.size()>0)
	{
		pthread_rwlock_wrlock(&m_tranfer_map_rw_lock);
		transfer_map::iterator item = m_transfer_map.begin();
		for (;item!=m_transfer_map.end();item++)
		{
			item->second->update_time();
		}
		pthread_rwlock_unlock(&m_tranfer_map_rw_lock);
	}

	return 0;
}
int video_stream_manger::update_transfer_map()
{
	pthread_rwlock_wrlock(&m_tranfer_map_unuse_rw_lock);
	transfer_map::iterator item = m_transfer_unuse_map.begin();
	for (;item!=m_transfer_unuse_map.end();)
	{
		if(time(NULL)+TIME_OFFSET-item->second->m_time_update>360)
		{
			delete item->second;
			m_transfer_unuse_map.erase(item++);
		}
		else
		{
			item++;
		}
	}
	pthread_rwlock_unlock(&m_tranfer_map_unuse_rw_lock);
	return 0;
}
int video_stream_manger::add_dc_dest(string ip,unsigned int port,unsigned int recv_port,string username,string password,string channel)
{
	device_manger *pdc = new device_manger();
	if(pdc->device_login(username,password,ip,port))
	{
		cout<<"解码器登录成功"<<ip<<":"<<recv_port<<endl;
		if(pdc->start_video_stream(m_localip,recv_port,channel))
		{
			cout<<"开始向解码器发送流 "<<ip<<recv_port<<"--通道"<<channel<<endl;
			string key = ip+channel;
			m_dc_device_map.insert(map<string,device_manger *>::value_type(key,pdc));
			//m_preal_transfer->add_dest_addr_for_realvideo(ip,recv_port);
			add_dest(ip,recv_port);
			return 0;
		}
		cout<<"启动向解码器发送流失败  "<<ip<<recv_port<<"--通道"<<channel<<endl;
		return 1;
	}
	cout<<"解码器登录失败"<<ip<<":"<<recv_port<<endl;
	return 2;	
}
int video_stream_manger::add_dest(string ip,unsigned int port)
{
	//m_preal_transfer->add_dest_addr_for_realvideo(ip,port);
	cout<<"设备"<<m_video_id<<"添加目标地址"<<ip<<":"<<port<<endl;
	return add_dest_ex(ip,port);
}
int video_stream_manger::del_dc_dest(string ip,unsigned int recv_port,string channel)
{
	//删除解码器目标地址
	//m_preal_transfer->del_dest_addr_for_realvideo(ip,recv_port);
	string key = ip+channel;
	if(m_dc_device_map.find(key)!=m_dc_device_map.end())
	{
		m_dc_device_map.find(key)->second->stop_video_stream();
		m_dc_device_map.find(key)->second->device_logout();
		m_dc_device_map.erase(key);
	}
	return 0;
}
int video_stream_manger::del_dest(string ip,unsigned int port)
{
	//m_preal_transfer->del_dest_addr_for_realvideo(ip,port);
	return del_dest_ex(ip,port);
}
int video_stream_manger::add_dest_ex(string ip,unsigned int port)
{
	char	str_key[128];
	if(m_brun)
	{
		sprintf(str_key,"%s_%d",ip.c_str(),port);
		pthread_rwlock_wrlock(&m_tranfer_map_rw_lock);
		transfer_map::iterator item = m_transfer_map.find(string(str_key));
		if(item==m_transfer_map.end())
		{
			if(m_transfer_unuse_map.size()==0)
			{
				data_transfer * ptransfer = new data_transfer();
				ptransfer->init_transfer_for_realvideo2(m_pub_block,m_device_sip_id);
				//ptransfer->init_transfer_for_realvideo(m_pbq,m_device_sip_id);
				ptransfer->add_dest_addr_for_realvideo(ip,port);
	
				m_transfer_map.insert(map<string,data_transfer *>::value_type(string(str_key),ptransfer));
				pthread_rwlock_unlock(&m_tranfer_map_rw_lock);
				return ptransfer->start_transfer();
			}
			else
			{
				transfer_map::iterator item = m_transfer_unuse_map.begin();
				item->second->add_dest_addr_for_realvideo(ip,port);
				
				m_transfer_map.insert(map<string,data_transfer *>::value_type(string(str_key),item->second));
				pthread_rwlock_wrlock(&m_tranfer_map_unuse_rw_lock);
				m_transfer_unuse_map.erase(item);
				pthread_rwlock_unlock(&m_tranfer_map_unuse_rw_lock);
				pthread_rwlock_unlock(&m_tranfer_map_rw_lock);
				return item->second->start_transfer();
					
			}
		}
		else
		{
			pthread_rwlock_unlock(&m_tranfer_map_rw_lock);
			return item->second->start_transfer();
		}
	}
	else
	{
		cout<<"设备不在线或故障中，视频传输请求执行失败"<<endl;
		return 0;
	}

}
int video_stream_manger::del_dest_ex(string ip,unsigned int port)
{
	char	str_key[128];
	sprintf(str_key,"%s_%d",ip.c_str(),port);
	pthread_rwlock_wrlock(&m_tranfer_map_rw_lock);
	transfer_map::iterator item = m_transfer_map.find(string(str_key));
	if(item!=m_transfer_map.end())
	{
		item->second->stop_transfer();
		pthread_rwlock_wrlock(&m_tranfer_map_unuse_rw_lock);
		m_transfer_unuse_map.insert(map<string,data_transfer *>::value_type(item->first,item->second));
		pthread_rwlock_unlock(&m_tranfer_map_unuse_rw_lock);
		m_transfer_map.erase(item);
		//cout<<"停止视频传输"<<str_key<<endl;
	}
	pthread_rwlock_unlock(&m_tranfer_map_rw_lock);
	return 0;
}
int video_stream_manger::delete_stream_sql()
{
	if(g_sm->delete_device_sql(m_video_id))
	{
		return 1;
	}
	return 0;
}
int video_stream_manger::delete_stream_mem()
{
	if(m_benable)
	{
		stop_stream();
	}
	if(m_pec_device)
	{
		delete m_pec_device;
		m_pec_device = NULL;
	}
	if(m_pbq)
	{
		delete m_pbq;
		m_pbq = NULL;
	}
	if(m_preal_transfer)
	{
		delete m_preal_transfer;
		m_preal_transfer = NULL;
	}
	if(m_pub_block)
	{
		m_pub_block->uninit_zmq_block();
		delete m_pub_block;
		m_pub_block = NULL;
	}
	return 0;
}
bool video_stream_manger::check_next_hour_video_plan(struct tm *p)
{
	if(p->tm_wday==0)
	{
		p->tm_wday=7;
	}
	unsigned int nexthour  = (p->tm_wday-1)*24+p->tm_hour+1;
	//cout<<"---"<<p->tm_wday<<"--"<<p->tm_hour<<endl;
	if(nexthour==0xA8)
	{
		nexthour = 0;
	}
	planlist::iterator	item;
	for(item = m_plan.begin();item!=m_plan.end();item++)
	{
		//cout<<(*item).sthour<<"--nexthour--"<<nexthour<<(*item).ethour<<endl;
		if((*item).sthour<=nexthour&&(*item).ethour>=nexthour)
		{
			return true;
		}
	}
	return false;
}
bool video_stream_manger::is_enable()
{
	return m_benable;
}
bool video_stream_manger::is_temp()
{
	if(m_pwriter)
	{
		return false;
	}
	return true;
}
bool video_stream_manger::keepalive()
{
	if(m_brun)
	{
		m_pec_device->device_keepalive();
		dc_device_map::iterator item = m_dc_device_map.begin();
		for (;item!=m_dc_device_map.end();item++)
		{
			item->second->device_keepalive();
		}
		return true;
	}
	return false;

}
bool video_stream_manger::load_video_plan()
{
	m_plan.clear();
	if(m_device_video_plan.length()%6==0)
	{
		for(int i=0;i<m_device_video_plan.length()/6;i++)
		{
			video_plan vp;
			string st = m_device_video_plan.substr(i*6,3);
			string et = m_device_video_plan.substr(i*6+3,3);
			vp.sthour = atoi(st.c_str());
			vp.ethour = atoi(et.c_str());
			m_plan.push_back(vp);
		}
		return true;
	}
	return false;
}
time_t video_stream_manger::get_last_time()
{
	return m_last_time_recv_data;
}

void * video_stream_manger::recv_data_thread(void *lp)
{
	char	buffer[10240];
	video_stream_manger *pvsm = (video_stream_manger *)lp;
	struct sockaddr_in clientAddr;
	int n;
	socklen_t len = sizeof(clientAddr);
	int shot_len = 0;
	bool	start_record = false;

	pvsm->m_brun = true;
	pvsm->m_last_time_recv_data = time(NULL)+TIME_OFFSET;
	int		m = 0;
	struct timeval tv_out;
	tv_out.tv_sec=2;
	tv_out.tv_usec=0;
	pvsm->m_device_status = 1;
	setsockopt(pvsm->m_udp_sock,SOL_SOCKET,SO_RCVTIMEO,&tv_out,sizeof(tv_out));
	cout<<"start recv data at video "<<pvsm->m_video_id<<endl;
	pvsm->update_time();
	while(pvsm->m_brun)
	{
		n = recvfrom(pvsm->m_udp_sock, buffer, 10240, 0, (struct sockaddr*)&clientAddr, &len);
		if(n>0)
		{
			//cout<<"recv data length "<<n<<endl;
			//pvsm->m_pbq->insert_block_buf(buffer,n);
			pvsm->m_pub_block->insert_block_buf(buffer,n);
			//cout<<"insert block buf "<<n<<endl;
			pvsm->m_last_time_recv_data = time(NULL)+TIME_OFFSET;
			//想截图只能在这里截图
			if(pvsm->m_bstart_shot)
			{
				if(start_record==false)
				{
					int count = n/188;
					for(int i=0;i<count;i++)
					{
						unsigned start_indicator = buffer[i*188+1]>>6&0x01;
						if(start_indicator)
						{
							short pid = ReadInt16((unsigned char *)buffer+i*188+1);
							if((pid&0x1FFF)==0)
							{
								start_record = true;
							}
						}
					}
				}
				if(pvsm->m_fp_shot&&start_record)
				{
					shot_len+=n;
					fwrite(buffer,n,1,pvsm->m_fp_shot);
					if(shot_len>500*1024)
					{
						fclose(pvsm->m_fp_shot);
						shot_len = 0;
						pvsm->m_fp_shot = NULL;
						pvsm->m_bstart_shot = false;
						start_record = false;
						sem_post(&pvsm->m_sem);
					}
				}
			}
			pvsm->update_time();
		}
		else
		{
				if(errno!=4)
				{
					if(errno==11)
					{
						m++;
					}
					pvsm->m_device_status = 4;
					cout<<"接受超时"<<pvsm->m_device_name<<"接受端口"<<pvsm->m_recv_port<<"错误号"<<errno<<endl;
				}
				if(m>5)
					break;
		}
	}
	cout<<"recv data over at video "<<pvsm->m_video_id<<"接受端口"<<pvsm->m_recv_port<<endl;
	pvsm->m_brun = false;
	close(pvsm->m_udp_sock);
	if(pvsm->m_pec_device)
		pvsm->m_pec_device->device_logout();
	pvsm->m_benable = false;
	
	return NULL;
}
bool video_stream_manger::pre_file(char *filename,unsigned int file_id)
{
	if(m_pwriter)
	{
		return m_pwriter->pre_file(filename,file_id);
	}
	return false;
}
bool video_stream_manger::using_prefile()
{
	if(m_pwriter)
	{
		return m_pwriter->using_prefile();
	}
	return false;
}
string		video_stream_manger::get_prefile_name()
{
	if(m_pwriter)
	{
		return m_pwriter->get_prefile_name();
	}
	return "";
}
bool video_stream_manger::get_idx_buffer(char *buffer,unsigned int len)
{
	if(m_pwriter)
	{
		return m_pwriter->get_idx_buffer(buffer,len);
	}
	return false;
}
bool video_stream_manger::have_a_short()
{
	m_bstart_shot = true;
	char str[128];
	sprintf(str,"%s%d.dat",SHOT_FILE_DIR,m_video_id);
	m_fp_shot = fopen(str,"wb+");
	cout<<"创建截图准备文件"<<str<<endl;
	struct timespec ts;
	ts.tv_sec = time(NULL)+5;
	ts.tv_nsec = 0;
	int ret = sem_timedwait(&m_sem,&ts);
	//sem_wait(&m_sem);
	if(ret==0)
	{
		cout<<"截图准备文件完成"<<endl;
		return true;
	}
	else
	{
		cout<<"截图准备文件失败"<<endl;
		return false;
	}
}



















video_transit_manger::video_transit_manger()
{
}
video_transit_manger::~video_transit_manger()
{
}
int video_transit_manger::create_transit(string sip_id,unsigned int video_id)
{
	unsigned short recv_port;
	m_source_sipid = sip_id;
	struct sockaddr_in addr;
	recv_port = get_free_port();
	addr.sin_family = AF_INET;
	addr.sin_port = htons(recv_port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	m_udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (bind(m_udp_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
			cout<<"绑定端口"<<recv_port<<"失败"<<endl;
			return -1;
	}
	m_recv_port = recv_port;
	m_brun = true;
	m_benable = true;
	pthread_create(&m_recv_data_pthread,NULL,recv_data_thread,this);
	m_pub_block = new zmq_block_pub(); 
	if(m_pub_block)
	{
		m_pub_block->init_zmq_block_pub(video_id);
	}
	cout<<"绑定端口"<<recv_port<<"成功"<<endl;
	return recv_port;
}
bool video_transit_manger::delete_transit()
{
	transfer_map::iterator item = m_transfer_map.begin();
	for(;item!=m_transfer_map.end();item++)
	{
		item->second->stop_transfer();
		delete item->second;
	}
	m_transfer_map.clear();
	m_brun = false;
	close(m_udp_sock);
	usleep(100000);
	delete m_pub_block;
	m_pub_block = NULL;
	return true;
}
int video_transit_manger::add_dest(string dest_ip,unsigned short dest_port)
{
	char	str_key[128];
	if(m_brun)
	{
		sprintf(str_key,"%s_%d",dest_ip.c_str(),dest_port);
		transfer_map::iterator item = m_transfer_map.find(string(str_key));
		if(item==m_transfer_map.end())
		{
			data_transfer * ptransfer = new data_transfer();
			ptransfer->init_transfer_for_realvideo2(m_pub_block,m_source_sipid);
			ptransfer->add_dest_addr_for_realvideo(dest_ip,dest_port);
	
			m_transfer_map.insert(map<string,data_transfer *>::value_type(string(str_key),ptransfer));
			return ptransfer->start_transfer();
		}
		else
		{
			return item->second->start_transfer();
		}
	}
	else
	{
		cout<<"设备不在线或故障中，视频传输请求执行失败"<<endl;
		return 0;
	}
}
bool video_transit_manger::del_dest(string dest_ip,unsigned short dest_port)
{
	char	str_key[128];
	sprintf(str_key,"%s_%d",dest_ip.c_str(),dest_port);
	transfer_map::iterator item = m_transfer_map.find(string(str_key));
	if(item!=m_transfer_map.end())
	{
		item->second->stop_transfer();
		delete item->second;
		m_transfer_map.erase(item);
		cout<<"停止视频传输"<<str_key<<endl;
		return true;
	}
	return false;
}	 
void * video_transit_manger::recv_data_thread(void *lp)
{
	
	char	buffer[102400];
	video_transit_manger	*pvtm = (video_transit_manger *)lp;
	struct sockaddr_in clientAddr;
	int n;
	socklen_t len = sizeof(clientAddr);
	pvtm->m_last_time_recv_data = time(NULL)+TIME_OFFSET;
	int		m = 0;
	struct timeval tv_out;
	tv_out.tv_sec=2;
	tv_out.tv_usec=0;
	pvtm->m_brun = true;
	setsockopt(pvtm->m_udp_sock,SOL_SOCKET,SO_RCVTIMEO,&tv_out,sizeof(tv_out));
	cout<<"start recv data at video "<<pvtm->m_source_sipid<<endl;
	while(pvtm->m_brun)
	{
		n = recvfrom(pvtm->m_udp_sock, buffer, 102400, 0, (struct sockaddr*)&clientAddr, &len);
		if(n>0)
		{
			pvtm->m_pub_block->insert_block_buf(buffer,n);
			pvtm->m_last_time_recv_data = time(NULL)+TIME_OFFSET;
		}
		else
		{
				if(errno!=4)
				{
					if(errno==11)
					{
						m++;
					}
					cout<<"接受超时"<<pvtm->m_source_sipid<<"错误号"<<errno<<endl;
				}
				if(m>5)
					break;
		}
	}
	cout<<"start recv data over at video "<<pvtm->m_source_sipid<<endl;
	pvtm->delete_transit();
	return 0;
}