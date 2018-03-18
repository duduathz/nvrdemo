#include "data_transfer.h"
#include "Units.h"
#include "sip_manger.h"
#include "video_manger.h"
extern sip_manger			*g_sipm;
extern unsigned int			g_btrans_thread_num;
extern video_manger			*g_vm;
data_transfer::data_transfer(void)
{
	m_preader = NULL;
	m_pblock_queue = NULL;
	m_breal = false;
	m_brun = false;
	m_ncount = 0;
	m_block_mem.block_pointer = (char *)malloc(BLOCK_CACHE_SIZE);
	m_block_mem.block_size = BLOCK_CACHE_SIZE;
	pthread_rwlock_init(&m_rw_lock,NULL);
	m_bpause = false;
	m_nspeed = 1;
	m_bprint = false;
	m_sub_block = NULL;
	m_send_thread = 0;
}

data_transfer::~data_transfer(void)
{
	
	
	if(m_brun)
	{
		stop_transfer();
	}
	if(m_sub_block)
	{
		delete m_sub_block;
		m_sub_block = NULL;
	}
	free(m_block_mem.block_pointer);
	m_block_mem.block_pointer = NULL;
	pthread_rwlock_destroy(&m_rw_lock);
	
}
bool data_transfer::init_transfer_for_reader(data_reader *preader,string sip_id,string ip,unsigned short port,int did,bool download)
{
	m_preader = preader;
	m_breal = false;
	m_did= did;
	m_sip_id= sip_id;
	if(download)
		m_nspeed = 32;
	sockaddr_in		addr;
	char	portstr[32];
	sprintf(portstr,"%d",port);
	string key = ip+string(portstr);
	pthread_rwlock_wrlock(&m_rw_lock);
	if(m_dest_map.find(key)==m_dest_map.end())
	{
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = inet_addr(ip.c_str());
		m_dest_map.insert(map<string,sockaddr_in>::value_type(key,addr));
		pthread_rwlock_unlock(&m_rw_lock);
		uint64_t	len = preader->get_reader_length();
		cout<<"获取录像的长度为"<<len<<endl;
		g_sipm->build_message_file_length(m_did,sip_id,len);
		return true;
	}
	pthread_rwlock_unlock(&m_rw_lock);
	return false;
}
bool data_transfer::init_transfer_for_realvideo(block_queue *pbq,string sip_id)
{
	m_pblock_queue = pbq;	
	m_breal = true;
	m_sip_id = sip_id;
	return true;
}
bool data_transfer::init_transfer_for_realvideo2(zmq_block_pub *pbq,string sip_id)
{
	m_pbq = pbq;
	if(m_sub_block)
	{
		delete m_sub_block;
		m_sub_block = NULL;
	}
	//m_sub_block = new zmq_block_sub(pbq);	
	m_breal = true;
	m_sip_id = sip_id;
	return true;
}
bool data_transfer::is_real_transfer()
{
	return m_breal;
}
size_t	 data_transfer::dest_size()
{
	return m_dest_map.size();
}
bool data_transfer::update_time()
{
	m_time_update = time(NULL)+TIME_OFFSET;
	return true;
}

