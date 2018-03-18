#pragma once
#include "video_stream_manger.h"
#include "video_data_manger.h"
typedef map<unsigned int,video_stream_manger *>		stream_manger_map;
typedef map<uint64_t,video_data_manger *>			data_manger_map;
typedef map<string ,unsigned int>			sip_id_device_id_map;
typedef map<string ,video_transit_manger *>		transit_manger_map;
				
class video_manger
{
public:
	video_manger(void);
	~video_manger(void);
public:
	stream_manger_map	m_manger_map;
	data_manger_map		m_data_manger_map;
	stream_manger_map	m_manger_reconnect_map;
	transit_manger_map	m_transit_map;
private:
	unsigned int			device_id_temp;
	pthread_t				m_thread_keep_alive;
	pthread_t				m_thread_prepair;
	pthread_t				m_thread_check_video_plan;
	pthread_t				m_thread_check_device_online;
	pthread_t				m_thread_have_short;
	pthread_t				m_thread_reconnect;
	bool						m_brun;
	pthread_rwlock_t		m_stream_lock;
	pthread_rwlock_t		m_data_lock;
	file_list				m_pripair_file_list;
	sip_id_device_id_map	m_sip_map_id;
	list<unsigned int>		m_shot_task_list;
public:
	unsigned int	get_free_temp_device_id();
	bool load_video_stream();
	bool stop_video();
	int  add_video_data_stream(uint64_t did,unsigned int video_id,string sip_id,time_t st,time_t et,string ip,unsigned short port,int did_2,bool download);
	bool pause_video_data_stream(uint64_t did);
	bool speed_video_data_stream(uint64_t did,float nspeed);
	bool stop_video_data_stream(uint64_t did);
	bool add_video_stream(unsigned int device_type,string device_name,string ip,unsigned int port,string username,string password,string channel,string sip_id);
	unsigned int add_video_stream_temp(unsigned int device_type,string device_name,string ip,unsigned int port,string username,string password,string channel,string sip_id);

	bool update_video_stream(unsigned int device_id,unsigned int device_type,string device_name,string ip,unsigned int port,string username,string password,string channel);
	bool start_video_stream(unsigned int device_id);
	bool stop_video_stream(unsigned int device_id);
	bool delete_video_stream(unsigned int device_id);
	
	bool update_video_stream_plan(unsigned int device_id,string plan,unsigned int maxday);
	//这个函数已经不用了
	bool add_dc_dest(unsigned int device_id,string ip,unsigned int port,unsigned int recv_port,string username,string password,string channel);
	int add_dest(unsigned int device_id,string ip,unsigned int port);
	bool delete_video_stream_temp(unsigned int device_id);
	bool del_dc_dest(unsigned int device_id,string ip,unsigned int recv_port,string channel);
	bool del_dest(unsigned int device_id,string ip,unsigned int port);
	
	bool get_idx_buffer(unsigned int device_id,char *buffer,unsigned int len);
	int	get_id_from_sip_id(string sip_id);
	bool prepair_file_block(unsigned int device_id);
	bool have_shot(unsigned int device_id);

	
	unsigned short	add_transit_reciver(string request);
	bool						del_transit_reciver(string request);
	unsigned short	add_transit_dest(string request,string dest_ip,unsigned short dest_port);
	bool						del_transit_dest(string request,string dest_ip,unsigned short dest_port);
	

	static void * thread_check_device_online(void * lp);
	static void * thread_check_prepair_fileblock(void * lp);
	static void * thread_check_video_recycle(void * lp);
	static void * thread_keep_device_alive(void *lp);
	static void * thread_check_video_plan(void *lp);
	static void * thread_take_short(void *lp);
	static void * thread_reconnect(void *lp);

	
};
