#pragma once
#ifndef BLOCK_QUEUE
#define BLOCK_QUEUE
#include "stdprj.h"
typedef list<unsigned int>		offsetlist;
typedef map<uint64_t,unsigned int,greater<uint64_t> >		offsetmap;
struct block_mem
{
	unsigned int total_offset;
	unsigned int block_size;
	char	*	 block_pointer;
};
struct block
{
	unsigned int offset;
	unsigned int len;
	uint64_t	 count;
};
class block_queue
{
public:
	block_queue(void);
	~block_queue(void);
private:
	char *			total_pointer;
	unsigned int	total_size;
	unsigned int	new_offset;
	offsetlist		m_list;
	offsetmap		m_map;
	unsigned int	m_start_code;
	unsigned int	m_start_code_size;
	uint64_t		m_counter;
	pthread_rwlock_t	m_rw_lock;
	pthread_spinlock_t	m_spin_lock;
	unsigned int	m_last_offset;
	unsigned int	m_new_offset;
	unsigned int	m_temp_offset;
	uint64_t		m_nLaps;
public:
	string			block_name;
	bool init_block_queue(unsigned int nsize,string name,unsigned int start_code=0x01,unsigned int start_code_size=4);
	bool insert_block_buf(char *buffer,unsigned int len);
	unsigned int get_block_by_count(block_mem *pbm,uint64_t count = 0);
	int get_block_by_count(block *pb,unsigned int offset,uint64_t lapcount=0);
	uint64_t get_max_count();
	bool release_block_queue();
	char *get_buffer();
	
private:
	bool recycle_block(unsigned int startpos,unsigned int offset);

};

class zmq_block_pub
{
public:
	zmq_block_pub(void);
	~zmq_block_pub(void);
private:
	void	*	zmq_context;
	void	*	zmq_pub_socket;
	pthread_rwlock_t	m_rw_lock;
public:
	string		zmq_name;
	bool		m_binit;
public:
	bool	init_zmq_block_pub(int video_id);
	void	*get_context();
	bool	insert_block_buf(char *buffer,unsigned int len);
	bool	uninit_zmq_block();
	bool 	in_lock();
	bool 	out_lock();
};

class zmq_block_sub
{
public:
	string		zmq_name;
private:
	void	*	zmq_context;
	void	*	zmq_sub_socket;
	pthread_rwlock_t	m_rw_lock;
	unsigned int 	num;
public:
	zmq_block_sub(zmq_block_pub *zbp);
	~zmq_block_sub();
public:	
	int get_buf(char *buffer,int len);
	bool reconnect();
};


class shmque
{
private:
	char *	m_lpshm;
	char *	m_pbuffer;
	char	m_shmname[128];
	int		m_current_read_pos;
	int		m_current_write_pos;
	int		read_item(char *pdata,int len);
	int		write_item(char *pdata,int len);

public:
	int		create(char *name);
	int		open(char *name);
	int		close();
	int		push_in(char *pin,int len);
	int		stream_get(char *pdata,int len);
};
#endif
