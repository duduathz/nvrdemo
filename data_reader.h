#pragma once
#include "./include/stdprj.h"
#include "sql_manger.h"
class data_reader
{
public:
	data_reader(void);
	~data_reader(void);
private:
	file_list		m_file_list;
	time_t			m_st;
	time_t			m_et;
	FILE			*m_fp;
	int				m_first_len;
	int				m_offsetpos;
	int				m_endpos;
	offset_map		m_offmap;
	bool			m_blast;
	unsigned int	m_video_id;
	uint64_t		m_reader_len;
	
public:
	bool init_reader(unsigned int video_id,time_t read_st);	
	bool init_reader(unsigned int video_id,time_t read_st,time_t read_et);
	int getbuf(char *buf,unsigned int len);
	uint64_t get_reader_length();
	int getesbuf(char *buf,unsigned int len,/*[out]*/ int *ntype);
	bool uninit_reader();
};
