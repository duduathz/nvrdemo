#include "video_data_manger.h"
video_data_manger::video_data_manger(void)
{

}
video_data_manger::~video_data_manger(void)
{

}
int video_data_manger::create(unsigned int video_id,string sip_id,time_t st,time_t et,string dest_ip,unsigned short port,int did,bool download)
{
	m_preader= new data_reader();
	m_preader->init_reader(video_id,st,et);
	m_ptranfer = new data_transfer();
	m_ptranfer->init_transfer_for_reader(m_preader,sip_id,dest_ip,port,did,download);
	return m_ptranfer->start_transfer();

}
bool video_data_manger::pause()
{
	if(m_ptranfer)
	{
		return m_ptranfer->pause_transfer();
	}
	return false;
}
bool video_data_manger::speed(float nspeed)
{
	if(m_ptranfer)
	{
		return m_ptranfer->speed_transfer(nspeed);
	}
	return false;
}
bool video_data_manger::stop()
{
	if(m_ptranfer)
	{
		m_ptranfer->stop_transfer();
		delete m_ptranfer;
		m_ptranfer = NULL;
		if(m_preader)
		{
			delete m_preader;
			m_preader = NULL;
		}
		return true;
	}
	return false;
}