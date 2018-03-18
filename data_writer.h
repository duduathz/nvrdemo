#pragma once
#include "./include/stdprj.h"
#include "block_queue.h"
#include "bits.h"
#include "sql_manger.h"

class data_writer
{
public:
	data_writer(void);
	~data_writer(void);
	
private:
	FILE			*m_idxfp;
	block_queue		*m_p;
	zmq_block_sub	*m_sub_block;
	pthread_t		m_threadHandle;
	bool			m_brun;
	unsigned int	m_video_id;
	unsigned int	m_track_id;
	FILE			*m_datafp;
	unsigned int	m_serial;
	unsigned int	m_file_length;
	unsigned int	m_file_header_size;
	unsigned int	m_file_current_id;
	char			*m_pheader;
	unsigned int	m_indexnum;
	unsigned int	m_indexoffset;
	time_t			m_current_time;
	bits_buffer_s	m_bbs;
	block_mem		m_block_mem;
	bool 				m_binit;
	
public:
	string			m_prefile;
	unsigned int	m_prefile_id;
public:
	bool		pre_file(char *filename,unsigned int file_id);
	bool		using_prefile();
	string		get_prefile_name();
	bool		restore_idx(unsigned int video_id,char *filepath);
	bool		init_writer(unsigned int video_id);
	pthread_t	start_thread(block_queue *pbq);
	pthread_t	start_thread2(zmq_block_pub *pbq);
	bool		stop_thead();
	bool		close_writer();
	unsigned int get_offset(time_t t); //0 ´íÎó
	bool		get_idx_buffer(char *buffer,unsigned int len);
private:
	static void *raw_data(void *lp);
	static void *raw_data2(void *lp);
	bool		writer_file(char *pdata,unsigned int len);
	bool		open_file();
	bool		close_file();
	bool		re_open_idx();
	bool		write_index(time_t t,unsigned int offset);
	
};
