#include "block_queue.h"
#include "bits.h"
#include "Units.h"
block_queue::block_queue(void)
{
	total_pointer = 0;
	new_offset = 0;
	m_list.clear();
	pthread_rwlock_init(&m_rw_lock,NULL);
	pthread_spin_init(&m_spin_lock,1);
	 
}

block_queue::~block_queue(void)
{
	pthread_rwlock_destroy(&m_rw_lock);
	pthread_spin_destroy(&m_spin_lock);
}
bool block_queue::init_block_queue(unsigned int nsize,string name,unsigned int start_code,unsigned int start_code_size)
{
	total_pointer = (char *)malloc(nsize);
	cout<<"初始化内存块队列"<<endl;
	if(total_pointer)
	{
		cout<<"初始化内存块队列成功"<<endl;
		total_size = nsize;
		new_offset = 0;
		block_name = name;
		m_start_code = start_code;
		m_counter = 0;
		m_start_code_size = start_code_size;
		m_last_offset =total_size;
		m_nLaps = 0;
		return true;
	}

	return false;
}
bool block_queue::insert_block_buf(char *buffer,unsigned int len)
{
	int size = sizeof(unsigned int);
	//pthread_rwlock_wrlock(&m_rw_lock);
	pthread_spin_lock(&m_spin_lock);
	bits_buffer_s	bbs;
	//char	str[128];


	if(new_offset+len+size+m_start_code_size>total_size)
	{
		//内存拐歪的时候使用
		//OutputDebugStringA("内存拐弯了\n");
		m_last_offset = new_offset;
		new_offset = 0; 
		recycle_block(new_offset,new_offset+len+size+m_start_code_size);
		//m_list.push_back(new_offset);
		m_counter+=1;
		m_map.insert(map<uint64_t,unsigned int>::value_type(m_counter,new_offset));

		bits_initwrite(&bbs,size+m_start_code_size,total_pointer+new_offset);
		bits_write(&bbs,size*8,len+m_start_code_size);
		if(m_start_code_size)
			bits_write(&bbs,m_start_code_size*8,m_start_code);
		//sprintf_s(str,"写入的内存偏移量为%d 长度为 %d \n",new_offset,len+m_start_code_size);
		//OutputDebugStringA(str);
		new_offset+=m_start_code_size;
		new_offset+=size;
		memcpy(total_pointer+new_offset,buffer,len);
		new_offset += len;
		//pthread_rwlock_unlock(&m_rw_lock);
		pthread_spin_unlock(&m_spin_lock);
		m_new_offset = 0;
		m_nLaps++;
		return true;

	}
	else if(new_offset+len+size+m_start_code_size==total_size)
	{
		m_last_offset = total_size;
		int offs = new_offset;
		//OutputDebugStringA("内存将要拐弯了\n");
		recycle_block(new_offset,new_offset+len+size+m_start_code_size);
		//m_list.push_back(new_offset);
		m_counter+=1;
		m_map.insert(map<uint64_t,unsigned int>::value_type(m_counter,new_offset));
		bits_initwrite(&bbs,size,total_pointer+new_offset);
		bits_write(&bbs,size*8,len+m_start_code_size);
		if(m_start_code_size)
			bits_write(&bbs,m_start_code_size*8,m_start_code);
		//sprintf_s(str,"写入的内存偏移量为%d 长度为 %d \n",new_offset,len+m_start_code_size);
		//OutputDebugStringA(str);
		new_offset+=m_start_code_size;
		new_offset+=size;
		memcpy(total_pointer+new_offset,buffer,len);
		new_offset = 0;
		//pthread_rwlock_unlock(&m_rw_lock);
		pthread_spin_unlock(&m_spin_lock);
		m_new_offset = offs;
		m_nLaps++;
		return true;
	}
	else
	{
		int offs = new_offset;
		recycle_block(new_offset,new_offset+len+size+m_start_code_size);
		//m_list.push_back(new_offset);
		m_counter+=1;
		m_map.insert(map<uint64_t,unsigned int>::value_type(m_counter,new_offset));
		bits_initwrite(&bbs,size,total_pointer+new_offset);
		bits_write(&bbs,size*8,len+m_start_code_size);
		if(m_start_code_size)
			bits_write(&bbs,m_start_code_size*8,m_start_code);
		unsigned int lenret;
		lenret = ReadInt32((unsigned char *)total_pointer+new_offset);

		//sprintf_s(str,"写入的内存偏移量为%d 长度为 %d   %d \n",new_offset,lenret,len+m_start_code_size);
		//OutputDebugStringA(str);
		new_offset+=m_start_code_size;
		new_offset+=size;
		memcpy(total_pointer+new_offset,buffer,len);
		new_offset +=len;
		//pthread_rwlock_unlock(&m_rw_lock);
		pthread_spin_unlock(&m_spin_lock);
		m_new_offset = offs;
		return true;
	}
	//pthread_rwlock_unlock(&m_rw_lock);
	pthread_spin_unlock(&m_spin_lock);
	return false;
}
uint64_t block_queue::get_max_count()
{
	return m_counter;
}
unsigned int block_queue::get_block_by_count(block_mem *pbm,uint64_t count)
{
	offsetmap::iterator item;
	int size = sizeof(unsigned int);
	pthread_spin_lock(&m_spin_lock);
	
	if(count)
		item = m_map.find(count);
	else
		item = m_map.begin();

	if(item!=m_map.end())
	{
		unsigned int offset = item->second;
		unsigned int len;
		len = ReadInt32((unsigned char *)total_pointer+offset);
		//cout<<"读取块长度"<<len<<"偏移量"<<offset<<"序号"<<count<<endl;
		if(pbm->block_size>len)
		{
			memcpy(pbm->block_pointer,total_pointer+offset+size,len);
			pbm->block_size = len;
			pthread_spin_unlock(&m_spin_lock);
			return len;
		}
	}
	pthread_spin_unlock(&m_spin_lock);
	
	return 0;
}
int  block_queue::get_block_by_count(block *pb,unsigned int offset,uint64_t lapcount)
{

	unsigned int len;
	int size = sizeof(unsigned int);
	if(offset==0)
	{
		pb->count = m_nLaps;
		offset = m_new_offset;
	}
	if(m_nLaps==lapcount)
	{
		if(offset>=m_new_offset)
		{
			//读数据访问过快。。没有新数据
			pb->count = m_nLaps;
			pb->offset = m_new_offset;
			pb->len = 0;
			return 0;
		}
		else
		{
			//正常情况 顺序访问
			pb->count = m_nLaps;
		}
	}
	else if(m_nLaps==lapcount+1)
	{
		if(offset<m_last_offset)
		{
			//写指针已经掉头 读指针还没有
			pb->count = lapcount;
		}
		else
		{
			//写指针已经掉头 读指针也要开始掉头
			pb->count = m_nLaps;
			offset=0;
			if(offset>=m_new_offset)
			{
				//读数据访问过快。。没有新数据
				pb->count = m_nLaps;
				pb->offset = m_new_offset;
				pb->len = 0;
				return 0;
			}
			else
			{
				//正常情况 顺序访问
				pb->count = m_nLaps;
			}
		}
	}	
	//cout<<"编号"<<lapcount<<":"<<m_nLaps<<"偏移量"<<new_offset<<"参数偏移量"<<offset<<endl;
	len = ReadInt32((unsigned char *)total_pointer+offset);
	//cout<<"读取块长度"<<len<<"偏移量"<<offset<<"序号"<<count<<endl;
	if(len<BLOCK_CACHE_SIZE)
	{	
		pb->offset=offset+size;
		pb->len = len;
		m_temp_offset = offset;
		return len;
	}
	else
	{
		//数据读取异常 
		pb->count = m_nLaps;
		pb->offset = m_new_offset;
		pb->len = 0;

	}
	cout<<"长度异常返回"<<len<<"参数偏移量"<<offset<<"最新数据偏移"<<m_new_offset<<"上次数据访问"<<m_temp_offset<<endl;
	cout<<"最新数据编号"<<m_nLaps<<"参数访问编号"<<lapcount<<endl;
	//offsetmap::iterator item;
	//for(item = m_map.begin();item!=m_map.end();item++)
	//{
	//	//if(item->second<m_new_offset&&item->second-100>=offset)
	//	{
	//		cout<<item->second<<"  ";
	//	}
	//	
	//}
	//cout<<endl;
	return -1;
}
char *block_queue::get_buffer()
{
	return total_pointer;
}
bool block_queue::release_block_queue()
{
	total_pointer = 0;
	new_offset = 0;
	//m_list.clear();
	m_map.clear();
	if(total_pointer)
	{
		free(total_pointer);
	}
	return true;
}
bool block_queue::recycle_block(unsigned int startpos,unsigned int offset)
{

	offsetmap::iterator item;
	item = m_map.begin();
	for(;item!=m_map.end();)
	{
		if(item->second<offset&&item->second>=startpos)
		{
			//item = m_map.erase(item);
			m_map.erase(item++);
		}
		else
		{
			item++;
		}
	}
	return true;
}
zmq_block_pub::zmq_block_pub()
{
	zmq_context = NULL;
	zmq_pub_socket = NULL;
	m_binit = false;
	pthread_rwlock_init(&m_rw_lock,NULL);
}
zmq_block_pub::~zmq_block_pub()
{
	pthread_rwlock_destroy(&m_rw_lock);
}
bool zmq_block_pub::init_zmq_block_pub(int video_id)
{
	if(m_binit==false)
	{
		char	str[128]={0};
		sprintf(str,"inproc://gentek-video-%d",video_id);
		//sprintf(str,"ipc:///tmp/gentek-video-%d",video_id);
		zmq_name = string(str);
		zmq_context = zmq_ctx_new();
		zmq_pub_socket = zmq_socket(zmq_context,ZMQ_PUB);
		int ret = zmq_bind(zmq_pub_socket,str);
		//cout<<"发布数据"<<str<<endl;
		//int val = 300;
		//ret = zmq_setsockopt(zmq_pub_socket,ZMQ_SNDTIMEO,&val,sizeof(val));
		m_binit = true;
	}
	return true;
}
void *zmq_block_pub::get_context()
{
	return zmq_context;
}
bool zmq_block_pub::insert_block_buf(char *buffer,unsigned int len)
{
	if(zmq_pub_socket)
	{
		size_t zmq_fd_size = sizeof (int);
    int notify_fd;
		if (zmq_getsockopt (zmq_pub_socket, ZMQ_FD, &notify_fd, &zmq_fd_size) == -1)
		{
				return false;
		}
		int ret = fcntl(notify_fd,F_GETFL);
		if(ret==-1)
		{
				return false;
		}
		pthread_rwlock_wrlock(&m_rw_lock);
		ret = zmq_send(zmq_pub_socket,buffer,len,0);
		pthread_rwlock_unlock(&m_rw_lock);
		if(ret>=0)
		{
			return true;
		}
  }
	return false;
}
bool zmq_block_pub::uninit_zmq_block()
{
	cout<<"停止发布数据"<<zmq_name<<endl;
	sleep(1);
	pthread_rwlock_wrlock(&m_rw_lock);
	zmq_close(zmq_pub_socket);
	zmq_pub_socket = NULL;
	pthread_rwlock_unlock(&m_rw_lock);
	zmq_ctx_destroy(zmq_context);
	zmq_context = NULL;
	m_binit = false; 
	return true;
}
bool 	zmq_block_pub::in_lock()
{
		pthread_rwlock_wrlock(&m_rw_lock);
		return true;
}
bool 	zmq_block_pub::out_lock()
{
			pthread_rwlock_unlock(&m_rw_lock);
			return true;
}
zmq_block_sub::zmq_block_sub(zmq_block_pub *zbp)
{
	zbp->in_lock();
	zmq_context = zbp->get_context();
	zmq_sub_socket = zmq_socket(zmq_context,ZMQ_SUB);
	int ret =zmq_connect(zmq_sub_socket,zbp->zmq_name.c_str());
	zmq_name = zbp->zmq_name;
	//cout<<"订阅数据"<<zbp->zmq_name<<zmq_sub_socket<<endl;
	ret = zmq_setsockopt(zmq_sub_socket, ZMQ_SUBSCRIBE,NULL,0);
	zbp->out_lock();
	pthread_rwlock_init(&m_rw_lock,NULL);
	int val = 1;
	num = 0;
	//ret = zmq_setsockopt(zmq_sub_socket, ZMQ_LINGER, &val, sizeof(val));
	//val = 300;
	//ret = zmq_setsockopt(zmq_sub_socket,ZMQ_RCVTIMEO,&val,sizeof(val));

}
zmq_block_sub::~zmq_block_sub()
{
	//cout<<"取消订阅"<<zmq_name<<endl;
	pthread_rwlock_wrlock(&m_rw_lock);
	zmq_setsockopt(zmq_sub_socket, ZMQ_UNSUBSCRIBE,NULL,0);
	zmq_disconnect(zmq_sub_socket,zmq_name.c_str());
	zmq_close(zmq_sub_socket);
	zmq_sub_socket = NULL;
	pthread_rwlock_unlock(&m_rw_lock);
	zmq_context = NULL;
	pthread_rwlock_destroy(&m_rw_lock);
	//zmq_ctx_destroy(zmq_context); 
	
}
int zmq_block_sub::get_buf(char *buffer,int len)
{
	pthread_rwlock_wrlock(&m_rw_lock);
	if(zmq_sub_socket)
	{
		size_t zmq_fd_size = sizeof (int);
    int notify_fd;
		if (zmq_getsockopt (zmq_sub_socket, ZMQ_FD, &notify_fd, &zmq_fd_size) == -1)
		{
				pthread_rwlock_unlock(&m_rw_lock);
				return 0;
		}
		int ret = fcntl(notify_fd,F_GETFL);
		if(ret==-1)
		{
				//cout<<"重新订阅"<<zmq_name<<endl;
				zmq_setsockopt(zmq_sub_socket, ZMQ_UNSUBSCRIBE,NULL,0);
				zmq_close(zmq_sub_socket);
				zmq_sub_socket = NULL;
				zmq_sub_socket = zmq_socket(zmq_context,ZMQ_SUB);
				ret =zmq_connect(zmq_sub_socket,zmq_name.c_str());
				ret = zmq_setsockopt(zmq_sub_socket, ZMQ_SUBSCRIBE,NULL,0);
				pthread_rwlock_unlock(&m_rw_lock);
				return 0;
		}
		zmq_pollitem_t items []={{zmq_sub_socket, 0, ZMQ_POLLIN, 0 } };  
		int rc = zmq_poll(items, 1, 30);  
		if(rc==-1)
		{
			pthread_rwlock_unlock(&m_rw_lock);
			return 0;
		}
		if (items[0].revents & ZMQ_POLLIN) 
		{ 
			int recv_len = zmq_recv(zmq_sub_socket,buffer,len,0);
			pthread_rwlock_unlock(&m_rw_lock);
			return recv_len;
		}
		//cout<<"获取数据超时"<<endl;
	}
	pthread_rwlock_unlock(&m_rw_lock);
	return -1;
	
	
}
bool zmq_block_sub::reconnect()
{
	/*
	zmq_setsockopt(zmq_sub_socket, ZMQ_UNSUBSCRIBE,NULL,0);
	zmq_close(zmq_sub_socket);
	zmq_ctx_destroy(zmq_context);
	zmq_context = zmq_ctx_new();
	//zmq_sub_socket = zmq_socket(zbp->get_context(),ZMQ_SUB);
	zmq_sub_socket = zmq_socket(zmq_context,ZMQ_SUB);
	int ret =zmq_connect(zmq_sub_socket,zmq_name.c_str());
	if(ret<0)
	{
		zmq_close(zmq_sub_socket);
		zmq_sub_socket = NULL;
		//zmq_ctx_destroy(zmq_context);
		return false;
	}
	ret = zmq_setsockopt(zmq_sub_socket, ZMQ_SUBSCRIBE,NULL,0);
	//int val = 300;
	//ret = zmq_setsockopt(zmq_sub_socket,ZMQ_RCVTIMEO,&val,sizeof(val));
	*/
	return true;
}