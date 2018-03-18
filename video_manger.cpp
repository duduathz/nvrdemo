
#include "video_manger.h"
extern sql_manger	* g_sm;

video_manger::video_manger(void)
{
	device_id_temp = 50000;
	pthread_rwlock_init(&m_stream_lock,NULL);
	pthread_rwlock_init(&m_data_lock,NULL);
	
}

video_manger::~video_manger(void)
{
	pthread_rwlock_destroy(&m_stream_lock);
	pthread_rwlock_destroy(&m_data_lock);
}
unsigned int	video_manger::get_free_temp_device_id()
{
	if(device_id_temp>100000)
	{
		device_id_temp = 50000;
	}
	device_id_temp++;
	return device_id_temp;
}
bool video_manger::load_video_stream()
{
	device_list		dl;
	int size = sizeof(time_t);
	device_list::iterator item;
	g_sm->get_device_list_sql(dl);

	if(dl.size())
	{
		for (item=dl.begin();item!=dl.end();item++)
		{
			video_stream_manger		*pvsm = new video_stream_manger();
			pvsm->create_by_sql((*item).device_id,(*item).type,(*item).name,(*item).ip,(*item).port,(*item).user,(*item).pass,(*item).channel,(*item).video_plan,(*item).sip_id);
			
			if(m_manger_map.find((*item).device_id)==m_manger_map.end())
			{
				m_sip_map_id.insert(map<string,unsigned int>::value_type((*item).sip_id,(unsigned int )(*item).device_id));
				cout<<"加载设备"<<(*item).sip_id<<endl;
				if((*item).status==1)
				{
					bool ret = start_video_stream((*item).device_id);
					if(ret==false)
					{
						cout<<(*item).device_id<<"设备登录启动失败,加入重连列表"<<endl;
						//加入重连列表
						m_manger_reconnect_map.insert(map<unsigned int,video_stream_manger *>::value_type((unsigned int )(*item).device_id,pvsm));
					}
					else
					{
						m_manger_map.insert(map<unsigned int,video_stream_manger *>::value_type((unsigned int )(*item).device_id,pvsm));
					}
				}
			}
			else
			{
				delete pvsm;
				pvsm = NULL;
			}
			
		}
	}
	cout<<"start video stream server "<<endl;
	m_brun = true;
	pthread_create(&m_thread_prepair,NULL,thread_check_prepair_fileblock,this);
	usleep(10000);
	pthread_create(&m_thread_keep_alive,NULL,thread_keep_device_alive,this);
	pthread_create(&m_thread_check_device_online,NULL,thread_check_device_online,this);
	pthread_create(&m_thread_have_short,NULL,thread_take_short,this);
	pthread_create(&m_thread_reconnect,NULL,thread_reconnect,this);
	usleep(10000);
	pthread_create(&m_thread_check_video_plan,NULL,thread_check_video_plan,this);
	
	return true;
}
bool video_manger::stop_video()
{
	m_brun = false;
	cout<<"stop video stream server "<<endl;
	data_manger_map::iterator item_data;
	stream_manger_map::iterator item_stream;
	pthread_rwlock_wrlock(&m_data_lock);
	for(item_data = m_data_manger_map.begin();item_data!=m_data_manger_map.end();item_data++)
	{
		item_data->second->stop();
		delete item_data->second;
	}
	m_data_manger_map.clear();
	pthread_rwlock_unlock(&m_data_lock);
	pthread_rwlock_wrlock(&m_stream_lock);
	for(item_stream = m_manger_map.begin();item_stream!=m_manger_map.end();item_stream++)
	{
		item_stream->second->stop_stream();
		delete item_stream->second;
	}
	m_manger_map.clear();
	pthread_rwlock_unlock(&m_stream_lock);

}
int video_manger::add_video_data_stream(uint64_t did,unsigned int video_id,string sip_id,time_t st,time_t et,string ip,unsigned short port,int did_2,bool download)
{
	video_data_manger	*vdm = new video_data_manger();
	int send_port = vdm->create(video_id,sip_id,st,et,ip,port,did_2,download);
	if(send_port>0)
	{
		pthread_rwlock_wrlock(&m_data_lock);
		if(m_data_manger_map.find(did)==m_data_manger_map.end())
		{
			m_data_manger_map.insert(map<uint64_t,video_data_manger *>::value_type(did,vdm));
		}
		pthread_rwlock_unlock(&m_data_lock);
	}
	return send_port;
}
bool video_manger::pause_video_data_stream(uint64_t did)
{
	pthread_rwlock_wrlock(&m_data_lock);
	if(m_data_manger_map.find(did)!=m_data_manger_map.end())
	{
		if(m_data_manger_map.find(did)->second->pause())
		{
			pthread_rwlock_unlock(&m_data_lock);
			return true;
		}
	}
	pthread_rwlock_unlock(&m_data_lock);
	return false;
}
bool video_manger::speed_video_data_stream(uint64_t did,float nspeed)
{
	pthread_rwlock_wrlock(&m_data_lock);
	if(m_data_manger_map.find(did)!=m_data_manger_map.end())
	{
		if(m_data_manger_map.find(did)->second->speed(nspeed))
		{
			pthread_rwlock_unlock(&m_data_lock);
			return true;
		}
	}
	pthread_rwlock_unlock(&m_data_lock);
	return false;
}
bool video_manger::stop_video_data_stream(uint64_t did)
{
	pthread_rwlock_wrlock(&m_data_lock);
	if(m_data_manger_map.find(did)!=m_data_manger_map.end())
	{
		if(m_data_manger_map.find(did)->second->stop())
		{
			delete m_data_manger_map.find(did)->second;
			m_data_manger_map.erase(did);
			pthread_rwlock_unlock(&m_data_lock);
			return true;
		}
	}
	pthread_rwlock_unlock(&m_data_lock);
	return false;
}
bool video_manger::add_video_stream(unsigned int device_type,string device_name,string ip,unsigned int port,string username,string password,string channel,string sip_id)
{
	video_stream_manger		*pvsm = new video_stream_manger();
	int device_id = pvsm->create_by_cmd(device_type,device_name,ip,port,username,password,channel,sip_id);
	if(device_id>=0)
	{
		pthread_rwlock_wrlock(&m_stream_lock);
		//cout<<"进入锁1"<<endl;
		if(m_manger_map.find(device_id)==m_manger_map.end())
		{
			m_manger_map.insert(map<unsigned int,video_stream_manger *>::value_type((unsigned int )device_id,pvsm));
			m_sip_map_id.insert(map<string,unsigned int>::value_type(sip_id,(unsigned int )device_id));
			//cout<<"出入锁1"<<endl;
			pthread_rwlock_unlock(&m_stream_lock);
			cout<<"添加摄像机成功"<<endl;
			return true;
		}
		//cout<<"出入锁1"<<endl;
		pthread_rwlock_unlock(&m_stream_lock);
	}
	cout<<"添加摄像机失败"<<device_id<<endl;
	pvsm->delete_stream_mem();
	delete pvsm;
	pvsm = NULL;
	return false;
}
unsigned int  video_manger::add_video_stream_temp(unsigned int device_type,string device_name,string ip,unsigned int port,string username,string password,string channel,string sip_id)
{
	video_stream_manger		*pvsm = new video_stream_manger();
	int device_id = pvsm->create_by_temp(get_free_temp_device_id(),device_type,device_name,ip,port,username,password,channel,sip_id);
	if(device_id>=0)
	{
		pthread_rwlock_wrlock(&m_stream_lock);
		//cout<<"进入锁2"<<endl;
		if(m_manger_map.find(device_id)==m_manger_map.end())
		{
			m_manger_map.insert(map<unsigned int,video_stream_manger *>::value_type((unsigned int )device_id,pvsm));
			m_sip_map_id.insert(map<string,unsigned int>::value_type(sip_id,(unsigned int )device_id));
			//cout<<"出入锁2"<<endl;
			pthread_rwlock_unlock(&m_stream_lock);
			return device_id;
		}
		//cout<<"出入锁2"<<endl;
		pthread_rwlock_unlock(&m_stream_lock);
	}
	pvsm->delete_stream_mem();
	delete pvsm;
	pvsm = NULL;
	return 0;
}
bool video_manger::update_video_stream(unsigned int device_id,unsigned int device_type,string device_name,string ip,unsigned int port,string username,string password,string channel)
{
	pthread_rwlock_wrlock(&m_stream_lock);
	//cout<<"进入锁3"<<endl;
	if(m_manger_map.find(device_id)!=m_manger_map.end())
	{
		m_manger_map.find(device_id)->second->update_by_cmd(device_id,device_type,device_name,ip,port,username,password,channel);
		//cout<<"出入锁3"<<endl;
		pthread_rwlock_unlock(&m_stream_lock);
		return true;
	}
	//cout<<"出入锁3"<<endl;
	pthread_rwlock_unlock(&m_stream_lock);
	return false;
}
bool video_manger::start_video_stream(unsigned int device_id)
{
	pthread_rwlock_wrlock(&m_stream_lock);
	//cout<<"进入锁4"<<endl;
	if(m_manger_map.find(device_id)!=m_manger_map.end())
	{
		if(m_manger_map.find(device_id)->second->start_stream()==0)
		{
			//检查录像计划
			struct tm		*ptm;
			time_t timep = time(NULL)+TIME_OFFSET;
			ptm=localtime(&timep);
			if(m_manger_map.find(device_id)->second->check_next_hour_video_plan(ptm))
			{
				m_manger_map.find(device_id)->second->start_writer();
			}
			//cout<<"出入锁4"<<endl;
			pthread_rwlock_unlock(&m_stream_lock);
			return true;
		}
		else
		{
			cout<<"设备启动失败"<<endl;
		}
		
	}
	else
	{
		cout<<"没有找到设备"<<device_id<<endl;
	}
	//cout<<"出入锁4"<<endl;
	pthread_rwlock_unlock(&m_stream_lock);
	return false;
}
bool video_manger::stop_video_stream(unsigned int device_id)
{
	cout<<"停止视频流"<<device_id<<endl;
	pthread_rwlock_wrlock(&m_stream_lock);
	//cout<<"进入锁5"<<endl;
	if(m_manger_map.find(device_id)!=m_manger_map.end())
	{
		m_manger_map.find(device_id)->second->stop_stream();
		//cout<<"出入锁5"<<endl;
		pthread_rwlock_unlock(&m_stream_lock);
		return true;
	}
	//cout<<"出入锁5"<<endl;
	pthread_rwlock_unlock(&m_stream_lock);
	return false;
}
bool video_manger::delete_video_stream(unsigned int device_id)
{
	cout<<"删除视频流"<<device_id<<endl;
	pthread_rwlock_wrlock(&m_stream_lock);
	//cout<<"进入锁6"<<endl;
	if(m_manger_map.find(device_id)!=m_manger_map.end())
	{
		m_manger_map.find(device_id)->second->stop_stream();
		m_manger_map.find(device_id)->second->delete_stream_mem();
		if(m_manger_map.find(device_id)->second->delete_stream_sql())
		{
			delete m_manger_map.find(device_id)->second;
			m_manger_map.erase(device_id);
			sip_id_device_id_map::iterator	item;
			for(item=m_sip_map_id.begin();item!=m_sip_map_id.end();item++)
			{
				if(item->second==device_id)
				{
					m_sip_map_id.erase(item);
					break;
				}
			}
			//cout<<"出入锁6"<<endl;
			pthread_rwlock_unlock(&m_stream_lock);
			return true;
		}
		
	}
	else
	{
		if(m_manger_reconnect_map.find(device_id)!=m_manger_reconnect_map.end())
		{
			m_manger_reconnect_map.find(device_id)->second->stop_stream();
			m_manger_reconnect_map.find(device_id)->second->delete_stream_mem();
			if(m_manger_reconnect_map.find(device_id)->second->delete_stream_sql())
			{
				delete m_manger_reconnect_map.find(device_id)->second;
				m_manger_reconnect_map.erase(device_id);
				sip_id_device_id_map::iterator	item;
				for(item=m_sip_map_id.begin();item!=m_sip_map_id.end();item++)
				{
					if(item->second==device_id)
					{
						m_sip_map_id.erase(item);
						break;
					}
				}
				//cout<<"出入锁6"<<endl;
				pthread_rwlock_unlock(&m_stream_lock);
				return true;
			}
		}
		
	}
	//cout<<"出入锁6"<<endl;
	pthread_rwlock_unlock(&m_stream_lock);
	return false;
}
bool video_manger::update_video_stream_plan(unsigned int device_id,string plan,unsigned int maxday)
{
	pthread_rwlock_wrlock(&m_stream_lock);
	//cout<<"进入锁7"<<endl;
	if(m_manger_map.find(device_id)!=m_manger_map.end())
	{
		m_manger_map.find(device_id)->second->update_plan_by_cmd(device_id,plan,maxday);
		//cout<<"出入锁7"<<endl;
		pthread_rwlock_unlock(&m_stream_lock);
		return true;
	}
	//cout<<"出入锁7"<<endl;
	pthread_rwlock_unlock(&m_stream_lock);
	return false;
}
bool video_manger::add_dc_dest(unsigned int device_id,string ip,unsigned int port,unsigned int recv_port,string username,string password,string channel)
{
	pthread_rwlock_wrlock(&m_stream_lock);
	//cout<<"进入锁8"<<endl;
	if(m_manger_map.find(device_id)!=m_manger_map.end())
	{
		if(m_manger_map.find(device_id)->second->add_dc_dest(ip,port,recv_port,username,password,channel)==0)
		{
			//cout<<"出入锁8"<<endl;
			pthread_rwlock_unlock(&m_stream_lock);
			return true;
		}
	}
	//cout<<"出入锁8"<<endl;
	pthread_rwlock_unlock(&m_stream_lock);
	return false;
}
int video_manger::add_dest(unsigned int device_id,string ip,unsigned int port)
{
	pthread_rwlock_wrlock(&m_stream_lock);
	//cout<<"进入锁9"<<endl;
	if(m_manger_map.find(device_id)!=m_manger_map.end())
	{
		
		int send_port = m_manger_map.find(device_id)->second->add_dest(ip,port);
		pthread_rwlock_unlock(&m_stream_lock);
		return send_port;
	}
	//cout<<"出入锁9"<<endl;
	pthread_rwlock_unlock(&m_stream_lock);
	return 0;
}
bool video_manger::delete_video_stream_temp(unsigned int device_id)
{
	pthread_rwlock_wrlock(&m_stream_lock);
	//cout<<"进入锁10"<<endl;
	if(m_manger_map.find(device_id)!=m_manger_map.end())
	{
		if(m_manger_map.find(device_id)->second->is_temp())
		{
			m_manger_map.find(device_id)->second->stop_stream();
			m_manger_map.find(device_id)->second->delete_stream_mem();
			delete m_manger_map.find(device_id)->second;
			m_manger_map.erase(device_id);
		}
		//cout<<"出入锁10"<<endl;
		pthread_rwlock_unlock(&m_stream_lock);
		
		return true;
	}
	//cout<<"出入锁10"<<endl;
	pthread_rwlock_unlock(&m_stream_lock);
	return false;
}
bool video_manger::del_dc_dest(unsigned int device_id,string ip,unsigned int recv_port,string channel)
{
	pthread_rwlock_wrlock(&m_stream_lock);
	//cout<<"进入锁11"<<endl;
	if(m_manger_map.find(device_id)!=m_manger_map.end())
	{
		m_manger_map.find(device_id)->second->del_dc_dest(ip,recv_port,channel);
		//cout<<"出入锁11"<<endl;
		pthread_rwlock_unlock(&m_stream_lock);
		return true;
	}
	//cout<<"出入锁11"<<endl;
	pthread_rwlock_unlock(&m_stream_lock);
	return false;
}
bool video_manger::del_dest(unsigned int device_id,string ip,unsigned int port)
{
	pthread_rwlock_wrlock(&m_stream_lock);
	//cout<<"进入锁12"<<endl;
	/*
	if(m_manger_map.find(device_id)!=m_manger_map.end())
	{
	int ret = m_manger_map.find(device_id)->second->del_dest(ip,port);
	pthread_rwlock_unlock(&m_stream_lock);
	if(ret==1)
	return true;
	else
	return false;
	}
	*/
	if(m_manger_map.size()<=0)
	{
		//cout<<"出入锁12"<<endl;
		pthread_rwlock_unlock(&m_stream_lock);
		return false;
	}
	stream_manger_map::iterator item = m_manger_map.begin();
	for (;item!=m_manger_map.end();item++)
	{
		item->second->del_dest(ip,port);
	}
	//cout<<"出入锁12"<<endl;
	pthread_rwlock_unlock(&m_stream_lock);
	return true;
}
int video_manger::get_id_from_sip_id(string sip_id)
{
	//cout<<"查询设备ID"<<sip_id<<"设备表总共有"<<m_sip_map_id.size()<<endl;
	if(m_sip_map_id.find(sip_id)!=m_sip_map_id.end())
	{
		return m_sip_map_id.find(sip_id)->second;
	}
	return -1;
}

