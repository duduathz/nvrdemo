#pragma once
#include "./include/stdprj.h"
class device_manger
{
public:
	device_manger(void);
	~device_manger(void);
public:
	bool device_login(string user,string pass,string ip,unsigned short port);
	bool start_video_stream(string dest_ip,unsigned short dest_port,string req_channel);
	bool device_keepalive();
	bool stop_video_stream();
	bool device_logout();
	bool device_ptz_cmd();
private:
	int /*socket*/			m_device_sock;
	struct 	sockaddr_in 	m_device_addr;
	string					m_user;
	string					m_pass;
	string					m_device_ipaddr;
	unsigned short			m_device_port;
	string					m_login_str;
};
