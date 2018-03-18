#pragma once
#include "data_reader.h"
#include "block_queue.h"

typedef map<string,sockaddr_in>		dest_map;
class data_transfer
{
public:
	data_transfer(void);
	~data_transfer(void);
private:
	data_reader		*m_preader;
	block_queue		*m_pblock_queue;
	zmq_block_sub	*m_sub_block;
	bool			m_breal;
	pthread_t		m_send_thread;
	bool			m_brun;
	uint64_t		m_ncount;
	block_mem		m_block_mem;
	pthread_rwlock_t	m_rw_lock;
	dest_map			m_dest_map;
	int				m_send_sock;
	bool			m_bpause;
	float			m_nspeed;
	string			m_sip_id;
	zmq_block_pub *m_pbq;
	
	string			m_dest_ip;
	unsigned short	m_dest_port;
public:
	int					m_did;
	bool				m_bprint;
	time_t			m_time_update;
	bool init_transfer_for_reader(data_reader *preader,string sip_id,string ip,unsigned short port,int did,bool download);

	bool init_transfer_for_realvideo(block_queue *pbq,string sip_id);
	bool init_transfer_for_realvideo2(zmq_block_pub *pbq,string sip_id);
	bool is_real_transfer();
	bool add_dest_addr_for_realvideo(string ip,unsigned short port);
	bool del_dest_addr_for_realvideo(string ip,unsigned short port);
	int  start_transfer();
	bool stop_transfer();
	bool pause_transfer();
	bool speed_transfer(float nspeed);
	size_t dest_size();
	bool update_time();
	bool add_dest_addr_for_transfer(string key,sockaddr_in addr);
	static void * real_send_thread(void *lp);
	static void * data_send_thread(void *lp);
	static void * real_send_thread2(void *lp);

};