bool video_manger::get_idx_buffer(unsigned int device_id,char *buffer,unsigned int len)
{
	pthread_rwlock_wrlock(&m_stream_lock);
	//cout<<"进入锁13"<<endl;
	if(m_manger_map.find(device_id)!=m_manger_map.end())
	{
		m_manger_map.find(device_id)->second->get_idx_buffer(buffer,len);
		//cout<<"出入锁13"<<endl;
		pthread_rwlock_unlock(&m_stream_lock);
		return true;
	}
	//cout<<"出入锁13"<<endl;
	pthread_rwlock_unlock(&m_stream_lock);
	return false;
}
bool video_manger::prepair_file_block(unsigned int device_id)
{
	file_list::iterator			file_item;
	stream_manger_map::iterator item;
	pthread_rwlock_wrlock(&m_stream_lock);
	//cout<<"进入锁14"<<endl;
	item = m_manger_map.find(device_id);
	if(item!=m_manger_map.end())
	{
		if(m_pripair_file_list.empty())
		{
			g_sm->recycle_file_block(m_manger_map.size());
			cout<<"回收文件块"<<m_manger_map.size()<<"个"<<endl;
			g_sm->get_free_block(m_manger_map.size(), m_pripair_file_list);
		}
		file_item = m_pripair_file_list.begin();
		if(item->second->pre_file((char *)(*file_item).file_path.c_str(),(*file_item).file_id))
		{
			cout<<"为视频"<<item->second->m_video_id<<"分配一个文件块"<<(*file_item).file_path<<"文件号"<<(*file_item).file_id<<endl;
			m_pripair_file_list.pop_front();
		}
	}
	//cout<<"出入锁14"<<endl;
	pthread_rwlock_unlock(&m_stream_lock);
	return true;
}
bool video_manger::have_shot(unsigned int device_id)
{
	stream_manger_map::iterator item;
	pthread_rwlock_wrlock(&m_stream_lock);
	//cout<<"进入锁15"<<endl;
	item = m_manger_map.find(device_id);
	if(item!=m_manger_map.end())
	{
		item->second->have_a_short();
		//cout<<"出入锁15"<<endl;
		pthread_rwlock_unlock(&m_stream_lock);
		
		return true;
	}
	//cout<<"出入锁15"<<endl;
	pthread_rwlock_unlock(&m_stream_lock);
	return false;
}
void * video_manger::thread_check_device_online(void * lp)
{
	//这里还有同时检测时间是否被修改
	video_manger *pvm = (video_manger *)lp;
	cout<<"启动设备在线检查线程"<<endl;
	time_t	last_sec = time(NULL)+TIME_OFFSET;
	while(pvm->m_brun)
	{
		time_t cur = time(NULL)+TIME_OFFSET;
		if(cur-last_sec>10)
		{
			//系统时间被修改
			sleep(5);
			last_sec = time(NULL)+TIME_OFFSET;
			continue;
		}
		if(last_sec!=cur)
		{
			last_sec = cur;
		}
		pthread_rwlock_wrlock(&pvm->m_stream_lock);
		//cout<<"进入锁16"<<endl;
		stream_manger_map::iterator item;
		//cout<<"系统时间"<<last_sec<<endl;
		for (item=pvm->m_manger_map.begin();item!=pvm->m_manger_map.end();)
		{
			
			if(last_sec - item->second->get_last_time()>10&&last_sec - item->second->get_last_time()<1000)
			{
				
				//视频停止
				cout<<"上次接受数据的时间"<<item->second->get_last_time()<<endl;
				item->second->stop_recv_stream();
				cout<<item->second->m_device_name<<"设备丢失，停止设备，加入重连列表"<<endl;
				//移入重连列表
				pvm->m_manger_reconnect_map.insert(map<unsigned int,video_stream_manger *>::value_type(item->first,item->second));
				//移除正常列表
				pvm->m_manger_map.erase(item++);
				continue;
			}

			item++;
		}
		//cout<<"出入锁16"<<endl;
		pthread_rwlock_unlock(&pvm->m_stream_lock);
		sleep(1);
	}
	cout<<"设备在线检查线程结束"<<endl;
	return 0;
}
void * video_manger::thread_check_prepair_fileblock(void * lp)
{
	video_manger *pvm = (video_manger *)lp;
	cout<<"启动文件块预分配线程"<<endl;
	while(pvm->m_brun)
	{
		int stream_num = pvm->m_manger_map.size()+2;
		if(pvm->m_pripair_file_list.size()<stream_num)
		{
			int nget = stream_num-pvm->m_pripair_file_list.size();
			int nval = g_sm->get_free_block(stream_num-pvm->m_pripair_file_list.size(), pvm->m_pripair_file_list);
			if(nval<nget)
			{
				g_sm->recycle_file_block(nget-nval);
				cout<<"回收文件块"<<nget-nval<<"个"<<endl;
				g_sm->get_free_block(stream_num-pvm->m_pripair_file_list.size(), pvm->m_pripair_file_list);

			}
		}
		
		if(pvm->m_pripair_file_list.size()<stream_num)
		{
			int nget = stream_num-pvm->m_pripair_file_list.size();
			g_sm->recycle_file_block(stream_num-nget+1);
			cout<<"回收文件块"<<stream_num-nget<<"个"<<endl;
			g_sm->get_free_block(stream_num-pvm->m_pripair_file_list.size(), pvm->m_pripair_file_list);
			if(pvm->m_pripair_file_list.size()<stream_num)
			{
				cout<<"文件回收错误，线程退出，并程序退出"<<pvm->m_pripair_file_list.size()<<"-"<<stream_num<<endl;
				pvm->stop_video();
				exit(1);
			}
		}
		file_list::iterator			file_item;
		stream_manger_map::iterator item;
		pthread_rwlock_wrlock(&pvm->m_stream_lock);
		for (item=pvm->m_manger_map.begin();item!=pvm->m_manger_map.end();item++)
		{
			if(pvm->m_pripair_file_list.size())
			{
				file_item = pvm->m_pripair_file_list.begin();
				if(item->second->pre_file((char *)(*file_item).file_path.c_str(),(*file_item).file_id))
				{
					cout<<"为视频"<<item->second->m_video_id<<"分配一个文件块"<<(*file_item).file_path<<"文件号"<<(*file_item).file_id<<endl;
					pvm->m_pripair_file_list.pop_front();
				}
			}
			else
			{
				cout<<"没有文件块可供分配"<<endl;
			}
		}
		pthread_rwlock_unlock(&pvm->m_stream_lock);
		sleep(15);
	}
	cout<<"文件块预分配线程结束"<<endl;
	return 0;
}
void * video_manger::thread_check_video_recycle(void * lp)
{
	//暂时不用 文件回收主要用来回收超过保存最大天数的视频
	//预分配文件块中会主动进行最老文件块回收
	video_manger *pvm = (video_manger *)lp;
	cout<<"启动文件块回收线程"<<endl;
	while(pvm->m_brun)
	{
		sleep(100);
	}
	cout<<"文件块回收线程停止"<<endl;
	return 0;
}
void * video_manger::thread_keep_device_alive(void *lp)
{
	video_manger *pvm = (video_manger *)lp;
	cout<<"启动设备连接维持线程"<<endl;
	while(pvm->m_brun)
	{
		cout<<"发送心跳时间"<<time(NULL)<<endl;
		pthread_rwlock_wrlock(&pvm->m_stream_lock);
		stream_manger_map::iterator item;
		for (item=pvm->m_manger_map.begin();item!=pvm->m_manger_map.end();item++)
		{
			item->second->keepalive();
			//cout<<item->second->m_device_name<<"发送心跳,端口"<<item->second->m_recv_port<<endl;
			item->second->update_transfer_map();
			usleep(1000);
		}
		for (item=pvm->m_manger_reconnect_map.begin();item!=pvm->m_manger_reconnect_map.end();item++)
		{
			//重连列表中不发送心跳 ，时间继续更新
			item->second->update_time();
		}
		pthread_rwlock_unlock(&pvm->m_stream_lock);
		sleep(15);
	}
	cout<<"设备连接维持线程结束"<<endl;
	return 0;
}
void * video_manger::thread_check_video_plan(void *lp)
{
	video_manger *pvm = (video_manger *)lp;
	cout<<"启动录像计划检测线程"<<endl;
	bool first_check = true;
	while(pvm->m_brun)
	{
		struct tm		*ptm;
		time_t timep = time(NULL)+TIME_OFFSET;
		ptm=localtime(&timep);

		stream_manger_map::iterator item;
		//cout<<"当前分钟"<<ptm->tm_min<<endl;
		if(ptm->tm_min==59||first_check)
		{
			while(ptm->tm_sec<58&&first_check==false)
			{
				//cout<<"当前秒"<<ptm->tm_sec<<endl;
				sleep(1);
				timep = time(NULL)+TIME_OFFSET;
				ptm=localtime(&timep);
			}
			cout<<"开始检查录像计划"<<endl;
			pthread_rwlock_wrlock(&pvm->m_stream_lock);
			cout<<"总共检查"<<pvm->m_manger_map.size()<<"路视频的录像计划"<<endl;
			for (item=pvm->m_manger_map.begin();item!=pvm->m_manger_map.end();item++)
			{
				cout<<"检查设备"<<item->second->m_video_id<<"的录像计划"<<endl;
				if(item->second->check_next_hour_video_plan(ptm))
				{
					cout<<"设备"<<item->second->m_video_id<<"启动录像"<<endl;
					item->second->start_writer();
				}
				else
				{
					cout<<"设备"<<item->second->m_video_id<<"停止录像"<<endl;
					item->second->stop_writer();
				}
			}
			pthread_rwlock_unlock(&pvm->m_stream_lock);
			cout<<"结束检查录像计划"<<endl;
			first_check = false;
		}
		sleep(20);

	}
	cout<<"录像计划检测线程结束"<<endl;
	return 0;
}
void * video_manger::thread_take_short(void *lp)
{
	video_manger *pvm = (video_manger *)lp;
	cout<<"启动视频实时截图线程"<<endl;
	list<unsigned int>::iterator item;
	while(pvm->m_brun)
	{
		if(pvm->m_shot_task_list.size())
		{
			unsigned int vid = *(item);
			cout<<"开始截取视频"<<vid<<"图片"<<endl;
		}
		else
		{
			sleep(1);
		}
		
	}
	cout<<"视频实时截图线程结束"<<endl;
}
void * video_manger::thread_reconnect(void *lp)
{
	video_manger *pvm = (video_manger *)lp;
	cout<<"启动设备重连线程"<<endl;
	while(pvm->m_brun)
	{
		pthread_rwlock_wrlock(&pvm->m_stream_lock);
		stream_manger_map::iterator item;
		for (item=pvm->m_manger_reconnect_map.begin();item!=pvm->m_manger_reconnect_map.end();)
		{
			cout<<"开始重连"<<item->second->m_device_addr<<" 通道"<<item->second->m_device_channel<<endl;
			int ret = item->second->start_recv_stream();
			if(ret==0)
			{
				//移入正常列表
				//检查录像计划
				struct tm		*ptm;
				time_t timep = time(NULL)+TIME_OFFSET;
				ptm=localtime(&timep);
				if(item->second->check_next_hour_video_plan(ptm))
				{
					item->second->start_writer();
				}
				pvm->m_manger_map.insert(map<unsigned int,video_stream_manger *>::value_type(item->first,item->second));
				//移除重连列表
				pvm->m_manger_reconnect_map.erase(item++);
				continue;
			}
			item++;
		}
		pthread_rwlock_unlock(&pvm->m_stream_lock);
		sleep(10);
	}
}