bool data_transfer::add_dest_addr_for_transfer(string key,sockaddr_in addr)
{
	pthread_rwlock_wrlock(&m_rw_lock);
	if(m_dest_map.find(key)==m_dest_map.end())
	{
		m_dest_map.insert(map<string,sockaddr_in>::value_type(key,addr));
	}
	pthread_rwlock_unlock(&m_rw_lock);
}
bool data_transfer::add_dest_addr_for_realvideo(string ip,unsigned short port)
{
	sockaddr_in		addr;
	char	portstr[32];
	sprintf(portstr,"%d",port);
	string key = ip+string(portstr);
	pthread_rwlock_wrlock(&m_rw_lock);
	m_dest_map.clear();
	if(m_dest_map.find(key)==m_dest_map.end())
	{
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = inet_addr(ip.c_str());
		m_dest_map.insert(map<string,sockaddr_in>::value_type(key,addr));
		pthread_rwlock_unlock(&m_rw_lock);
		m_dest_ip = ip;
		m_dest_port = port;
		return true;
	}
	pthread_rwlock_unlock(&m_rw_lock);
	return false;

}
bool data_transfer::del_dest_addr_for_realvideo(string ip,unsigned short port)
{
	char	portstr[32];
	sprintf(portstr,"%d",port);
	string key = ip+string(portstr);
	pthread_rwlock_wrlock(&m_rw_lock);
	m_dest_map.erase(key);
	pthread_rwlock_unlock(&m_rw_lock);
	return false;
}
int data_transfer::start_transfer()
{
	m_brun = true;
	int port = get_free_port();
	
	if(port>0)
	{
	
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	m_send_sock = socket(AF_INET, SOCK_DGRAM, 0);
	
	if (bind(m_send_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		cout<<"端口绑定失败"<<endl;
		close(m_send_sock);
		return 0;
	}
	
	cout<<"发送端口为"<<port<<"socket"<<m_send_sock<<endl;
	int size = 0;
	if (setsockopt(m_send_sock,SOL_SOCKET,SO_SNDBUF,(const char *)&size,sizeof(int)) != 0)
	{
		close(m_send_sock);
		return 0;
	}
	m_sub_block = new zmq_block_sub(m_pbq);	
	m_time_update = time(NULL)+TIME_OFFSET;
	if(m_breal)
		pthread_create(&m_send_thread,NULL,real_send_thread2,this);
	else
		pthread_create(&m_send_thread,NULL,data_send_thread,this);
	}
	return port;
}
bool data_transfer::stop_transfer()
{
	int num = 0;
	if(m_brun)
	{
		m_brun = false;
		/*
		//pthread_join(m_thread_send,NULL);
		usleep(1000);
		if(m_send_thread)
		{
			int pthread_kill_err = pthread_kill(m_send_thread,0);
			if(pthread_kill_err!=ESRCH&&pthread_kill_err!=EINVAL&&pthread_kill_err==0)
			{	
				while(pthread_kill_err==0)
				{
					m_brun = false;
					usleep(1000);
					if(m_send_thread)
					{
						pthread_kill_err = pthread_kill(m_send_thread,0);
					}
					else
					{
						break;
					}
					num++;
					if(num%10==0)
					{
						cout<<num<<"线程还未结束"<<endl;
						//break;
					}
				}
			}
		}*/
		while(m_send_thread)
		{
			m_brun = false;
			usleep(120000);
			num++;
			//if(num%10==0)
			{
				cout<<num<<"线程还未结束"<<endl;
				//break;
			}
			if(num>10)
			{
				break;
			}
		}
	}
	if(m_send_sock)
	{
		close(m_send_sock);
		m_send_sock = 0;
	}
	if(m_sub_block)
	{
		delete m_sub_block;
		m_sub_block = NULL;
	}
	m_brun = false;
	return true;
}
bool data_transfer::pause_transfer()
{
	m_bpause = true;
	return m_bpause;
}
bool data_transfer::speed_transfer(float nspeed)
{
	m_nspeed = nspeed;
	m_bpause = false;
	return false;
}
void * data_transfer::real_send_thread(void *lp)
{
	char	*psend;
	block	pb;
	data_transfer *pdt = (data_transfer *)lp;
	char	*ptotal = pdt->m_pblock_queue->get_buffer();
	unsigned int last_offset=0;
	dest_map::iterator	item=pdt->m_dest_map.begin();
	unsigned int last_send_offset = 0;
	if(pdt->m_breal)
	{
		pdt->m_ncount = pdt->m_pblock_queue->get_max_count();
		while(pdt->m_brun)
		{
			if(time(NULL)+TIME_OFFSET-pdt->m_time_update>45)
			{
				pdt->m_brun = false;
				cout<<"通道管理异常,停止发送数据"<<endl;
				g_vm->del_dest(0,pdt->m_dest_ip,pdt->m_dest_port);
				//delete pdt;
				return NULL;
			}
			int len = pdt->m_pblock_queue->get_block_by_count(&pb,last_offset,pdt->m_ncount);
			psend = ptotal+pb.offset;
			last_offset = pb.offset;
			pdt->m_ncount = pb.count;
			if(last_send_offset==last_offset)
			{
				usleep(10);
				continue;
			}
			last_send_offset = last_offset;
			if (len>0)
			{
				//在这里给所有目标发送数据
				//cout<<"发送数据长度"<<len<<"偏移量"<<last_offset<<":"<<last_send_offset<<endl;
				last_offset+=len;
				pthread_rwlock_wrlock(&pdt->m_rw_lock);
				item = pdt->m_dest_map.begin();
				for (;item!=pdt->m_dest_map.end();item++)
				{
					int nData = sendto(pdt->m_send_sock,psend,len,0,(struct sockaddr *)&item->second, sizeof(item->second));
					if(nData<=0)
					{
						cout<<"发送数据失败错误号"<<errno<<"socket"<<pdt->m_send_sock<<"长度为"<<len<<"偏移量"<<pb.offset<<"---"<<item->first<<endl;
						if(pdt->m_send_sock==0||errno==22)
						{
							return NULL;
						}
					}
				}
				pthread_rwlock_unlock(&pdt->m_rw_lock);	
			}
			else
			{
				if(len==0)
				{
					//cout<<"没有新数据"<<len<<"偏移量"<<last_offset<<"编号"<<pdt->m_ncount<<endl;
					usleep(1);
				}
				else
				{
					cout<<"可能是发送数据太慢造成"<<pdt->m_sip_id<<endl;
					//usleep(10);
				}
			}
		}
		pdt->m_brun = false;
	}
	pdt->m_send_thread = 0;
	return NULL;
}
void * data_transfer::real_send_thread2(void *lp)
{
	char	psend[4096];
	int		timeout=0;
	int		reconnect_num = 0;
	data_transfer *pdt = (data_transfer *)lp;
	dest_map::iterator	item=pdt->m_dest_map.begin();
	g_btrans_thread_num++;
	cout<<"当前转发线程数量为"<<g_btrans_thread_num<<endl;
	pdt->m_brun = true;
	if(pdt->m_breal)
	{
		while(pdt->m_brun)
		{
			if(time(NULL)+TIME_OFFSET-pdt->m_time_update>45&&timeout>10)
			{
				pdt->m_brun = false;
				cout<<"通道管理异常,停止发送数据"<<pdt->m_dest_ip<<"_"<<pdt->m_dest_port<<endl;
				g_btrans_thread_num--;
				g_vm->del_dest(0,pdt->m_dest_ip,pdt->m_dest_port);
				//delete pdt;
				return NULL;
			}
			if(pdt->m_sub_block==NULL)
			{
				break;
			}
			int len = pdt->m_sub_block->get_buf(psend,4096);
			if (len>0)
			{
				reconnect_num = 0;
				timeout = 0;
				if(len<=4096)
				{
					//在这里给所有目标发送数据

					pthread_rwlock_wrlock(&pdt->m_rw_lock);
					item = pdt->m_dest_map.begin();
					for (;item!=pdt->m_dest_map.end();item++)
					{
						int nData = sendto(pdt->m_send_sock,psend,len,0,(struct sockaddr *)&item->second, sizeof(item->second));
						if(nData<=0)
						{
							cout<<"发送数据失败错误号"<<errno<<"socket"<<pdt->m_send_sock<<"长度为"<<len<<"---"<<item->first<<pdt->m_sub_block->zmq_name<<endl;
							if(pdt->m_send_sock==0||errno==22 ||errno==88)
							{
								pdt->m_brun = false;
								pthread_rwlock_unlock(&pdt->m_rw_lock);	
								g_btrans_thread_num--;
								return NULL;
							}
						}
					}
					pthread_rwlock_unlock(&pdt->m_rw_lock);	
				}
				else
				{
					cout<<"发送数据太慢 导致数据堆积 并且丢失"<<endl;
				}

			}
			else
			{
					timeout++;
					/* 这里是为了设备断开后不停止线程，等待线程重连后，继续向客户端发送数据
					timeout++;
					if(timeout>10)
					{
						cout<<"转发获取数据超时,停止转发"<<endl;
						break;
					}
					*/
					//这个太大会操作视频太卡
					usleep(10000);
			}
		}
		pdt->m_brun = false;
	}
	cout<<"目标为"<<pdt->m_dest_ip<<":"<<pdt->m_dest_port<<"发送线程退出"<<endl;
	g_btrans_thread_num--;
	pdt->m_send_thread = 0;
	return NULL;
}
void * data_transfer::data_send_thread(void *lp)
{
	data_transfer *pdt = (data_transfer *)lp;
	int offset = 0;
	int find_offset = 0;
	int send_pos = 0;
	unsigned int pmt_id = 0x62;
	uint64_t	last_send_hz = 0;
	timeval		last_send_time;
	unsigned int last_pid = 0;
	dest_map::iterator	item=pdt->m_dest_map.begin();
	while(pdt->m_brun)
	{
		int len = pdt->m_preader->getbuf(pdt->m_block_mem.block_pointer+offset,BLOCK_CACHE_SIZE-offset);
		if(len==0)
		{
			cout<<"录像结束"<<endl;
			g_sipm->build_message_file_end(pdt->m_did,pdt->m_sip_id);
			break;
		}
		while(pdt->m_bpause)
		{
			sleep(1);
			if(pdt->m_brun==false)
			{
				break;
			}
		}
		//cout<<"读取文件长度 "<<len<<endl;
		offset += len;

		int pos = -1;
		do 
		{
			if(pdt->m_brun==false)
			{
				break;
			}
			pos	= SearchFirstTSStartCode((const unsigned char *)pdt->m_block_mem.block_pointer+find_offset,offset-find_offset);
			if(pos<0)
			{
				if(find_offset-send_pos>0)
				{
					//在这里给所有目标发送数据
					int nData = sendto(pdt->m_send_sock,pdt->m_block_mem.block_pointer+send_pos,find_offset-send_pos,0,(struct sockaddr *)&item->second, sizeof(item->second));
					send_pos=find_offset;
				}
				break;
			}
			//这里需要再查找问题 重新读取文件块会是的pos位置不为0
			find_offset+=pos;
			if(pos!=0)
			{
				cout<<"查找的位置"<<pos<<"----"<<find_offset<<endl;
			}
			unsigned start_indicator = pdt->m_block_mem.block_pointer[find_offset+pos+1]>>6&0x01;
			unsigned int pid = (pdt->m_block_mem.block_pointer[find_offset+pos+1]&0x1F)<<8|pdt->m_block_mem.block_pointer[find_offset+pos+2];
			/*
			if(pid==0)
			{
				if(find_offset-send_pos>0)
				{
					//在这里给所有目标发送数据
					int nData = sendto(pdt->m_send_sock,pdt->m_block_mem.block_pointer+send_pos,find_offset-send_pos,0,(struct sockaddr *)&item->second, sizeof(item->second));
					send_pos=find_offset;
				}
				usleep(33000/pdt->m_nspeed);
				find_offset+=188;
				continue;
			}
			if(pid==pmt_id)
			{
				find_offset+=188;
				last_pid = pid;
				continue;
			}
			if(start_indicator&&last_pid!=pmt_id)*/
			if(start_indicator)
			{
				if(find_offset-send_pos>0)
				{
					//在这里给所有目标发送数据
					int nData = sendto(pdt->m_send_sock,pdt->m_block_mem.block_pointer+send_pos,find_offset-send_pos,0,(struct sockaddr *)&item->second, sizeof(item->second));
					send_pos=find_offset;
				}

				int afc = (pdt->m_block_mem.block_pointer[find_offset+pos+3]&0x30)>>4;
				int afcv = pdt->m_block_mem.block_pointer[find_offset+pos+5];


				//分析PES 得出时间 并进行控制 只分析视频
				if(afc==0x03&&afcv<=31&&afcv>0)//包含调整字段
				{
					uint64_t pcr_base = ReadInt64((unsigned char *)pdt->m_block_mem.block_pointer+find_offset+pos+6);
					pcr_base = pcr_base>>16;
					uint64_t pcr_f = pcr_base&0xFFFFFFFF8000;	 //111111111111111111111111111111111000000000000000
					uint64_t pcr_b = pcr_base&0x1FF;
					pcr_f = pcr_f>>6;
					uint64_t pcr = pcr_f&pcr_b;
					if(last_send_hz==0)
					{
						last_send_hz = pcr_f;
						gettimeofday(&last_send_time,NULL);
					}
					else
					{
						int s_us = (pcr_f - last_send_hz)/27;
						timeval	tv;
						gettimeofday(&tv,NULL);						
						int o_us = (tv.tv_sec-last_send_time.tv_sec)*1000000+tv.tv_usec-last_send_time.tv_usec;
						if(s_us-o_us<100000)
						{
							if(o_us<s_us)
							{
								usleep((s_us-o_us)/(2*pdt->m_nspeed));
								//cout<<"时间间隔"<<(s_us-o_us)/(2*pdt->m_nspeed)<<endl;
							}
							else
							{
								usleep(33000/pdt->m_nspeed);
								//cout<<"时间间隔"<<(s_us-o_us)/(2*pdt->m_nspeed)<<endl;
							}
							last_send_hz = pcr_f;
							gettimeofday(&last_send_time,NULL);
						}
						else
						{
							usleep(33000/pdt->m_nspeed);
							last_send_hz = pcr_f;
							gettimeofday(&last_send_time,NULL);
						}
					}

				}

			}
			else
			{
				if(find_offset-send_pos>1300)
				{
					//在这里给所有目标发送数据
					int nData = sendto(pdt->m_send_sock,pdt->m_block_mem.block_pointer+send_pos,find_offset-send_pos,0,(struct sockaddr *)&item->second, sizeof(item->second));
					send_pos=find_offset;
				}				
			}
			last_pid = pid;
			find_offset+=188;

		} while (pos>=0);
		memmove(pdt->m_block_mem.block_pointer,pdt->m_block_mem.block_pointer+find_offset,offset - find_offset);

		offset = offset - find_offset;
		find_offset = 0;
		send_pos = 0;
		//cout<<"内存移动 "<<offset<<endl;

	}
	pdt->m_brun = false;
	pdt->m_send_thread = 0;
	return NULL;
}