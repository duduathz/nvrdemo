#pragma once
#include "data_writer.h"
#include "data_transfer.h"
#include "sql_manger.h"
#include "block_queue.h"
#include "device_manger.h"
typedef map<string,device_manger *>			dc_device_map;
typedef map<string ,data_transfer *>				transfer_map;
class video_stream_manger
{
public:
	video_stream_manger(void);
	~video_stream_manger(void);
private:
	data_writer		*m_pwriter;
	block_queue		*m_pbq;
	dc_device_map	m_dc_device_map;
	device_manger	*m_pec_device;
	data_transfer	*m_preal_transfer;
	transfer_map	 m_transfer_map;
	transfer_map	 m_transfer_unuse_map;
	bool			m_benable;
	time_t			m_last_time_recv_data;
	int				m_udp_sock;
	pthread_t		m_recv_data_pthread;
	bool			m_brun;
	char			*m_buffer;
	string			m_localip;
	planlist		m_plan;
	bool			m_bstart_record;
	bool			m_bstart_shot;
	FILE			*m_fp_shot;
	sem_t			m_sem;
	zmq_block_pub	*m_pub_block;
	pthread_rwlock_t	m_tranfer_map_rw_lock;
	pthread_rwlock_t	m_tranfer_map_unuse_rw_lock;
	
public:
	unsigned short	m_recv_port;
	unsigned int	m_video_id; //视频编号 就是设备编号
	string			m_device_name;
	string			m_device_addr;
	unsigned int	m_device_status;
	unsigned int	m_device_type;
	string			m_device_user;
	string			m_device_pass;
	unsigned int	m_device_port;
	string			m_device_channel;
	string			m_device_sip_id;
	string			m_device_video_plan;
public:
	int create_by_cmd(unsigned int device_type,string device_name,string ip,unsigned int port,string username,string password,string channel,string sip_id);
	int create_by_sql(unsigned int device_id,unsigned int device_type,string device_name,string ip,unsigned int port,string username,string password,string channel,string video_plan,string sip_id);
	int create_by_temp(unsigned int device_id,unsigned int device_type,string device_name,string ip,unsigned int port,string username,string password,string channel,string sip_id);
	int update_by_cmd(unsigned int device_id,unsigned int device_type,string device_name,string ip,unsigned int port,string username,string password,string channel);
	int update_plan_by_cmd(unsigned int device_id,string plan,unsigned int maxday);
	int start_stream();
	int stop_stream();
	int	stop_recv_stream();
	int	start_recv_stream();
	int	start_writer();
	int	stop_writer();
	int update_time();
	int update_transfer_map();
	int add_dc_dest(string ip,unsigned int port,unsigned int recv_port,string username,string password,string channel);
	int add_dest(string ip,unsigned int port);
	int del_dc_dest(string ip,unsigned int recv_port,string channel);
	int del_dest(string ip,unsigned int port);

	int add_dest_ex(string ip,unsigned int port);
	int del_dest_ex(string ip,unsigned int port);

	int delete_stream_sql();
	int delete_stream_mem();

	bool		check_next_hour_video_plan(struct tm *p);
	bool		is_enable();
	bool		is_temp();
	bool		keepalive();
	time_t		get_last_time();
	bool		load_video_plan();
	bool		pre_file(char *filename,unsigned int file_id);
	bool		using_prefile();
	string		get_prefile_name();
	bool		get_idx_buffer(char *buffer,unsigned int len);
	bool		have_a_short();

	static void * recv_data_thread(void *lp);
};

class video_transit_manger
{
public:
	video_transit_manger();
	~video_transit_manger();
public:
		transfer_map	m_transfer_map;
		string				m_source_sipid;
		zmq_block_pub	*m_pub_block;
		bool					m_brun;
		time_t			m_last_time_recv_data;
		int					m_udp_sock;
		pthread_t		m_recv_data_pthread;
		bool				m_benable;
		unsigned short 	m_recv_port;
public:	
	 int create_transit(string sip_id,unsigned int video_id);
	 int add_dest(string dest_ip,unsigned short dest_port);
	 bool del_dest(string dest_ip,unsigned short dest_port);
	 bool delete_transit();
	 
	 static void * recv_data_thread(void *lp);
};