unsigned short	video_manger::add_transit_reciver(string request)
{
		transit_manger_map::iterator	item;
		item = m_transit_map.find(request);
		if(item==m_transit_map.end())
		{
			video_transit_manger 	*pvtm = new video_transit_manger();
			int recv_port =  pvtm->create_transit(request,get_free_temp_device_id());;
			if(recv_port>0)
			{
					m_transit_map.insert(map<string,video_transit_manger *>::value_type(request,pvtm));
					return recv_port;
			}
			cout<<"端口分配失败"<<endl;
			return 0;
		}
		else
		{
			if(item->second->m_transfer_map.size()==0)
			{
					delete item->second;
					m_transit_map.erase(item);
					video_transit_manger 	*pvtm = new video_transit_manger();
					int recv_port =  pvtm->create_transit(request,get_free_temp_device_id());;
					if(recv_port>0)
					{
							m_transit_map.insert(map<string,video_transit_manger *>::value_type(request,pvtm));
							return recv_port;
					}
			}
		}
		cout<<"设备已经存在"<<m_transit_map.size()<<endl;
		return 0;
}
bool video_manger::del_transit_reciver(string request)
{
		transit_manger_map::iterator	item;
		item = m_transit_map.find(request);
		if(item!=m_transit_map.end())
		{
			item->second->delete_transit();
			m_transit_map.erase(item);
			return true;
		}
		return false;
}
unsigned short	video_manger::add_transit_dest(string request,string dest_ip,unsigned short dest_port)
{
		transit_manger_map::iterator	item;
		item = m_transit_map.find(request);
		if(item!=m_transit_map.end())
		{
			return item->second->add_dest(dest_ip,dest_port);
		}
		return 0;
}
bool video_manger::del_transit_dest(string request,string dest_ip,unsigned short dest_port)
{
		transit_manger_map::iterator	item;
		item = m_transit_map.find(request);
		if(item!=m_transit_map.end())
		{
			return item->second->del_dest(dest_ip,dest_port);
		}
		return false;
}