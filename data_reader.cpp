#include "data_reader.h"
#include "Units.h"
#include "video_manger.h"
extern video_manger * g_vm;
extern sql_manger	* g_sm;
data_reader::data_reader(void)
{
	m_fp = NULL;
	m_blast = false;
	m_endpos = 0;
	m_video_id = 0;
	m_first_len = 0;
	m_reader_len = 0;
}

data_reader::~data_reader(void)
{
}
bool data_reader::init_reader(unsigned int video_id,time_t read_st)
{
	m_st = read_st;
	m_et = 0;
	m_file_list.clear();
	m_video_id = video_id;
	m_offmap.clear();
	int size = sizeof(unsigned int)+sizeof(unsigned int)+sizeof(unsigned int)+MAX_FILE_INDEX*(sizeof(time_t)+sizeof(unsigned int));
	g_sm->get_track_video_list_sql(video_id,m_file_list,read_st);
	
	if(m_file_list.size())
	{
		int offset = 0;
		file_list::iterator item = m_file_list.begin();
		
		m_fp = fopen((*item).file_path.c_str(),"rb");
		if(m_fp)
		{
			m_first_len = (*item).file_lenth;
			unsigned char *header = (unsigned char *)malloc(size);
			fread(header,size,1,m_fp);

			for(int i=0;i<MAX_FILE_INDEX;i++)
			{
				time_t t =ReadInt64(header+12+i*(sizeof(time_t)+sizeof(int)));
				//cout<<"read time "<<t<<endl;
				if(t==0)
				{
					break;
				}
				offset = ReadInt32(header+12+i*(sizeof(time_t)+sizeof(int))+sizeof(time_t));
				//cout<<"read offset"<<offset<<endl;
				if(t==read_st)
				{
					fseek(m_fp,offset,0);
					cout<<"��ʼ��reader ��ʼʱ��"<<read_st<<"��ƫ��λ��Ϊ"<<offset<<endl;
					break;
				}
			}
			free(header);
		}
		//cout<<"��ʼ��reader  "<<read_st<<"��ƫ��λ��Ϊ"<<offset<<endl;
		m_offsetpos = offset?offset:size;
		m_file_list.pop_front();
		return true;
	}
	return false;
}
bool data_reader::init_reader(unsigned int video_id,time_t read_st,time_t read_et)
{
	m_st = read_st;
	m_et = read_et;
	m_video_id = video_id;
	m_file_list.clear();
	int size = sizeof(unsigned int)+sizeof(unsigned int)+sizeof(unsigned int)+MAX_FILE_INDEX*(sizeof(time_t)+sizeof(unsigned int));
	g_sm->get_track_video_list_sql(video_id,m_file_list,read_st,read_et);

	cout<<"¼��ط��ļ�������"<<m_file_list.size()<<endl;

	if(m_file_list.size())
	{
		int offset = 0;
		file_list::iterator item = m_file_list.begin();
		cout<<"���ļ�"<<(*item).file_path.c_str()<<endl;
		m_fp = fopen((*item).file_path.c_str(),"rb");
		if(m_fp)
		{
			m_reader_len = 0;
			for(item=m_file_list.begin();item!=m_file_list.end();item++)
			{
				if(read_st>(*item).file_st&&read_st<=(*item).file_et)
				{
					int len = (*item).file_lenth*((*item).file_et- read_st)/((*item).file_et-(*item).file_st);
					m_reader_len+=len;
				}
				else if(read_et>=(*item).file_st&&read_et<(*item).file_et)
				{
					int len = (*item).file_lenth*(read_et - (*item).file_st)/((*item).file_et-(*item).file_st);
					m_reader_len+=len;
				}
				else
				{
					m_reader_len+=(*item).file_lenth;
				}
				
				//cout<<"�ļ���"<<(*item).file_path.c_str()<<endl;
				//cout<<"�ۼ�¼��γ���"<<m_reader_len<<endl;
			}
			unsigned char *header = (unsigned char *)malloc(size);
			fread(header,size,1,m_fp);
			for(int i=0;i<MAX_FILE_INDEX;i++)
			{
				time_t t =ReadInt64(header+12+i*(sizeof(time_t)+sizeof(int)));
				if(t==0)
				{
					cout<<"ʱ���ȡֵΪ0"<<endl;
					break;
				}
				if(t>read_st)
				{
					cout<<"ʱ���ȡֵΪ"<<t<<endl;
					break;
				}
				//struct tm		*st_tb;
				//st_tb = localtime(&t);
				//char 			timestr[128];
				//sprintf(timestr,"%4d-%02d-%02dT%02d:%02d:%02d",st_tb->tm_year+1900,st_tb->tm_mon+1,st_tb->tm_mday,st_tb->tm_hour,st_tb->tm_min,st_tb->tm_sec);
				//cout<<"ʱ������ֵ"<<timestr<<"--"<<t<<"---"<<read_st<<endl;
				
				offset = ReadInt32(header+12+i*(sizeof(time_t)+sizeof(int))+sizeof(time_t));
				
				if(t==read_st)
				{
					cout<<"�ҵ���ʼ����λ��"<<offset<<endl;
					fseek(m_fp,offset,0);
					if(m_file_list.size()!=1)
					{
						m_offsetpos = offset;
						//m_reader_len = m_reader_len -m_offsetpos;
						m_file_list.pop_front();
						cout<<"1��ʼ��reader  "<<read_st<<"��ƫ��λ��Ϊ"<<offset<<endl;
						return true;
					}
				}
				if(t==read_et)
				{
					cout<<"�ҵ���������λ��"<<offset<<endl;
					m_endpos = offset;
					m_blast = true;
				}
			}
			free(header);
		}
		else
		{
			cout<<"�ļ���ʧ��"<<errno<<endl;
		}
		m_offsetpos = offset?offset:size;
		//m_reader_len = m_reader_len-m_offsetpos;
		m_file_list.pop_front();
		cout<<"2��ʼ��reader  "<<read_st<<"��ƫ��λ��Ϊ"<<m_offsetpos<<"�ܳ���"<<m_reader_len<<endl;
		return true;
	}
	return false;
}
uint64_t data_reader::get_reader_length()
{
	//cout<<"��ǰ¼���ܳ���"<<m_reader_len<<endl;
	return m_reader_len;
}
int data_reader::getbuf(char *buf,unsigned int len)
{
	int leng = 0;
	if(m_fp)
	{
		leng = fread(buf,1,len,m_fp);
		//cout<<"��ȡ�ļ�����"<<len<<endl;
		if(leng<len&&leng>0)
		{
			fclose(m_fp);
			m_fp = NULL;
			m_offsetpos = 0;
			if (m_file_list.size())
			{
				file_list::iterator item = m_file_list.begin();
				m_fp = fopen((*item).file_path.c_str(),"rb");
				int size = sizeof(unsigned int)+sizeof(unsigned int)+sizeof(unsigned int)+MAX_FILE_INDEX*(sizeof(time_t)+sizeof(unsigned int));
				if(m_file_list.size()==1&&m_et!=0)
				{
					
					m_blast = true;
					unsigned char *header = (unsigned char *)malloc(size);
					fread(header,size,1,m_fp);
					time_t ct =ReadInt64(header+12+(sizeof(time_t)+sizeof(int)));
					if(ct==0)
					{
						//�ļ������ڱ�ʹ��
						memset(header,0,size);
						g_vm->get_idx_buffer(m_video_id,(char *)header,size);
					}
					for(int i=0;i<MAX_FILE_INDEX;i++)
					{
						time_t t =ReadInt64(header+12+i*(sizeof(time_t)+sizeof(int)));
						if(t==0)
						{
							break;
						}
						int offset = ReadInt32(header+12+i*(sizeof(time_t)+sizeof(int))+sizeof(time_t));
						if(t==m_et)
						{
							m_endpos = offset;
						}
					}
					free(header);
				}
				fseek(m_fp,size,0);
				m_offsetpos = size;
				m_file_list.pop_front();
			}			
		}
		else if(leng<=0)
		{
			
		}
		else
		{
			m_offsetpos+=leng;
			if(m_blast&&m_offsetpos>m_endpos)
			{
				fclose(m_fp);
				m_fp = NULL;
				m_offsetpos = 0;
			}
		}
		return leng;
	}
	else
	{
		return 0;
	}
	return 0;
}
bool data_reader::uninit_reader()
{
	if(m_fp)
	{	
		fclose(m_fp);
		m_fp = NULL;
	}
	return false;
}
int data_reader::getesbuf(char *buf,unsigned int len,/*[out]*/ int *ntype)
{
	return 0;
}