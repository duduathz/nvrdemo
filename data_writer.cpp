#include "data_writer.h"
#include "video_manger.h"
extern sql_manger	* g_sm;
extern video_manger			*g_vm;
data_writer::data_writer(void)
{
	m_idxfp = NULL;
	m_datafp = NULL;
	m_threadHandle = 0;
	m_prefile_id = 0;
	m_file_header_size = sizeof(unsigned int)+sizeof(unsigned int)+sizeof(unsigned int)+MAX_FILE_INDEX*(sizeof(time_t)+sizeof(unsigned int));
	m_pheader = (char *)malloc(m_file_header_size);
	memset(m_pheader,0,m_file_header_size);
	m_current_time = 0;
	bits_initwrite(&m_bbs,m_file_header_size,m_pheader);
	m_block_mem.block_pointer = (char *)malloc(BLOCK_CACHE_SIZE);
	m_block_mem.block_size = BLOCK_CACHE_SIZE;
	m_sub_block = NULL;
	m_brun = false;
	m_track_id = 0;
	m_binit = false;
}

data_writer::~data_writer(void)
{
	free(m_pheader);
	m_pheader = NULL;
	free(m_block_mem.block_pointer);
	m_block_mem.block_pointer = NULL;
}

void *data_writer::raw_data(void *lp)
{
	data_writer *pdw = (data_writer *)lp;
	cout<<"启动视频"<<pdw->m_video_id<<"的写流线程 "<<endl;
	uint64_t count = 0;
	pdw->m_serial = 0;
	pdw->m_track_id = g_sm->start_video_track_sql(pdw->m_video_id,time(NULL)+TIME_OFFSET,NULL);
	cout<<"创建新的录像段 "<<pdw->m_track_id<<"-----"<<pdw->m_video_id<<endl;
	pdw->m_indexnum  = 0;
	count = pdw->m_p->get_max_count();
	while(pdw->m_brun)
	{
		memset(pdw->m_block_mem.block_pointer,0,BLOCK_CACHE_SIZE);
		pdw->m_block_mem.block_size = BLOCK_CACHE_SIZE;
		unsigned int len = pdw->m_p->get_block_by_count(&pdw->m_block_mem,count);
		if(len)
		{			
			count++;
			pthread_testcancel();
			if(pdw->writer_file(pdw->m_block_mem.block_pointer,pdw->m_block_mem.block_size)==false)
			{
				cout<<"文件写入失败"<<endl;
				pdw->m_brun = false;
				pdw->close_file();
				g_sm->stop_video_track_sql(pdw->m_track_id,time(NULL)+TIME_OFFSET);
				return 0;
			}
		}
		else
		{
			if(count<pdw->m_p->get_max_count())
			{
				count+=1;
			}
			else
			{

				usleep(100);
			}
		}
		pthread_testcancel();
	}
	cout<<"录像段 "<<pdw->m_track_id<<"结束-----"<<pdw->m_video_id<<endl;
	cout<<"视频"<<pdw->m_video_id<<"的写流线程结束 "<<endl;
	pthread_exit(NULL);
	return NULL;
}

