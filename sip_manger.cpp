#include "sip_manger.h"
#include "Units.h"
#include "tinyxml.h"
#include "video_manger.h"
extern video_manger			*g_vm;
extern sql_manger			*g_sm;
sip_manger::sip_manger(void)
{
	pthread_rwlock_init(&m_sip_server_lock,NULL);
}

sip_manger::~sip_manger(void)
{
	pthread_rwlock_destroy(&m_sip_server_lock);
}
bool sip_manger::init_sip_manger()
{
	char		str[128];
	read_conf_value("nvr_sip_client",str,CONFIG_FILE);
	if(atoi(str))
	{		
		memset(str,0,128);
		read_conf_value("sip_server",str,CONFIG_FILE);
		m_sip_server = string(str);
		memset(str,0,128);
		read_conf_value("sip_server_id",str,CONFIG_FILE);
		m_sip_id = string(str);
		memset(str,0,128);
		read_conf_value("sip_server_port",str,CONFIG_FILE);
		m_sport_str = string(str);
		m_sport = atoi(str);
		memset(str,0,128);
		read_conf_value("sip_device_nvr_id",str,CONFIG_FILE);
		m_device_id = string(str);
		memset(str,0,128);
		read_conf_value("sip_device_nvr_pass",str,CONFIG_FILE);
		m_device_pass = string(str);

		memset(str,0,128);
		read_conf_value("sip_device_nvr_port",str,CONFIG_FILE);
		m_lport = atoi(str);
		memset(str,0,128);
		read_conf_value("media_center_addr",str,CONFIG_FILE);
		m_media_center = string(str);
		m_sip_domain_id = m_device_id.substr(0,10);
		eXosip_guess_localip(AF_INET,m_localip, 128); 
		cout<<"sip listen "<<m_lport<<endl;
		eXosip_listen_addr(IPPROTO_UDP,NULL,m_lport,AF_INET,0);
		eXosip_set_user_agent("Gentek NVR-H3C 2.0.2");
		eXosip_clear_authentication_info();
		eXosip_add_authentication_info(m_device_id.c_str(),m_device_id.c_str(),m_device_pass.c_str(),"MD5",NULL); //添加认证信息
		m_brun = true;
		int ret = pthread_create(&m_pid2,NULL,wait_event,this);
		if(ret)
			return false;
		ret = pthread_create(&m_thread_check_sip_server,NULL,check_sip_server_alive,this);
		if(ret)
			return false;
		m_zmq_context = NULL;
		m_zmq_req_socket = NULL;
		if(m_media_center.length()!=0)
		{
			m_zmq_context =  zmq_ctx_new();
			m_zmq_req_socket = zmq_socket(m_zmq_context,ZMQ_REQ);
			memset(str,0,128);
			sprintf(str,"tcp://%s:5555",m_media_center.c_str());
			zmq_connect(m_zmq_req_socket,str);
		}
		m_sip_route ="<sip:"+m_sip_id+"@"+m_sip_server+":"+m_sport_str+">";
		cout<<"start nvr sip service..."<<endl;
	}
	return m_brun;
}
bool sip_manger::answer_message_to_sip(eXosip_event_t *je)
{
	osip_body_t *body;
	osip_message_t	*send;
	char local_url[128] = {0};
	char dest_url[128] = {0};
	char sip_server_url[128] = {0};
	sprintf(local_url,"sip:%s@%s",m_device_id.c_str(),m_sip_domain_id.c_str());
	sprintf(dest_url,"sip:%s@%s",je->request->from->url->username,m_sip_domain_id.c_str());
	eXosip_message_build_request(&send,"MESSAGE",dest_url,local_url,m_sip_route.c_str());
	eXosip_message_send_request(send);
	return true;
}
bool sip_manger::prase_dev_str(string dev_str,role_str &rs)
{
	int offset = 0;
	int pos = dev_str.find(':',offset);
	if(pos==string::npos)
	{
		return false;
	}
	rs.role_type = dev_str.substr(offset,pos-offset);
	offset=pos;
	pos = dev_str.find(':',offset+1);
	if(pos==string::npos)
	{
		return false;
	}
	rs.dev_id = dev_str.substr(offset+1,pos-offset-1);
	offset=pos;
	pos = dev_str.find(':',offset+1);
	if(pos==string::npos)
	{
		return false;
	}
	rs.dev_ip = dev_str.substr(offset+1,pos-offset-1);
	offset=pos;
	pos = dev_str.find(':',offset+1);
	if(pos==string::npos)
	{
		rs.dev_port = dev_str.substr(offset+1,dev_str.length()-pos);
	}
	else
	{
		rs.dev_port = dev_str.substr(offset+1,pos-offset-1);
		offset=pos;
		rs.dev_channel = dev_str.substr(offset+1,dev_str.length()-pos);
	}
	return true;
}
int sip_manger::process_real_invite(string sip_server,uint64_t id,string encoder,string decoder,int cid,int did)
{
	role_str	rs_ec;
	int 			send_port = 0;
	dialog	dl;
	bool	benc = false;
	bool	bdec = false;
	int		dev_id = 900000;
	stream_channel		sc;
	if(prase_dev_str(encoder,rs_ec))
	{
		cout<<"编码器解析成功"<<rs_ec.dev_id<<endl;
		dev_id = g_vm->get_id_from_sip_id(rs_ec.dev_id);
		
		if(dev_id==-1)
		{
			cout<<"设备未找到，应该是转发流"<<endl;
			return 0;
			if(rs_ec.dev_channel!="")
			{
				dev_id = g_vm->add_video_stream_temp(1,rs_ec.dev_id,rs_ec.dev_ip,81,"admin","21232f297a57a5a743894a0e4a801fc3",rs_ec.dev_channel,rs_ec.dev_id);
			}
		}
		dl.source_id = dev_id;
		//cout<<"设备id号"<<dl.source_id<<endl;
		if(dl.source_id>=900000||dev_id==-1)
		{
			cout<<"设备信息配置失败"<<endl;
			return false;
		}
		benc = true;
		dl.st = 0;
		
		
		
	}
	role_str	rs_dc;
	if(prase_dev_str(decoder,rs_dc))
	{
		//cout<<"解码器解析成功"<<rs_dc.dev_id<<endl;
		//cout<<"解码器角色为:"<<rs_dc.role_type<<endl;
		string dest = rs_dc.dev_ip+"_"+rs_dc.dev_port;
		sc_map::iterator item;
		item = m_stream_channel_map.find(dest);
		if(item!=m_stream_channel_map.end())
		{
			cout<<"端口重复,主动结束会话"<<dest<<"大小"<<m_stream_channel_map.size()<<endl;
			eXosip_lock();
			eXosip_call_terminate(item->second.cid,item->second.did);
			eXosip_unlock();
			close_dialog(item->second.cdid);
			
		}
		
		if(m_stream_channel_map.size()<MAX_TRANSFER_REAL)
		{
			sc.cdid = id;
			sc.channel = rs_ec.dev_channel;
			sc.source_sip_id  = rs_ec.dev_id; 
			sc.sip_server_id = sip_server;
			sc.cid = cid;
			sc.did = did;
			m_stream_channel_map.insert(map<string,stream_channel>::value_type(dest,sc));
			//cout<<"添加目标地址11"<<rs_dc.role_type<<"11111111111111111111111"<<rs_dc.dev_port<<endl;
			if(rs_dc.role_type=="client")
			{
				
				send_port = g_vm->add_dest((unsigned int)dev_id,rs_dc.dev_ip,atoi(rs_dc.dev_port.c_str()));
				if(send_port)
				{
					dl.channel="";
					dl.dest_ip = rs_dc.dev_ip;
					dl.dest_port = rs_dc.dev_port;
					bdec = true;
				}
				else
				{
					cout<<"添加目标客户端失败"<<endl;
				}

			}
			else if(rs_dc.role_type=="monitor")
			{
				/*
				现在由SIP server来登录
				g_vm->add_dc_dest(id,rs_dc.dev_ip,81,atoi(rs_dc.dev_port.c_str()),"admin","21232f297a57a5a743894a0e4a801fc3",rs_dc.dev_channel);
				dl.channel=rs_dc.dev_channel;
				dl.dest_ip = rs_dc.dev_ip;
				dl.dest_port = rs_dc.dev_port;
				bdec = true;
				*/
				send_port = g_vm->add_dest((unsigned int)dev_id,rs_dc.dev_ip,atoi(rs_dc.dev_port.c_str()));
				if(send_port)
				{
					dl.channel="";
					dl.dest_ip = rs_dc.dev_ip;
					dl.dest_port = rs_dc.dev_port;
					bdec = true;
				}
				else
				{
					cout<<"添加目标解码器失败,设备id"<<dev_id<<endl;
				}
			}
		}
		else
		{
			//向MediaCenter发出转发请求
			dl.channel="";
			dl.dest_ip = rs_dc.dev_ip;
			dl.dest_port = rs_dc.dev_port;
			bdec = true;
		}	
	}
	if(benc==false)
	{
		cout<<"编码器解析失败"<<endl;
	}
	if(bdec==false)
	{
		cout<<"解码器解析失败"<<endl;
	}
	if(benc&&bdec)
	{
		m_dialog_map.insert(map<uint64_t,dialog>::value_type(id,dl));
		return send_port;
	}
	return send_port;
}
int sip_manger::process_data_inviate(string sip_server,uint64_t id,string source,string decoder,time_t st,time_t et,int cid,int did,bool download)
{
	role_str	rs_ec;
	int				send_port = 0;
	dialog	dl;
	bool	benc = false;
	bool	bdec = false;
	stream_channel		sc;
	if(prase_dev_str(source,rs_ec))
	{
		int dev_id = g_vm->get_id_from_sip_id(rs_ec.dev_id);
		dl.source_id = dev_id;
		if(id==-1)
		{
			return false;
		}
		dl.st = st;
		benc = true;
		
	}
	role_str	rs_dc;
	if(prase_dev_str(decoder,rs_dc))
	{
		string dest = rs_dc.dev_ip+"_"+rs_dc.dev_port;
		sc_map::iterator item;
		item = m_stream_channel_map.find(dest);
		if(item!=m_stream_channel_map.end())
		{
			cout<<"端口重复,主动结束会话"<<dest<<endl;
			//eXosip_lock();
			//eXosip_call_terminate(item->second.cid,item->second.did);
			//eXosip_unlock();
			close_dialog(item->second.cdid);
		}
		else
		{
			char	time_channel[32];
			sprintf(time_channel,"%d",dl.st);
			sc.cdid = id;
			sc.channel = string(time_channel);
			sc.source_sip_id  = rs_ec.dev_id; 
			sc.sip_server_id = sip_server;
			sc.cid = cid;
			sc.did = did;
			m_stream_channel_map.insert(map<string,stream_channel>::value_type(dest,sc));
		}

		dl.dest_ip = rs_dc.dev_ip;
		dl.dest_port = rs_dc.dev_port;
		send_port = g_vm->add_video_data_stream(id,dl.source_id,rs_ec.dev_id,st,et,dl.dest_ip,atoi(dl.dest_port.c_str()),did,download);
		bdec = true;
	}
	if(benc&&bdec)
	{
		m_dialog_map.insert(map<uint64_t,dialog>::value_type(id,dl));
		return send_port;
	}
	return send_port;
}
bool sip_manger::process_ack(uint64_t id)
{
	if(m_dialog_map.find(id)!=m_dialog_map.end())
	{
		g_vm->start_video_stream(m_dialog_map.find(id)->second.source_id);
		//cout<<"启动视频流"<<m_dialog_map.find(id)->second.source_id<<endl;
		return true;
	}
	return false;
}
int sip_manger::process_invite(eXosip_event_t *je)
{
	sdp_message_t	*remote_sdp = NULL;
	uint64_t id = je->did;
	id = id<<32;
	id = id|je->cid;
	int		send_port = 0;
	//协商后 收到协商的SDP结果 非国标设备 就只有这一次Invite

	//char *from_sip =(char *)malloc(1024);
	char *sip_name = osip_uri_get_username(osip_from_get_url(osip_message_get_from((osip_message_t *)je->request)));
	//osip_uri_to_str(osip_from_get_url(osip_message_get_from((osip_message_t *)je->request)),&from_sip);
	//osip_from_to_str(osip_message_get_from((osip_message_t *)je->request),&from_sip);
	string sip_server = string(sip_name);
	//free(from_sip);
	//int pos_1 = sip_server.find(':');
	//int pos_2 = sip_server.find('@');

	//string sip_server_id = sip_server.substr(pos_1,pos_2-pos_1);
	pthread_rwlock_wrlock(&m_sip_server_lock);
	m_sip_server_update_map.insert(map<string,time_t>::value_type(sip_server,time(NULL)));
	pthread_rwlock_unlock(&m_sip_server_lock);
	cout<<"收到Invite"<<id<<"来自"<<sip_server<<endl;
	cout<<je->did<<":"<<je->cid<<":"<<je->tid<<endl;
	remote_sdp = eXosip_get_sdp_info(je->request);
	bool	bStdSipDevice = false;
	string request="";
	osip_header_t	*ht;
	osip_message_get_subject(( osip_message_t *)je->request,0,&ht);
	char *p = osip_header_get_value(ht);
	request = string(p);
	int pos = request.find(':',1);
	char seq = request.at(pos+1);
	if(seq=='0')//实时
	{
		request = request.substr(0,pos);
	}
	int dev_id = g_vm->get_id_from_sip_id(request);
	if(dev_id==-1)
	{
		gb_source_map::iterator item;
		for(item = m_db_source_map.begin();item!=m_db_source_map.end();item++)
		{
				if(item->second==request)
				{
					bStdSipDevice  = true;
					
				}
		}	
	}
		
	if(remote_sdp)
	{
		//cout<<"远程获取SDP"<<endl;
		unsigned short video_destport = 0;
		unsigned short audio_destport = 0;
		string	request_type = sdp_message_s_name_get(remote_sdp);
		string  destip = string(sdp_message_o_addr_get(remote_sdp));
	
		for(int i=0;i<5;i++)
		{
			int ret = sdp_message_endof_media(remote_sdp,i);
			if(ret!=0)
				break;
			string media = string(sdp_message_m_media_get(remote_sdp,i));
			//cout<<"media type "<<media<<endl;
			if(media=="protocol"&&bStdSipDevice==false)
			{
				bStdSipDevice = false;
				string encoder = string(sdp_message_a_att_value_get(remote_sdp,i,0));
				string decoder = string(sdp_message_a_att_value_get(remote_sdp,i,1));
				string encoder_role = string(sdp_message_a_att_field_get(remote_sdp,i,0));
				string decoder_role = string(sdp_message_a_att_field_get(remote_sdp,i,1));
				encoder = encoder_role+":"+encoder;
				decoder = decoder_role+":"+decoder;
				//cout<<encoder<<endl;
				//cout<<decoder<<endl;
				//cout<<"request type "<<request_type<<endl;
				if(request_type=="Play")
				{
					send_port = process_real_invite(sip_server,id,encoder,decoder,je->cid,je->did);
				}
				else if(request_type=="Playback"||request_type=="Download")
				{
					string start_time = string(sdp_message_t_start_time_get(remote_sdp,0));
					string end_time = string(sdp_message_t_stop_time_get(remote_sdp,0));
					time_t	st = atoll(start_time.c_str())+TIME_OFFSET;
					time_t	et = atoll(end_time.c_str())+TIME_OFFSET;

					struct tm		*st_tb;
					st_tb = localtime(&st);
					char 			timestr[128];
					sprintf(timestr,"%4d-%02d-%02dT%02d:%02d:%02d",st_tb->tm_year+1900,st_tb->tm_mon+1,st_tb->tm_mday,st_tb->tm_hour,st_tb->tm_min,st_tb->tm_sec);
					cout<<"回放时间段"<<timestr<<"--"<<et<<endl;
					send_port = process_data_inviate(sip_server,id,encoder,decoder,st,et,je->cid,je->did,request_type=="Download");
				}
				break;
			}
		}
		

		if(bStdSipDevice==false)
		{
			cout<<"非国标请求完成"<<send_port<<endl;
		}
		if(bStdSipDevice)
		{
				//国标第二次请求
				cout<<"国标:第二次请求设备"<<request<<endl;
				transit_dest	td;
				td.request_sip_id = request;
				td.dest_ip = destip;
				td.dest_port = video_destport;
				send_port = g_vm->add_transit_dest(request,destip,video_destport);
				m_gb_dest_map.insert(map<uint64_t,transit_dest>::value_type(id,td));
		}
		sdp_message_free(remote_sdp);
		return send_port;
	}
	else
	{
		//国标第一次请求
		//osip_header_free(ht); 释放event的时候会释放的
		cout<<"国标:第一次请求的目标设备:"<<request<<endl;
		int recv_port = g_vm->add_transit_reciver(request);
		m_db_source_map.insert(map<uint64_t,string>::value_type(id,request));
		cout<<"国标第一次请求的通道端口号"<<recv_port<<endl;
		return recv_port;
		/*
		cout<<"没有获取到SDP,结束此次会话"<<endl;
		eXosip_lock();
		eXosip_call_terminate(je->cid,je->did);
		eXosip_unlock();
		*/
	}
	return 0;
		
}
/*
INFO sip:34010000001310000051@3401000000 SIP/2.0 Via: SIP/2.0/UDP 192.168.1.227:5060;rport;branch=z9hG4bK3874544667 
From: <sip:34020000001140000002@3402000000>;tag=1329712076 
To: <sip:34010000001310000051@3401000000>;tag=37007516 
Call-ID: 2297404381 
CSeq: 22 
INFO Contact: <sip:34020000001140000002@192.168.1.227:5060> 
Content-Type: Application/MANSRTSP 
Max-Forwards: 70 
Content-Length:    54 

PAUSE MANSRTSP/1.0 
CSeq: 1 
Scale: 1 
Range: npt=0- 
*/
//INFO etc
bool sip_manger::process_call_message(uint64_t id,osip_message_t *request)
{
	osip_body_t *body;
	osip_message_get_body (request, 0, &body);
	cout<<body->body<<endl;
	char	temp[128];
	if(sscanf(body->body,"%[^ ]",temp)==1)
	{
		if(strcmp(temp,"PLAY")==0)
		{
			float speed = 1.0;
			time_t st = 0;
			time_t et = 0;
			char * temp_line = body->body;
			while(1)
			{
				temp_line = get_line(temp_line);
				if(temp_line==NULL)
				{
					break;
				}
				if(sscanf(temp_line,"Scale: %[^\n]",temp)==1)
				{
					speed = atof(temp);
				}
				if(sscanf(temp_line,"Range: %[^\n]",temp)==1)
				{
					cout<<temp<<endl;
					char	temp2[128];
					if(sscanf(temp,"npt=%[^-]",temp2)==1)
						st = atoll(temp2);
					if(sscanf(temp,"npt=%*[^-]-%[^\n]",temp2)==1)
						et = atoll(temp2);
					//sscanf(temp,"smpte=")
				}
			}
			if(speed>0)
				g_vm->speed_video_data_stream(id,speed);
			else
				cout<<"不支持倒放"<<endl;
				

		}
		else if(strcmp(temp,"PAUSE")==0)
		{
			g_vm->pause_video_data_stream(id);
			return true;
		}
		else if(strcmp(temp,"TEARDOWN")==0)
		{
			g_vm->stop_video_data_stream(id);
			return true;
		}
		else
		{
			cout<<"命令匹配失败"<<endl;
		}
	}


	
	return false;
}
bool sip_manger::process_message(osip_message_t *request)
{
	osip_body_t *body;
	osip_message_t	*send;
	char local_url[128] = {0};
	char dest_url[128] = {0};
	char sip_server_url[128] = {0};
	int ret = osip_message_get_body (request, 0, &body);
	if(ret<0)
	{
		return false;
	}
	sprintf(local_url,"sip:%s@%s",m_device_id.c_str(),m_sip_domain_id.c_str());
	sprintf(dest_url,"sip:%s@%s",request->from->url->username,m_sip_domain_id.c_str());
	osip_via_t		*ova;
	osip_message_get_via(request,0,&ova);
	string sip_soute = "<sip:"+m_sip_id+"@"+ova->host+":"+ova->port+">";
	//cout<<sip_soute<<endl;
	eXosip_message_build_request(&send,"MESSAGE",dest_url,local_url,sip_soute.c_str());
	TiXmlDocument* Document = new TiXmlDocument();
	Document->Parse(body->body);
	
	if(strcmp(Document->RootElement()->Value(),"Query")==0)
	{

		if(strcmp(Document->RootElement()->FirstChildElement("CmdType")->GetText(),"Catalog")==0)
		{
			//设备目录查询
			Document->RootElement()->FirstChildElement("DeviceID")->GetText();
			//这里一般就是自己的设备id
			answer_query_catalog(send);
		}
		if(strcmp(Document->RootElement()->FirstChildElement("CmdType")->GetText(),"RecordInfo")==0)
		{
			//设备视音频检索
			const char *dev_id = Document->RootElement()->FirstChildElement("DeviceID")->GetText();
			const char *start_time = Document->RootElement()->FirstChildElement("StartTime")->GetText();
			const char *end_time = Document->RootElement()->FirstChildElement("EndTime")->GetText();
			answer_query_recordInfo(send,string(dev_id),string(start_time),string(end_time));
		}
	}
	if(strcmp(Document->RootElement()->Value(),"Control")==0)
	{
		if(strcmp(Document->RootElement()->FirstChildElement("CmdType")->GetText(),"DeviceControl")==0)
		{
			Document->RootElement()->FirstChildElement("DeviceID")->GetText();
			Document->RootElement()->FirstChildElement("PTZCmd")->GetText();
			delete Document;
			return false;
		}
	}
	if(strcmp(Document->RootElement()->Value(),"Notify")==0)
	{
		//Document->Print();
		if(strcmp(Document->RootElement()->FirstChildElement("CmdType")->GetText(),"Keepalive")==0)
		{
			//心跳
			const char *sip_server_id = Document->RootElement()->FirstChildElement("DeviceID")->GetText();
			if(m_sip_server_update_map.size()>0)
			{
				pthread_rwlock_wrlock(&m_sip_server_lock);
				//cout<<"sip_server_id"<<sip_server_id<<"--"<<m_sip_server_update_map.begin()->first<<endl;
				if(m_sip_server_update_map.find(string(sip_server_id))!=m_sip_server_update_map.end())
				{
					m_sip_server_update_map[string(sip_server_id)]= time(NULL);
					cout<<"收到来自"<<sip_server_id<<"的心跳,时间更新为"<<m_sip_server_update_map[string(sip_server_id)]<<"在"<<time(NULL)<<endl;
				}
				pthread_rwlock_unlock(&m_sip_server_lock);
			}

			delete Document;
			return false;
		}
	}
	ret = eXosip_message_send_request(send);
	cout<<"Message 回复"<<ret<<endl;
	//osip_message_free(send);
	delete Document;
	return false;
}
bool sip_manger::close_dialog(uint64_t id)
{
	//cout<<"结束会话"<<id<<endl;
	dialog_map::iterator item;
	item = m_dialog_map.find(id);
	if(m_dialog_map.end()!=item)
	{
		string dest = item->second.dest_ip+"_"+item->second.dest_port;
		m_stream_channel_map.erase(dest);
		cout<<"结束会话"<<item->second.source_id<<":"<<dest<<"时间"<<item->second.st<<"大小："<<m_stream_channel_map.size()<<endl;
		if(item->second.st==0)
		{
			//实时会话
			//g_vm->stop_video_stream(item->second.source_id);
			if(g_vm->del_dest(item->second.source_id,item->second.dest_ip,atoi(item->second.dest_port.c_str()))==false)
			{
				cout<<"删除目标解码器失败"<<endl;
			}
		}
		else
		{

			g_vm->stop_video_data_stream(id);
			//录像会话
		}
		m_dialog_map.erase(item);
		return true;
	}
	gb_dest_map::iterator	item1 = m_gb_dest_map.find(id);
	if(item1!=m_gb_dest_map.end())
	{
			cout<<"国标:删除目标会话"<<endl;
			g_vm->del_transit_dest(item1->second.request_sip_id,item1->second.dest_ip,item1->second.dest_port);
			m_gb_dest_map.erase(item1);
			return true;
	}	
	gb_source_map::iterator	item2 = m_db_source_map.find(id);
	if(item2!=m_db_source_map.end())
	{
			cout<<"国标:删除源设备会话"<<endl;
			g_vm->del_transit_reciver(item2->second);
			m_db_source_map.erase(item2);
			return true;
	}
	
	return false;
}
bool sip_manger::build_message_file_end(int did,string sip_id)
{
	osip_body_t *body;
	osip_message_t	*send;
	cout<<"创建回放结束消息"<<did<<endl;
	eXosip_call_build_request(did,"MESSAGE",&send);
	int i = 0;
	while(send==NULL&&i<10);
	{
		sleep(i);
		i++;
		eXosip_call_build_request(did,"MESSAGE",&send);
	}
	TiXmlDocument* Document = new TiXmlDocument();
	TiXmlDeclaration	*td=new TiXmlDeclaration("1.0","","");
	Document->LinkEndChild(td);
	TiXmlElement	*NotifyItem = new TiXmlElement("Notify");
	
	TiXmlElement	*CmdType = new TiXmlElement("CmdType");
	TiXmlText		*CmdTypeText = new TiXmlText("MediaStatus");
	CmdType->LinkEndChild(CmdTypeText);
	TiXmlElement	*SN = new TiXmlElement("SN");
	TiXmlText		*SNText = new TiXmlText("8");
	SN->LinkEndChild(SNText);
	TiXmlElement	*DeviceID = new TiXmlElement("DeviceID");
	TiXmlText		*DeviceIDText = new TiXmlText(sip_id.c_str());
	DeviceID->LinkEndChild(DeviceIDText);
	TiXmlElement	*NotifyType = new TiXmlElement("NotifyType");
	TiXmlText		*NotifyTypeText = new TiXmlText("121");
	NotifyType->LinkEndChild(NotifyTypeText);
	NotifyItem->LinkEndChild(CmdType);
	NotifyItem->LinkEndChild(SN);
	NotifyItem->LinkEndChild(DeviceID);
	NotifyItem->LinkEndChild(NotifyType);
	Document->LinkEndChild(NotifyItem);
	TiXmlPrinter printer; 
	Document->Accept(&printer);
	if(send)
	{
		//cout<<printer.CStr()<<endl;
		osip_message_set_body(send,printer.CStr(),printer.Size());
		osip_message_set_content_type(send,"Application/MANSCDP+xml");
		eXosip_call_send_request(did,send);
		delete Document;
		return true;
	}
	delete Document;
	return false;
	
}
bool sip_manger::build_message_file_length(int did,string sip_id,uint64_t len)
{
	if(len>0)
	{
		osip_body_t *body;
		osip_message_t	*send;
		cout<<"创建回放长度消息"<<did<<"---"<<len<<endl;
		eXosip_call_build_request(did,"MESSAGE",&send);
		TiXmlDocument* Document = new TiXmlDocument();
		TiXmlDeclaration	*td=new TiXmlDeclaration("1.0","","");
		Document->LinkEndChild(td);
		TiXmlElement	*NotifyItem = new TiXmlElement("Notify");

		TiXmlElement	*CmdType = new TiXmlElement("CmdType");
		TiXmlText		*CmdTypeText = new TiXmlText("MediaLength");
		CmdType->LinkEndChild(CmdTypeText);
		TiXmlElement	*SN = new TiXmlElement("SN");
		TiXmlText		*SNText = new TiXmlText("8");
		SN->LinkEndChild(SNText);
		TiXmlElement	*Length = new TiXmlElement("Length");
		char	lenstr[128];
		sprintf(lenstr,"%lld",len);
		TiXmlText		*LengthText = new TiXmlText(lenstr);
		Length->LinkEndChild(LengthText);
		TiXmlElement	*DeviceID = new TiXmlElement("DeviceID");
		TiXmlText		*DeviceIDText = new TiXmlText(sip_id.c_str());
		DeviceID->LinkEndChild(DeviceIDText);
		TiXmlElement	*NotifyType = new TiXmlElement("NotifyType");
		TiXmlText		*NotifyTypeText = new TiXmlText("121");
		NotifyType->LinkEndChild(NotifyTypeText);
		NotifyItem->LinkEndChild(CmdType);
		NotifyItem->LinkEndChild(SN);
		NotifyItem->LinkEndChild(Length);
		NotifyItem->LinkEndChild(DeviceID);
		NotifyItem->LinkEndChild(NotifyType);
		Document->LinkEndChild(NotifyItem);
		TiXmlPrinter printer; 
		Document->Accept(&printer);

		if(send)
		{
			//cout<<printer.CStr()<<endl;
			osip_message_set_body(send,printer.CStr(),printer.Size());
			osip_message_set_content_type(send,"Application/MANSCDP+xml");
			eXosip_call_send_request(did,send);
			delete Document;
			return true;
		}
		delete Document;
		return false;
		
	}
	return false;

}
bool sip_manger::build_message_keep_alive(string sip_id)
{
	osip_body_t		*body;
	osip_message_t	*send=NULL;
	char local_url[128] = {0};
	char dest_url[128] = {0};
	char sip_server_url[128] = {0};
	sprintf(local_url,"sip:%s@%s",m_device_id.c_str(),m_sip_domain_id.c_str());
	sprintf(dest_url,"sip:%s@%s",sip_id.c_str(),m_sip_domain_id.c_str());
	eXosip_message_build_request(&send,"MESSAGE",dest_url,local_url,m_sip_route.c_str());
	cout<<dest_url<<"--"<<local_url<<"--"<<m_sip_route<<endl;
	if(send)
	{
		
		TiXmlDocument* Document = new TiXmlDocument();
		TiXmlDeclaration	*td=new TiXmlDeclaration("1.0","","");
		Document->LinkEndChild(td);
		TiXmlElement	*NotifyItem = new TiXmlElement("Notify");

		TiXmlElement	*CmdType = new TiXmlElement("CmdType");
		TiXmlText		*CmdTypeText = new TiXmlText("Keepalive");
		CmdType->LinkEndChild(CmdTypeText);
		TiXmlElement	*SN = new TiXmlElement("SN");
		TiXmlText		*SNText = new TiXmlText("1");
		SN->LinkEndChild(SNText);
		TiXmlElement	*DeviceID = new TiXmlElement("DeviceID");
		TiXmlText		*DeviceIDText = new TiXmlText(m_device_id.c_str());
		DeviceID->LinkEndChild(DeviceIDText);
		TiXmlElement	*Status = new TiXmlElement("Status");
		TiXmlText		*StatusTypeText = new TiXmlText("OK");
		Status->LinkEndChild(StatusTypeText);
		NotifyItem->LinkEndChild(CmdType);
		NotifyItem->LinkEndChild(SN);
		NotifyItem->LinkEndChild(DeviceID);
		NotifyItem->LinkEndChild(Status);
		Document->LinkEndChild(NotifyItem);
		TiXmlPrinter printer; 
		Document->Accept(&printer);
		osip_message_set_body(send,printer.CStr(),printer.Size());
		osip_message_set_content_type(send,"Application/MANSCDP+xml");
		eXosip_message_send_request(send);
		delete Document;
		return true;
	}
	return false;
}
void *sip_manger::check_sip_server_alive(void *lp)
{
	sip_manger *psms = (sip_manger *)lp;
	for(;psms->m_brun;)
	{

		if(psms->m_sip_server_update_map.size())
		{
			pthread_rwlock_wrlock(&psms->m_sip_server_lock);
			sip_server_update_map::iterator item;
			for(item= psms->m_sip_server_update_map.begin();item!=psms->m_sip_server_update_map.end();)
			{
				if(time(NULL)-item->second>180)
				{
					//这里应该主动发送一个心跳包
					psms->build_message_keep_alive(item->first);
				}
				if(time(NULL)-item->second>200)
				{
					psms->close_all_on_sip_server(item->first);
					psms->m_sip_server_update_map.erase(item++);
					continue;
				}
				item++;
			}
			pthread_rwlock_unlock(&psms->m_sip_server_lock);
			sleep(100);
		}
		else
		{
			sleep(1);
		}
	}
}
bool sip_manger::apply_mediaserver_on_mediacenter(uint64_t id,string sip_id,string dest_ip,string dest_port)
{
	vector<string> v;
	sbuffer sbuf;
	string	cmd="applymediaserver";
	v.push_back(cmd);
	v.push_back(sip_id);
	v.push_back(dest_ip);
	v.push_back(dest_port);
	pack(&sbuf,v);
	char	buf[128];
	if(m_zmq_context&&m_zmq_req_socket)
	{
		zmq_send(m_zmq_req_socket,sbuf.data(),sbuf.size(),0);
		int len = zmq_recv(m_zmq_req_socket,buf,128,0);
		if(len>0)
		{
			unpacked msg;    // includes memory pool and deserialized object
			unpack(&msg, buf,len);

		}
	}
	return false;
	

}
bool sip_manger::apply_port_on_mediaserver(uint64_t id,string sip_id,string dest_ip,string dest_port)
{

}
void *sip_manger::wait_event(void *lp)
{
	sip_manger *psms = (sip_manger *)lp;
	eXosip_event_t *je = NULL;
	osip_message_t *answer = NULL;
	osip_message_t *send = NULL;
	sdp_message_t	*remote_sdp = NULL;
	int		last_reg_suc_time = 0;
	int		last_reg_time = time(NULL)+TIME_OFFSET;
	char	local_url[256];
	char	sip_server_url[256];
	char	port_str[32];
	sprintf(local_url,"sip:%s@%s",psms->m_device_id.c_str(),psms->m_sip_domain_id.c_str());
	sprintf(sip_server_url,"sip:%s@%s:%d",psms->m_sip_id.c_str(),psms->m_sip_server.c_str(),psms->m_sport);
	cout<<sip_server_url<<endl;
	eXosip_add_authentication_info(psms->m_device_id.c_str(),psms->m_device_id.c_str(),psms->m_device_pass.c_str(),"MD5",NULL); 
	int reg_id = eXosip_register_build_initial_register(local_url,sip_server_url,NULL,LIVE_TIME,&send);
	eXosip_register_send_register(reg_id,send);
#ifndef OSIP_MT 
	cout<<"sip进入单线程模式"<<endl;
#endif
	for(;psms->m_brun;)
	{
		je = eXosip_event_wait(0,200);
		if(time(NULL)+TIME_OFFSET- last_reg_suc_time >LIVE_TIME&&time(NULL)+TIME_OFFSET-last_reg_time>10)   
		{
			//超时，重新注册   
			eXosip_lock();
			eXosip_automatic_refresh(); 
			eXosip_unlock();  
			last_reg_time = time(NULL)+TIME_OFFSET;
		}
		if(je==NULL)
		{
#ifndef OSIP_MT  
      //eXosip_execute();  
      //eXosip_automatic_action ();  
#endif  
      usleep(10);  
      continue;  

		}
#ifndef OSIP_MT  
     //eXosip_execute();  
#endif  
    //eXosip_automatic_action (); 
		switch(je->type)
		{
		case EXOSIP_REGISTRATION_FAILURE:
			{
				if(je->response)
				{
					cout<<je->response->to->url->username<<" reg sip to server failed "<<endl;
					eXosip_clear_authentication_info();
					eXosip_add_authentication_info(je->response->to->url->username,je->response->to->url->username,psms->m_device_pass.c_str(),"MD5",NULL); //添加认证信息
				}
				eXosip_automatic_action();
				
				break;
			}
		case EXOSIP_REGISTRATION_SUCCESS:
			{
				if(0 < je->response->contacts.nb_elt) 
				{
					cout<<"国标:注册成功"<<endl;
					cout<<je->response->to->url->username<<" reg sip to server success "<<endl;
					if(strcmp(je->response->to->url->username,psms->m_device_id.c_str())==0)
					{
						last_reg_suc_time = time(NULL)+TIME_OFFSET;
					}
				}
				else
				{
					cout<<"canceled sip to server success"<<endl;
				}
				break;
			}
		case  EXOSIP_MESSAGE_NEW://
			{
				int ret = 0;
				if(MSG_IS_MESSAGE (je->request))//如果接受到的消息类型是MESSAGE
				{
					eXosip_lock();

					int i = eXosip_message_build_answer(je->tid,200,&answer);
					if (i != 0)
					{
						eXosip_message_send_answer(je->tid,400,NULL);
					}
					else
					{
						ret=eXosip_message_send_answer(je->tid,200,answer);
					}
					cout<<"收到MESSAGE"<<endl;
					psms->process_message(je->request);
					eXosip_unlock();
				}
				break;
			}
		case  EXOSIP_CALL_INVITE://被叫收到Invite，100，101自动回复，现在回复180，同时播放铃声
			{
				int send_port=psms->process_invite(je);

				eXosip_lock ();
				int i = eXosip_call_build_answer (je->tid, 200, &answer);
				if(i!=0)
				{
					 i = eXosip_call_build_answer (je->tid, 400, &answer);
				}
				if(i!=0)
				{
					cout<<"build answer packet error"<<endl;
				}
				if(send_port>0)
				{
					memset(port_str,0,32);
					sprintf(port_str,"%d",send_port);
				
					string sdp = "\
v=0 \r\n\
o="+psms->m_device_id+" 0 0 IN IP4 "+psms->m_localip+" \r\n\
s=Gentek MediaServer \r\n\
c=IN IP4 "+psms->m_localip+"\r\n\
t=0 0\r\n\
m=video "+port_str+" RTP/AVP 96 97 98\r\n\
a=recvonly\r\n\
a=rtpmap:96 PS/90000\r\n\
a=rtpmap:98 H264/90000\r\n\
a=rtpmap:97 MPEG4/90000\r\n";
					osip_message_set_body(answer,sdp.c_str(),sdp.size());
					osip_message_set_content_type (answer,"application/sdp");
		
					eXosip_call_send_answer (je->tid, 200, answer);
				}
				else
				{
					if(send_port<=0)
					{
						eXosip_call_send_answer (je->tid, 500, answer);
					}
				}
				eXosip_unlock();	
				break;
			}
		case EXOSIP_CALL_CANCELLED:
			{
				cout<<"主叫取消"<<endl;
				//主叫取消;
			}
			break;
		case EXOSIP_CALL_ACK:    //被叫收到ack
			{
				//开始发送视频流
				//uint64_t id = je->cid;
				uint64_t id = je->did;
				id = id<<32;
				id = id|je->cid;
				//cout<<"开始发送流"<<id<<endl;
				//cout<<je->did<<":"<<je->cid<<":"<<je->tid<<endl;
				psms->process_ack(id);
				
				break;
			}
		case EXOSIP_CALL_MESSAGE_NEW:
			{
				//uint64_t id = je->cid;
				uint64_t id = je->did;
				id = id<<32;
				id = id|je->cid;
				//cout<<"收到Call Message"<<id<<endl;
				//cout<<je->did<<":"<<je->cid<<":"<<je->tid<<endl;
				if(MSG_IS_INFO(je->request)) 
				{
					eXosip_lock();
					int i = eXosip_call_build_answer(je->tid,200,&answer);
					if (i != 0)
					{
						eXosip_call_send_answer(je->tid,400,NULL);
					}
					eXosip_call_send_answer(je->tid, 200, answer);
					eXosip_unlock();
					psms->process_call_message(id,je->request);
				}
				break;
			}
		case EXOSIP_CALL_CLOSED:    //收到bye
			{
				//uint64_t id = je->cid;
				
				uint64_t id = je->did;
				id = id<<32;
				id = id|je->cid;
				cout<<"收到Bye"<<id<<"--"<<je->did<<":"<<je->cid<<":"<<je->tid<<endl;
				// 删除临时设备 和发送流的目标地址
				psms->close_dialog(id);
				eXosip_lock();
				int i = eXosip_call_build_answer(je->tid, 200, &answer);
				if (i != 0)
				{
					eXosip_call_send_answer(je->tid, 400, NULL);
				}
				else
				{
					eXosip_call_send_answer(je->tid, 200, answer);
				}
				//eXosip_call_terminate(je->cid,je->did);
				eXosip_unlock();

				break;
			}
		case EXOSIP_CALL_GLOBALFAILURE://拒绝接听
			break;
		case EXOSIP_CALL_RELEASED:
			{
				//清除上下文
			}
			break;
		case EXOSIP_MESSAGE_ANSWERED:
			{
				//更新
				//cout<<je->response->to->url->username<<"回复消息"<<endl;
				const char *sip_server_id = je->response->to->url->username;
				if(psms->m_sip_server_update_map.size()>0)
				{
					pthread_rwlock_wrlock(&psms->m_sip_server_lock);
					//cout<<"sip_server_id"<<sip_server_id<<"--"<<m_sip_server_update_map.begin()->first<<endl;
					if(psms->m_sip_server_update_map.find(string(sip_server_id))!=psms->m_sip_server_update_map.end())
					{
						psms->m_sip_server_update_map[string(sip_server_id)]= time(NULL);
						cout<<"收到来自"<<sip_server_id<<"的心跳,时间更新为"<<psms->m_sip_server_update_map[string(sip_server_id)]<<"在"<<time(NULL)<<endl;
					}
					pthread_rwlock_unlock(&psms->m_sip_server_lock);
				}
			}
			break;
		default:
			break;

		}
		eXosip_event_free(je);
	}

}
char* sip_manger::get_line(char* start_of_line)
{
	// returns the start of the next line, or NULL if none
	for (char* ptr = start_of_line; *ptr != '\0'; ++ptr) {
		// Check for the end of line: /r/n (but also accept /r or /n by itself):
		if (*ptr == '\r' || *ptr == '\n') {
			// We found the end of the line
			if (*ptr == '\r') {
				*ptr++ = '\0';
				if (*ptr == '\n') ++ptr;
			} else {
				*ptr++ = '\0';
			}
			return ptr;
		}
	}
	return NULL;
}
bool sip_manger::close_all_on_sip_server(string sip_server)
{
	sc_map::iterator item;
	for(item=m_stream_channel_map.begin();item!=m_stream_channel_map.end();)
	{
		if(item->second.sip_server_id==sip_server)
		{
			cout<<"sip server 异常停止视频传输"<<endl;
			int size = m_stream_channel_map.size();
			close_dialog(item->second.cdid);
			int size2 = m_stream_channel_map.size();
			if(size<=size2)
			{
				return false;
			} 
			item=m_stream_channel_map.begin();
			continue;
		}
		item++;
		
	}
	
	return true;
}
bool sip_manger::answer_query_catalog(osip_message_t *answer)
{
	TiXmlDocument* DevDocument = new TiXmlDocument();
	TiXmlDeclaration	*td=new TiXmlDeclaration("1.0","","");
	DevDocument->LinkEndChild(td);
	TiXmlElement	*Response = new TiXmlElement("Response");
	DevDocument->LinkEndChild(Response);
	TiXmlElement	*CmdType = new TiXmlElement("CmdType");
	Response->LinkEndChild(CmdType);
	TiXmlText		*CmdText = new TiXmlText("Catalog");
	CmdType->LinkEndChild(CmdText);
	TiXmlElement	*SN = new TiXmlElement("SN");
	Response->LinkEndChild(SN);
	TiXmlText		*SnText = new TiXmlText("17430");
	SN->LinkEndChild(SnText);
	TiXmlElement	*DeviceID = new TiXmlElement("DeviceID");
	Response->LinkEndChild(DeviceID);
	TiXmlText		*DeviceIDText = new TiXmlText(m_device_id.c_str());
	DeviceID->LinkEndChild(DeviceIDText);
	TiXmlElement	*SumNum = new TiXmlElement("SumNum");
	Response->LinkEndChild(SumNum);
	TiXmlText		*SumNumText = new TiXmlText("100");
	SumNum->LinkEndChild(SumNumText);
	TiXmlElement	*DeviceList = new TiXmlElement("DeviceList");

	device_list	dl;
	g_sm->get_device_list_sql(dl);
	device_list::iterator item;
	DeviceList->SetAttribute("Num",dl.size());
	if(dl.size()>0)
	{
		for(item=dl.begin();item!=dl.end();item++)
		{
			TiXmlElement	*DeviceItem = new TiXmlElement("Item");
			TiXmlElement	*DeviceID = new TiXmlElement("DeviceID");
			TiXmlText		*DeviceIDText = new TiXmlText((*item).sip_id.c_str());
			DeviceID->LinkEndChild(DeviceIDText);
			TiXmlElement	*Name = new TiXmlElement("Name");
			TiXmlText		*NameText = new TiXmlText((*item).name.c_str());
			Name->LinkEndChild(NameText);
			TiXmlElement	*Manufacturer = new TiXmlElement("Manufacturer");
			TiXmlElement	*Model = new TiXmlElement("Model");
			TiXmlElement	*Owner = new TiXmlElement("Owner");
			TiXmlElement	*CivilCode = new TiXmlElement("CivilCode");
			TiXmlElement	*Block = new TiXmlElement("Block");
			TiXmlElement	*Address = new TiXmlElement("Address");
			TiXmlText		*AddressText = new TiXmlText("秋月路申江路");
			Address->LinkEndChild(AddressText);
			TiXmlElement	*Parental = new TiXmlElement("Parental");
			TiXmlElement	*ParentID = new TiXmlElement("ParentID");
			TiXmlElement	*SafetyWay = new TiXmlElement("SafetyWay");
			TiXmlText		*SafetyWayText = new TiXmlText("1");
			SafetyWay->LinkEndChild(SafetyWayText);
			TiXmlElement	*RegisterWay = new TiXmlElement("RegisterWay");
			TiXmlText		*RegisterWayText = new TiXmlText("1");
			RegisterWay->LinkEndChild(RegisterWayText);
			TiXmlElement	*CertNum = new TiXmlElement("CertNum");
			TiXmlElement	*Certifiable = new TiXmlElement("Certifiable");
			TiXmlElement	*ErrCode = new TiXmlElement("ErrCode");
			TiXmlElement	*EndTime = new TiXmlElement("EndTime");
			TiXmlElement	*Secrecy = new TiXmlElement("Secrecy");
			TiXmlElement	*IPAddress = new TiXmlElement("IPAddress");
			TiXmlText		*IPAddressText = new TiXmlText((*item).ip.c_str());
			IPAddress->LinkEndChild(IPAddressText);
			TiXmlElement	*Port = new TiXmlElement("Port");
			char	strport[10];
			sprintf(strport,"%d",(*item).port);
			TiXmlText		*PortText = new TiXmlText(strport);
			Port->LinkEndChild(PortText);
			TiXmlElement	*Password = new TiXmlElement("Password");
			TiXmlText		*PasswordText = new TiXmlText("admin");
			Password->LinkEndChild(PasswordText);
			TiXmlElement	*Status = new TiXmlElement("Status");
			TiXmlText		*StatusText = new TiXmlText("working");
			Status->LinkEndChild(StatusText);
			TiXmlElement	*Longitude = new TiXmlElement("Longitude");
			TiXmlText		*LongitudeText = new TiXmlText("0.0");
			Longitude->LinkEndChild(LongitudeText);
			TiXmlElement	*Latitude = new TiXmlElement("Latitude");
			TiXmlText		*LatitudeText = new TiXmlText("0.0");
			Latitude->LinkEndChild(LatitudeText);
			DeviceItem->LinkEndChild(DeviceID);
			DeviceItem->LinkEndChild(Name);
			DeviceItem->LinkEndChild(Manufacturer);
			DeviceItem->LinkEndChild(Model);
			DeviceItem->LinkEndChild(Owner);
			DeviceItem->LinkEndChild(CivilCode);
			DeviceItem->LinkEndChild(Block);
			DeviceItem->LinkEndChild(Address);
			DeviceItem->LinkEndChild(Parental);
			DeviceItem->LinkEndChild(ParentID);
			DeviceItem->LinkEndChild(SafetyWay);
			DeviceItem->LinkEndChild(RegisterWay);
			DeviceItem->LinkEndChild(CertNum);
			DeviceItem->LinkEndChild(Certifiable);
			DeviceItem->LinkEndChild(ErrCode);
			DeviceItem->LinkEndChild(EndTime);
			DeviceItem->LinkEndChild(Secrecy);
			DeviceItem->LinkEndChild(IPAddress);
			DeviceItem->LinkEndChild(Port);
			DeviceItem->LinkEndChild(Password);
			DeviceItem->LinkEndChild(Status);
			DeviceItem->LinkEndChild(Longitude);
			DeviceItem->LinkEndChild(Latitude);
			DeviceList->LinkEndChild(DeviceItem);
		}
	}
	Response->LinkEndChild(DeviceList);
	TiXmlPrinter printer; 
	DevDocument->Accept(&printer);
	cout<<"设备查询结果"<<endl;
	cout<<printer.CStr()<<endl;
	osip_message_set_body(answer,printer.CStr(),printer.Size());
	osip_message_set_content_type(answer,"Application/MANSCDP+xml");
	delete DevDocument;
	return true;

}
bool sip_manger::answer_query_recordInfo(osip_message_t *answer,string sip_id,string start_time,string end_time)
{
	struct tm st ={0};
	struct tm et ={0};
	time_t start_t = 0;
	time_t end_t = 0;
	if(sscanf(start_time.c_str(),"%4d-%2d-%2dT%2d:%2d:%2d",&st.tm_year,&st.tm_mon,&st.tm_mday,&st.tm_hour,&st.tm_min,&st.tm_sec)==6)
	{		
		st.tm_mon = st.tm_mon-1;
		st.tm_year = st.tm_year-1900;
		start_t  = mktime(&st)+TIME_OFFSET;
		cout<<start_time<<"--"<<start_t<<endl;
	}

	if(sscanf(end_time.c_str(),"%4d-%2d-%2dT%2d:%2d:%2d",&et.tm_year,&et.tm_mon,&et.tm_mday,&et.tm_hour,&et.tm_min,&et.tm_sec)==6)
	{
		et.tm_mon = et.tm_mon-1;
		et.tm_year = et.tm_year-1900;
		end_t  = mktime(&et)+TIME_OFFSET;
		cout<<end_time<<"--"<<end_t<<endl;
	}
	int video_id = g_vm->get_id_from_sip_id(sip_id);
	track_list	tl;
	g_sm->get_track_list_sql(video_id,tl,start_t,end_t);

	TiXmlDocument* RecDocument = new TiXmlDocument();
	TiXmlDeclaration	*td=new TiXmlDeclaration("1.0","","");
	RecDocument->LinkEndChild(td);
	TiXmlElement	*Response = new TiXmlElement("Response");
	RecDocument->LinkEndChild(Response);
	TiXmlElement	*CmdType = new TiXmlElement("CmdType");
	Response->LinkEndChild(CmdType);
	TiXmlText		*CmdText = new TiXmlText("RecordInfo");
	CmdType->LinkEndChild(CmdText);
	TiXmlElement	*SN = new TiXmlElement("SN");
	Response->LinkEndChild(SN);
	TiXmlText		*SnText = new TiXmlText("17430");
	SN->LinkEndChild(SnText);
	TiXmlElement	*DeviceID = new TiXmlElement("DeviceID");
	Response->LinkEndChild(DeviceID);
	TiXmlText		*DeviceIDText = new TiXmlText(sip_id.c_str());
	DeviceID->LinkEndChild(DeviceIDText);
	TiXmlElement	*SumNum = new TiXmlElement("SumNum");
	Response->LinkEndChild(SumNum);
	char	tempstr[128];
	sprintf(tempstr,"%d",tl.size());
	TiXmlText		*SumNumText = new TiXmlText(tempstr);
	SumNum->LinkEndChild(SumNumText);
	TiXmlElement	*RecordList = new TiXmlElement("RecordList");
	Response->LinkEndChild(RecordList);
	
	RecordList->SetAttribute("Num",tl.size());
	track_list::iterator item;
	if(tl.size())
	{
		for(item=tl.begin();item!=tl.end();item++)
		{
			TiXmlElement	*DeviceItem = new TiXmlElement("Item");
			TiXmlElement	*DeviceID = new TiXmlElement("DeviceID");
			TiXmlText		*DeviceIDText = new TiXmlText(sip_id.c_str());
			DeviceID->LinkEndChild(DeviceIDText);
			TiXmlElement	*Name = new TiXmlElement("Name");
			TiXmlText		*NameText = new TiXmlText(sip_id.c_str());
			Name->LinkEndChild(NameText);
			TiXmlElement	*FilePath = new TiXmlElement("FilePath");
			TiXmlText		*FilePathText = new TiXmlText("");
			FilePath->LinkEndChild(FilePathText);
			TiXmlElement	*Address = new TiXmlElement("Address");
			TiXmlText		*AddressText = new TiXmlText("未知");
			Address->LinkEndChild(AddressText);
			TiXmlElement	*StartTime = new TiXmlElement("StartTime");
			struct tm		*st_tb;
			time_t			st_t = (*item).st;
			if(start_t>st_t)
			{
				st_t = start_t;
			}
			st_t-=TIME_OFFSET;
			st_tb = localtime(&st_t);
			char 			timestr[128];
			sprintf(timestr,"%4d-%02d-%02dT%02d:%02d:%02d",st_tb->tm_year+1900,st_tb->tm_mon+1,st_tb->tm_mday,st_tb->tm_hour,st_tb->tm_min,st_tb->tm_sec);
			TiXmlText		*StartTimeText = new TiXmlText(timestr);
			StartTime->LinkEndChild(StartTimeText);
			TiXmlElement	*EndTime = new TiXmlElement("EndTime");
			struct tm		*et_tb;
			time_t			et_t = (*item).et;
			if(et_t==9999999999)
			{
				et_t = time(NULL)+TIME_OFFSET;
			}
			
			if(end_t<et_t)
			{
				et_t = end_t;
			}
			et_t-=TIME_OFFSET;
			et_tb = localtime(&et_t);
			sprintf(timestr,"%4d-%02d-%02dT%02d:%02d:%02d",et_tb->tm_year+1900,et_tb->tm_mon+1,et_tb->tm_mday,et_tb->tm_hour,et_tb->tm_min,et_tb->tm_sec);
			TiXmlText		*EndTimeText = new TiXmlText(timestr);
			EndTime->LinkEndChild(EndTimeText);
			TiXmlElement	*Secrecy = new TiXmlElement("Secrecy");
			TiXmlText		*SecrecyText = new TiXmlText("0");
			Secrecy->LinkEndChild(SecrecyText);

			TiXmlElement	*Type = new TiXmlElement("Type");
			TiXmlText		*TypeText = new TiXmlText("time");
			Type->LinkEndChild(TypeText);

			TiXmlElement	*RecorderID = new TiXmlElement("RecorderID");
			sprintf(timestr,"%d",(*item).track_id);
			TiXmlText		*RecorderIDText = new TiXmlText(timestr);
			RecorderID->LinkEndChild(RecorderIDText);

			DeviceItem->LinkEndChild(DeviceID);
			DeviceItem->LinkEndChild(Name);
			DeviceItem->LinkEndChild(FilePath);
			DeviceItem->LinkEndChild(Address);
			DeviceItem->LinkEndChild(StartTime);
			DeviceItem->LinkEndChild(EndTime);
			DeviceItem->LinkEndChild(Secrecy);
			DeviceItem->LinkEndChild(Type);
			DeviceItem->LinkEndChild(RecorderID);
			RecordList->LinkEndChild(DeviceItem);


			/*
			delete RecorderIDText;
			delete RecorderID;
			delete TypeText;
			delete Type;
			delete Secrecy;
			delete SecrecyText;
			delete EndTimeText;
			delete EndTime;
			delete StartTimeText;
			delete StartTime;
			delete AddressText;
			delete Address;
			delete FilePath;
			delete FilePathText;
			delete Name;
			delete NameText;
			delete DeviceID;
			delete DeviceIDText;
			delete DeviceItem;
			*/
		}
	}
	TiXmlPrinter printer; 
	RecDocument->Accept(&printer);
	cout<<"录像查询结果"<<endl;
	cout<<printer.CStr()<<endl;
	osip_message_set_body(answer,printer.CStr(),printer.Size());
	osip_message_set_content_type(answer,"Application/MANSCDP+xml");
	delete RecDocument;
	return true;
}	