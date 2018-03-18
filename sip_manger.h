#pragma once
#include "stdprj.h"

struct role
{
	char role_type[128];
	char dev_id[128];
	char dev_ip[128];
	char dev_port[128];
	char dev_channel[128];
};
struct role_str
{
	string	role_type;
	string	dev_id;
	string	dev_ip;
	string	dev_port;
	string	dev_channel;
};

struct dialog
{
	unsigned int source_id;
	string	dest_ip;
	string  dest_port;
	string	channel;
	time_t	st;
};
typedef map<uint64_t,dialog>	dialog_map;

struct stream_channel
{
	string		source_sip_id;
	string		channel;
	uint64_t	cdid;
	unsigned int	cid;
	unsigned int	did;
	string		sip_server_id;
};
typedef map<string ,stream_channel>		sc_map;


struct transit_dest
{
	string	request_sip_id;
	string	dest_ip;
	unsigned	short dest_port;
};
typedef map<uint64_t,transit_dest>		gb_dest_map;

typedef map<uint64_t,string>					gb_source_map;

typedef map<string ,time_t>				sip_server_update_map;
class sip_manger
{
public:
	sip_manger(void);
	~sip_manger(void);
public:
	char				m_localip[128];
	unsigned short		m_lport;
	string				m_device_id;
	string				m_device_pass;
	string				m_sip_id;
	string				m_sip_server;
	string				m_media_center;
	unsigned short		m_sport;
	string				m_sport_str;
	char				m_strAuth[1024];
	pthread_t			m_pid2;
	pthread_t			m_thread_check_sip_server;
	bool				m_brun;
	unsigned short		m_tran_port;
	string				m_sip_domain_id;
	string				m_sip_route;
	dialog_map			m_dialog_map;
	gb_dest_map		m_gb_dest_map;
	gb_source_map	m_db_source_map;
	sc_map				m_stream_channel_map;
	sip_server_update_map	m_sip_server_update_map;
	pthread_rwlock_t		m_sip_server_lock;
	void				*m_zmq_req_socket;
	void				*m_zmq_context;
	
public:
	
	bool init_sip_manger();
	bool prase_dev_str(string dev_str,role_str &rs);
	bool answer_message_to_sip(eXosip_event_t *je);
	int process_real_invite(string sip_server,uint64_t id,string encode,string decode,int cid,int did);
	int process_data_inviate(string sip_server,uint64_t id,string source,string decode,time_t st,time_t et,int cid,int did,bool download=false);
	bool process_ack(uint64_t id);
	int process_invite(eXosip_event_t *je);
	bool process_call_message(uint64_t id,osip_message_t *request);
	bool process_message(osip_message_t *request);
	bool close_dialog(uint64_t id);
	bool build_message_file_end(int did,string sip_id);
	bool build_message_file_length(int did,string sip_id,uint64_t len);
	bool build_message_keep_alive(string sip_id);
	static void *wait_event(void *lp);
	static void *check_sip_server_alive(void *lp);
	bool apply_mediaserver_on_mediacenter(uint64_t id,string sip_id,string dest_ip,string dest_port);
	bool apply_port_on_mediaserver(uint64_t id,string sip_id,string dest_ip,string dest_port);

private:
	bool close_all_on_sip_server(string sip_server);
	bool answer_query_catalog(osip_message_t *answer);
	bool answer_query_recordInfo(osip_message_t *answer,string sip_id,string start_time,string end_time);
	static char	*get_line(char* start_of_line);

};