void *data_writer::raw_data2(void *lp)
{
	data_writer *pdw = (data_writer *)lp;
	int		timeout = 0;
	cout<<"启动视频"<<pdw->m_video_id<<"的写流线程 "<<endl;
	pdw->m_serial = 0;
	pdw->m_track_id = g_sm->start_video_track_sql(pdw->m_video_id,time(NULL)+TIME_OFFSET,NULL);
	cout<<"创建新的录像段 "<<pdw->m_track_id<<"-----"<<pdw->m_video_id<<endl;
	pdw->m_indexnum  = 0;
	while(pdw->m_brun)
	{
		memset(pdw->m_block_mem.block_pointer,0,BLOCK_CACHE_SIZE);
		pdw->m_block_mem.block_size = BLOCK_CACHE_SIZE;
		//unsigned int len = pdw->m_p->get_block_by_count(&pdw->m_block_mem,count);
		int len = pdw->m_sub_block->get_buf(pdw->m_block_mem.block_pointer,BLOCK_CACHE_SIZE);
		if(len>0)
		{			
			pthread_testcancel();
			if(pdw->writer_file(pdw->m_block_mem.block_pointer,len)==false)
			{
				cout<<"文件写入失败"<<endl;
				pdw->m_brun = false;
				pdw->close_file();
				g_sm->stop_video_track_sql(pdw->m_track_id,time(NULL)+TIME_OFFSET);
				return 0;
			}
			timeout = 0;
		}
		else
		{
				pthread_testcancel();
				timeout++;
				if(timeout>10)
				{
					cout<<"录像获取数据超时,停止录像"<<pdw->m_video_id<<endl;
					pdw->m_brun = false;
					pdw->close_file();
					g_sm->stop_video_track_sql(pdw->m_track_id,time(NULL)+TIME_OFFSET);
					cout<<"录像段 "<<pdw->m_track_id<<"异常结束-----"<<pdw->m_video_id<<endl;
					cout<<"视频"<<pdw->m_video_id<<"的写流线程结束 "<<endl;
					pthread_exit(NULL);
					pdw->m_binit = false;
					return NULL;
				}
		}
		pthread_testcancel();
	}
	cout<<"录像段 "<<pdw->m_track_id<<"结束-----"<<pdw->m_video_id<<endl;
	cout<<"视频"<<pdw->m_video_id<<"的写流线程结束 "<<endl;
	pdw->m_binit = false;
	pthread_exit(NULL);
	return NULL;
}
bool data_writer::writer_file(char *pdata,unsigned int len)
{
	int dwrite= 0;
	if(m_datafp)
	{
		bool ret = write_index(time(NULL)+TIME_OFFSET,m_file_length);
		if(ret)
		{
			if(m_file_length+len<=FILE_BLOCK_SIZE)
			{
				dwrite = fwrite(pdata,len,1,m_datafp);
				if(dwrite)
				{
					m_file_length+=len;
					if(m_file_length==FILE_BLOCK_SIZE)
					{
						close_file();
						if(open_file())
						{

						}
						else
						{
							return false;
						}
					}
					return true;
				}
				else
				{
					cout<<"---5"<<"---"<<errno<<"---"<<len<<endl;
				}
			}
			else
			{
				unsigned int firstwrite = FILE_BLOCK_SIZE-m_file_length;
				dwrite =  fwrite(pdata,firstwrite,1,m_datafp);
				if(dwrite)
				{
					m_file_length = FILE_BLOCK_SIZE;
					close_file();
					if(open_file())
					{
						dwrite =  fwrite(pdata,len-firstwrite,1,m_datafp);
						if(dwrite)
						{
							m_file_length +=(len-firstwrite);
							return true;
						}
					}
					else
					{
						cout<<"---3"<<endl;
					}
				}
				else
				{
					cout<<"---4"<<"---"<<errno<<"---"<<firstwrite<<endl;
				}
			}
		}
		else
		{
			if(open_file())
			{
				dwrite =  fwrite(pdata,len,1,m_datafp);
				if(dwrite)
				{
					m_file_length+=len;
					return true;	
				}
				else
				{
					cout<<"---14"<<"---"<<errno<<"---"<<len<<endl;
				}
			}
			else
			{
				cout<<"---24"<<endl;
			}
		}
		
	}
	else
	{
		if(open_file())
		{
			dwrite =  fwrite(pdata,len,1,m_datafp);
			if(dwrite)
			{
				m_file_length+=len;
				return true;	
			}
			else
			{
				cout<<"---1"<<"---"<<errno<<"---"<<len<<endl;
			}
		}
		else
		{
			cout<<"---2"<<endl;
		}
	}
	return false;
}
bool data_writer::open_file()
{
	if(m_prefile!="")
	{
		m_datafp = fopen(m_prefile.c_str(),"wb+");
		if(m_datafp==NULL)
		{
			cout<<"文件打开失败"<<errno<<endl;
			return false;
		}
		m_file_current_id = m_prefile_id;
		g_sm->start_video_file_sql(m_file_current_id,time(NULL)+TIME_OFFSET,m_track_id,m_video_id,m_serial);
		m_prefile="";
		m_prefile_id = 0;
		int len = fwrite(m_pheader,m_file_header_size,1,m_datafp);
		m_file_length = m_file_header_size;
		if(re_open_idx())
		{
			len = fwrite(&m_serial,sizeof(m_serial),1,m_idxfp);
			bits_write(&m_bbs,sizeof(m_serial)*8,m_serial);
			write_index(time(NULL)+TIME_OFFSET,m_file_length);
			m_serial++;	
			
			return true;
		}
		cout<<"索引文件打开失败"<<endl;
	}
	else
	{
		g_vm->prepair_file_block(m_video_id);
		if(m_prefile!="")
		{
			m_datafp = fopen(m_prefile.c_str(),"wb+");
			if(m_datafp==NULL)
			{
				cout<<"文件打开失败"<<errno<<endl;
				return false;
			}
			m_file_current_id = m_prefile_id;
			g_sm->start_video_file_sql(m_file_current_id,time(NULL)+TIME_OFFSET,m_track_id,m_video_id,m_serial);
			m_prefile="";
			m_prefile_id = 0;
			int len = fwrite(m_pheader,m_file_header_size,1,m_datafp);
			m_file_length = m_file_header_size;
			if(re_open_idx())
			{
				len = fwrite(&m_serial,sizeof(m_serial),1,m_idxfp);
				bits_write(&m_bbs,sizeof(m_serial)*8,m_serial);
				write_index(time(NULL)+TIME_OFFSET,m_file_length);
				m_serial++;		
				return true;
			}
			cout<<"索引文件打开失败"<<endl;
		}

	}
	cout<<"预分配文件为空"<<endl;
	return false;
}
bool data_writer::re_open_idx()
{
	char str[128];
	sprintf(str,"/etc/GVR/%d.idx",m_video_id);
	m_idxfp = fopen(str,"wb+");
	if(m_idxfp)
	{
		memset(m_pheader,0,m_file_header_size);
		bits_initwrite(&m_bbs,m_file_header_size,m_pheader);
		int len = fwrite(&m_video_id,sizeof(m_video_id),1,m_idxfp);
		len = fwrite(&m_track_id,sizeof(m_track_id),1,m_idxfp);
		bits_write(&m_bbs,sizeof(m_video_id)*8,m_video_id);
		bits_write(&m_bbs,sizeof(m_track_id)*8,m_track_id);
		m_indexnum = 0;
		
	}
	else
	{
		return false;
	}
	return true;
}
bool data_writer::close_file()
{
	if(m_datafp)
	{
		fseek(m_datafp,0,SEEK_SET);
		fwrite(m_pheader,m_file_header_size,1,m_datafp);
		fclose(m_datafp);
		g_sm->stop_video_file_sql(m_file_current_id,time(NULL)+TIME_OFFSET,m_file_length);
		m_datafp = NULL;
	}
	if(m_idxfp)
	{
		fclose(m_idxfp);
		m_idxfp = NULL;
	}
}
bool data_writer::write_index(time_t t,unsigned int offset)
{
	if(t!=m_current_time)
	{
		m_current_time = t;
		if(m_indexnum<MAX_FILE_INDEX)
		{
			//这样写不确定在能喝直接写入文件的保持一致
			bits_write(&m_bbs,sizeof(time_t)*8,t);
			bits_write(&m_bbs,sizeof(offset)*8,offset);
			fwrite(&t,sizeof(time_t),1,m_idxfp);
			fwrite(&offset,sizeof(offset),1,m_idxfp);
			fflush(m_idxfp);
			m_indexnum++;
			return true;
		}
		else
		{
			//流量过小,主动切换文件
			cout<<"流量过小,主动切换文件"<<endl;
			close_file();
			return false;
			//stop_thead();

		}
		
	}
	return true;
}

