#pragma once
#include "data_reader.h"
#include "data_transfer.h"
#include "sql_manger.h"

class video_data_manger
{
public:
	video_data_manger(void);
	~video_data_manger(void);
private:
	data_reader		*m_preader;
	data_transfer	*m_ptranfer;
	bool			 m_brun;
public:
	int			create(unsigned int video_id,string sip_id,time_t st,time_t et,string dest_ip,unsigned short port,int did,bool download);
	bool			pause();
	bool			speed(float nspeed);
	bool			stop();
};