bool data_writer::restore_idx(unsigned int video_id,char *filepath)
{

}
bool data_writer::init_writer(unsigned int video_id)
{
	if(m_binit==false)
	{
		m_serial = 0;
		m_video_id = video_id;
		m_binit = true;
	}
	return true;
}
pthread_t	data_writer::start_thread(block_queue *pbq)
{
	if(m_brun==false)
	{
		m_brun = true;
		m_p = pbq;
		pthread_create(&m_threadHandle,NULL,raw_data,this);
		
	}
	return m_threadHandle;

}
pthread_t	data_writer::start_thread2(zmq_block_pub *pbq)
{
	if(m_brun==false)
	{
		m_brun = true;
		if(m_sub_block)
		{
			delete m_sub_block;
			m_sub_block = NULL;
		}
		m_sub_block = new zmq_block_sub(pbq);
		pthread_create(&m_threadHandle,NULL,raw_data2,this);
	}
	return m_threadHandle;
	
}
bool data_writer::stop_thead()
{
	void	*ret;
	if(m_brun)
	{
		m_brun = false;
		pthread_cancel(m_threadHandle);
		pthread_join(m_threadHandle,&ret);

	}
	m_serial = 0;
	m_brun = false;
	close_file();
	delete m_sub_block;
	m_sub_block = NULL;
	g_sm->stop_video_track_sql(m_track_id,time(NULL)+TIME_OFFSET);
}
bool data_writer::pre_file(char *filename,unsigned int file_id)
{
	if(m_prefile=="")
	{
		m_prefile = string(filename);
		m_prefile_id = file_id;
		return true;
	}
	return false;
}
bool data_writer::using_prefile()
{
	return m_prefile==""?true:false;
}
string	data_writer::get_prefile_name()
{
	return m_prefile;
}
bool data_writer::close_writer()
{
	if(m_brun)
	{
		stop_thead();
	}
	if(m_idxfp)
	{
		fclose(m_idxfp);
		m_idxfp = NULL;
	}
	return true;
}
unsigned int data_writer::get_offset(time_t t)
{
	return 0;
}
bool data_writer::get_idx_buffer(char *buffer,unsigned int len)
{
	if(m_file_header_size<=len)
	{
		memcpy(buffer,m_pheader,m_file_header_size);
		return true;
	}
	return false;
}