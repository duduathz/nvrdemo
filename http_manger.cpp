#include "http_manger.h"
#include "Units.h"
#include "sql_manger.h"
#include "http_manger.h"
#include "video_manger.h"
#include "sip_manger.h"
#include "nvr_device_mgr.h"
extern video_manger				*g_vm;
extern sql_manger					*g_sm;
extern nvr_http_server		*g_pnvr_http_server;
extern sip_manger					*g_sipm;
extern nvr_device_mgr			*g_ndm;
extern bool					g_bnvr_run;
identitymap					g_idenmap;
bool								g_bshot = false;
int API_TimeToString(string &strDateStr,const time_t &timeData)
{
	char chTmp[64]={0};
	struct tm *p;
	p = localtime(&timeData);
	p->tm_year = p->tm_year + 1900;
	p->tm_mon = p->tm_mon + 1;
	sprintf(chTmp,"%04d-%02d-%02d %02d:%02d:%02d",
		p->tm_year, p->tm_mon, p->tm_mday,p->tm_hour,p->tm_min,p->tm_sec);
	strDateStr = chTmp;
	return 0;
}
nvr_http_server::nvr_http_server()
{
	max_connect = 100;
	pthread_rwlock_init(&m_rwlock,NULL);
}
nvr_http_server::~nvr_http_server()
{
	pthread_rwlock_destroy(&m_rwlock);
}
bool nvr_http_server::load_config()
{
	
	m_options[0]="document_root";
	m_options[1]="/etc/GVR/html";
	m_options[2]="listening_ports";
	m_options[3]="9000,9001s";
	m_options[4]="ssl_certificate";
	m_options[5]= "/etc/GVR/ssl_cert.pem";
	m_options[6]="num_threads";
	m_options[7]="5";
	m_options[8]=0;
	return true;
}
bool nvr_http_server::start_server(bool bv)
{
	m_ctx = mg_start(&event_handler, NULL, (const char **)m_options);
	assert(m_ctx != NULL);
	m_bverification = bv;
	//m_rwlock = PTHREAD_RWLOCK_INITIALIZER;
	cout<<"start http server"<<endl;
	return true;

}
bool nvr_http_server::stop_server()
{
	mg_stop(m_ctx);
}
void nvr_http_server::get_qsvar(const struct mg_request_info *request_info,
								const char *name, char *dst, size_t dst_len) {
									const char *qs = request_info->query_string;
									mg_get_var(qs, strlen(qs == NULL ? "" : qs), name, dst, dst_len);
}
void nvr_http_server::my_strlcpy(char *dst, const char *src, size_t len) {
	strncpy(dst, src, len);
	dst[len - 1] = '\0';
}
void nvr_http_server::redirect_to_login(struct mg_connection *conn,const struct mg_request_info *request_info)
{
	mg_printf(conn, "HTTP/1.1 302 Found\r\n"
		"Set-Cookie: original_url=%s\r\n"
		"Location: %s\r\n\r\n",
		request_info->uri, login_url);
}
static void redirect_to_index(struct mg_connection *conn,const struct mg_request_info *request_info)
{
	mg_printf(conn, "HTTP/1.1 302 Found\r\n"
		"Set-Cookie: original_url=%s\r\n"
		"Location: %s\r\n\r\n",
		request_info->uri, index_url);
}
void *nvr_http_server::event_handler(enum mg_event event,
struct mg_connection *conn,
	const struct mg_request_info *request_info) {
		void *processed =(void *) "yes"; //没处理过置为null,默认请求该uri的文件 
		if (event == MG_NEW_REQUEST) {
			if(request_info->query_string)
			{
				cout<<request_info->uri<<"?"<<request_info->query_string<<endl;
			}
			if(strstr(request_info->uri,"resources"))
			{
				return NULL;
			}
			if(strstr(request_info->uri,"images"))
			{
				return NULL;
			}
			if(strstr(request_info->uri,"favicon.ico"))
			{
				return NULL;
			}
			if (strcmp(request_info->uri, "/shot") == 0)
			{
				return NULL;
				if(g_bshot==false)
				{
					g_bshot = true;
					process_shot(conn, request_info);
					g_bshot = false;
				}
				else
				{
					string str = "busy!";
					mg_printf(conn,"HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: %d\r\n\r\n",str.size());
					mg_printf(conn,str.c_str());
					mg_printf(conn,"\r\n\r\n");
				}
				return processed;
			}
			/*
			if(strcmp(request_info->uri,login_url)==0)
			{
				return NULL;
			}
			
			if(strcmp(request_info->uri,index_url)==0)
			{
				return NULL;
			}
			*/
			if (strcmp(request_info->uri, "/version") == 0)
			{
				char vstr[128];
				sprintf(vstr,"%s",current_version);
				string str = string(vstr);
				mg_printf(conn,"HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: %d\r\n\r\n",str.size());
				mg_printf(conn,str.c_str());
				mg_printf(conn,"\r\n\r\n");
			}
			if (strcmp(request_info->uri, "/login") == 0)
			{
				process_login(conn, request_info);
			}
			else
			{			
				if(process_identity(conn,request_info))
				{
					if(strcmp(request_info->uri, "/set") == 0){
						process_set(conn, request_info);
					}else if (strcmp(request_info->uri, "/cmd") == 0){
						process_cmd(conn, request_info);
					} else if (strcmp(request_info->uri, "/search") == 0){
						process_search(conn, request_info);
					} else if (strcmp(request_info->uri, "/get") == 0){
						process_get(conn, request_info);
					} else if(strcmp(request_info->uri, "/video")==0){
						process_video(conn, request_info);
					} else {
						// No suitable handler found, mark as not processed. Mongoose will
						// try to serve the request.
						processed = NULL;
					}
				}
				else
				{
					/*
					cout<<"页面转向"<<endl;
					redirect_to_login(conn, request_info);
					*/
					if (request_info->is_ssl)
					{
						redirect_to_login(conn, request_info);
					}
					else
					{
						cout<<"页面转向发送错误"<<endl;
						mg_send_http_error(conn,403, "Forbidden", "Access Forbidden");
					}
					
				}
			}
		}
		else
		{
			processed = NULL;
		}
		return processed;
}
bool nvr_http_server::process_identity(struct mg_connection *conn,const struct mg_request_info *request_info)
{
	char	pstr[128];
	if (request_info->is_ssl)
	{
		if (!strcmp(request_info->uri, login_url) ||
			!strcmp(request_info->uri, "/login")) {
				return true;
		}
		mg_get_cookie(conn, "session", pstr, sizeof(pstr));
	}
	else
	{
		get_qsvar(request_info, "identity", pstr, 128);
	}
	pthread_rwlock_wrlock(&rwlock);
	identitymap::iterator item = g_idenmap.find(atol(pstr));
	if(item!=g_idenmap.end())
	{
		time_t t = time(NULL)+TIME_OFFSET;
		if(t<item->first+item->second)
		{
			item = g_idenmap.begin();
			for(;item!=g_idenmap.end();item++)
			{
				if(t>item->first+item->second)
				{
					g_idenmap.erase(item);
					item =g_idenmap.begin();
				}
			}
			pthread_rwlock_unlock(&rwlock);
			return true;
		}
		else
		{
			g_idenmap.erase(item);
		}
	}
	pthread_rwlock_unlock(&rwlock);
	return false;
}
void nvr_http_server::process_shot(struct mg_connection *conn,const struct mg_request_info *request_info)
{
	char	var_action[128];
	char	var_callback[128];
	char	var_id[128];
	char	image_url[128];
	char	cmd_str[128];
	Json::Value	root;
	Json::FastWriter 		fast_writer;
	root["ret"]="1";
	char	var_temp[128];
	gbk2utf8(var_temp,"获取成功",128);
	root["msgtext"]=var_temp;
	get_qsvar(request_info, "action", var_action, sizeof(var_action));
	get_qsvar(request_info, "callback", var_callback, sizeof(var_callback));
	get_qsvar(request_info,"id",var_id,sizeof(var_id));
	int id = 0;
	bool have = false;;
	pthread_rwlock_wrlock(&ffmpeg_rwlock);
	if(strcmp(var_id,"")!=0)
	{
		id = atoi(var_id);
		have = g_vm->have_shot(id);
	}
	else
	{
		get_qsvar(request_info,"dev_id",var_id,sizeof(var_id));
		id = g_vm->get_id_from_sip_id(string(var_id));
		cout<<"设备"<<id<<"开始截图,截图开始时间"<<time(NULL)<<endl;
		have = g_vm->have_shot(id);
		sleep(1);
	}
	if(have)
	{		
		time_t t = time(NULL)+TIME_OFFSET;
		sprintf(cmd_str,"rm -rf %s%d.jpg",SHOT_FILE_DIR,id);
		system(cmd_str);
		sprintf(cmd_str,"ffmpeg -i %s%d.dat %s%d.jpg",SHOT_FILE_DIR,id,SHOT_FILE_DIR,id);
		system(cmd_str);
		sprintf(image_url,"images/%d.jpg",id);
		memset(cmd_str,0,128);
		sprintf(cmd_str,"%s%d.jpg",SHOT_FILE_DIR,id);
		int ret = access(cmd_str,R_OK);
		if(request_info->is_ssl==false)
		{
			root["image"] = string(image_url);
			root["size"] = 1;
		}
		else
		{
			mg_printf(conn, "HTTP/1.1 302 Found\r\n"
				"Set-Cookie: original_url=%s\r\n"
				"Location: %s\r\n\r\n",
				request_info->uri, image_url);
			cout<<"设备"<<id<<"截图结束,截图结束时间"<<time(NULL)<<endl;
			pthread_rwlock_unlock(&ffmpeg_rwlock);
			return;
		}
		
	}
	else
	{
		root["ret"]="0";
		gbk2utf8(var_temp,"获取失败",128);
		root["msgtext"]=var_temp;
	}
	pthread_rwlock_unlock(&ffmpeg_rwlock);
	string str = fast_writer.write(root);
	if(strcmp(var_callback,"")!=0)
	{
		str = string(var_callback)+"(["+str+"])";
		mg_printf(conn,"HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: %d\r\n\r\n",str.size());
		mg_printf(conn,str.c_str());
		mg_printf(conn,"\r\n\r\n");
	}
	else
	{
		mg_printf(conn,"HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: %d\r\n\r\n",str.size());
		mg_printf(conn,str.c_str());
		mg_printf(conn,"\r\n\r\n");
	}
	cout<<"设备"<<id<<"截图结束,截图结束时间"<<time(NULL)<<endl;
	return;
	
}
void nvr_http_server::process_login(struct mg_connection *conn,const struct mg_request_info *request_info)
{
	char	var_admin[128];
	char	var_pass[128];
	char	realpass[128];
	char	callback[128];
	//if (request_info->is_ssl)
	//{
	//	get_qsvar(request_info, "user", var_admin, sizeof(var_admin));
	//	get_qsvar(request_info, "password", var_pass, sizeof(var_pass));
	//}
	//else
	{
		get_qsvar(request_info, "admin", var_admin, sizeof(var_admin));
		get_qsvar(request_info, "pass", var_pass, sizeof(var_pass));
	}
	read_conf_value("adminpassword",realpass,CONFIG_FILE);
	get_qsvar(request_info,"callback",callback,sizeof(callback));
	string	str;
	if(strcmp(var_pass,realpass)==0)
	{
		pthread_rwlock_wrlock(&rwlock);
		time_t	t;
		t = time(NULL)+TIME_OFFSET;
		g_idenmap.insert(map<time_t,unsigned int>::value_type(t,1800));
		char	identity[64];
		sprintf(identity,"%d",t);
		str = string(identity);
		if (request_info->is_ssl)
		{
			mg_printf(conn, "HTTP/1.1 302 Found\r\n"
				"Set-Cookie: session=%d; max-age=3600; http-only\r\n"  // Session ID
				"Set-Cookie: user=%s\r\n"  // Set user, needed by Javascript code
				"Set-Cookie: original_url=/; max-age=0\r\n"  // Delete original_url
				"Location: /\r\n\r\n",
				t, var_admin);
		}

		pthread_rwlock_unlock(&rwlock);
	}
	else
	{
		str = "0";
		if (request_info->is_ssl)
		{
			redirect_to_login(conn, request_info);
		}
	}
	if (!request_info->is_ssl)
	{
		if(strcmp(callback,"")!=0)
		{
			str = string(callback)+"(["+str+"])";
			mg_printf(conn,"HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: %d\r\n\r\n",str.size());
			mg_printf(conn,str.c_str());
			mg_printf(conn,"\r\n\r\n");
		}
		else
		{
			mg_printf(conn,"HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: %d\r\n\r\n",str.size());
			mg_printf(conn,str.c_str());
			mg_printf(conn,"\r\n\r\n");
#ifdef	_DEBUG
			cout<<str<<endl;
#endif
		}
	}


}
void nvr_http_server::process_cmd(struct mg_connection *conn,const struct mg_request_info *request_info)
{
	char	var_action[128];
	char	var_callback[128];
	char	var_temp[128];
	Json::Value	root;
	Json::FastWriter 		fast_writer;
	get_qsvar(request_info, "action", var_action, sizeof(var_action));
	get_qsvar(request_info, "callback", var_callback, sizeof(var_callback));
	string action = string(var_action);

	if(action=="adddev"||action=="addipc")
	{
		char	var_name[128];
		char	var_ip[128];
		char	var_port[128];
		char	var_channel[128];
		char	var_user[128];
		char	var_type[128];
		char	var_pass[128];
		get_qsvar(request_info, "name", var_name, sizeof(var_name));
		get_qsvar(request_info, "addr", var_ip, sizeof(var_ip));
		get_qsvar(request_info, "type", var_type, sizeof(var_type));
		get_qsvar(request_info, "port", var_port, sizeof(var_port));
		get_qsvar(request_info, "user", var_user, sizeof(var_user));
		get_qsvar(request_info, "pass", var_pass, sizeof(var_pass));
		get_qsvar(request_info, "channel", var_channel, sizeof(var_channel));
		char	var_sip_id[128];
		get_qsvar(request_info, "sipid", var_sip_id, sizeof(var_sip_id));
		bool ret = g_vm->add_video_stream(atoi(var_type),string(var_name),string(var_ip),atoi(var_port),string(var_user),string(var_pass),string(var_channel),string(var_sip_id));
		if(ret)
		{
			root["ret"]="1";
			gbk2utf8(var_temp,"设备添加成功",128);
			root["msgtext"]=var_temp;
			//root["msgtext"]="设备添加成功";
		}
		else
		{
			root["ret"]="0";
			gbk2utf8(var_temp,"设备添加失败",128);
			root["msgtext"]=var_temp;
			//root["msgtext"]="设备添加失败";
		}

	}
	else if(action=="updatedev"||action=="updateipc")
	{
		char	var_device_id[128];
		char	var_name[128];
		char	var_ip[128];
		char	var_port[128];
		char	var_channel[128];
		char	var_user[128];
		char	var_type[128];
		char	var_pass[128];
		get_qsvar(request_info, "sipid", var_device_id, sizeof(var_device_id));
		get_qsvar(request_info, "name", var_name, sizeof(var_name));
		get_qsvar(request_info, "addr", var_ip, sizeof(var_ip));
		get_qsvar(request_info, "type", var_type, sizeof(var_type));
		get_qsvar(request_info, "port", var_port, sizeof(var_port));
		get_qsvar(request_info, "user", var_user, sizeof(var_user));
		get_qsvar(request_info, "pass", var_pass, sizeof(var_pass));

		int id = g_vm->get_id_from_sip_id(string(var_device_id));
		bool ret = g_vm->update_video_stream(id,atoi(var_type),string(var_name),string(var_ip),atoi(var_port),string(var_user),string(var_pass),string(var_channel));
		if(ret)
		{
			root["ret"]="1";
			gbk2utf8(var_temp,"设备更新成功",128);
			root["msgtext"]=var_temp;
			//root["msgtext"]="设备更新成功";
		}
		else
		{
			root["ret"]="0";
			gbk2utf8(var_temp,"设备更新失败",128);
			root["msgtext"]=var_temp;
			//root["msgtext"]="设备更新失败";
		}

	}
	else if(action=="startdev"||action=="startipc")
	{
		char	var_id[128];
		get_qsvar(request_info, "id", var_id, sizeof(var_id));
		int id = g_vm->get_id_from_sip_id(string(var_id));
		bool ret = g_vm->start_video_stream(id);
		if(ret)
		{
			root["ret"]="1";
			gbk2utf8(var_temp,"设备启动成功",128);
			root["msgtext"]=var_temp;
			//root["msgtext"]="设备启动成功";
		}
		else
		{
			root["ret"]="0";
			gbk2utf8(var_temp,"设备启动失败",128);
			root["msgtext"]=var_temp;
			//root["msgtext"]="设备启动失败";
		}
	}
	else if(action=="stopdev"||action=="stopipc")
	{
		char	var_id[128];
		get_qsvar(request_info, "id", var_id, sizeof(var_id));
		int id = g_vm->get_id_from_sip_id(string(var_id));
		bool ret = g_vm->stop_video_stream(id);
		if(ret)
		{
			root["ret"]="1";
			gbk2utf8(var_temp,"设备停止成功",128);
			root["msgtext"]=var_temp;
			//root["msgtext"]="设备停止成功";
		}
		else
		{
			root["ret"]="0";
			gbk2utf8(var_temp,"设备停止失败",128);
			root["msgtext"]=var_temp;
			//root["msgtext"]="设备停止失败";
		}
	}
	else if(action=="deldev"||action=="delipc")
	{
		char	var_id[128];
		get_qsvar(request_info, "id", var_id, sizeof(var_id));
		int id = g_vm->get_id_from_sip_id(string(var_id));
		bool ret = g_vm->delete_video_stream(id);
		if(ret)
		{
			root["ret"]="1";
			gbk2utf8(var_temp,"设备卸载成功",128);
			root["msgtext"]=var_temp;
			//root["msgtext"]="设备卸载成功";
		}
		else
		{
			root["ret"]="0";
			gbk2utf8(var_temp,"设备卸载失败",128);
			root["msgtext"]=var_temp;
			//root["msgtext"]="设备卸载失败";
		}
	}
	else if(action=="updatevideoplan")
	{
		char	var_id[128];
		char	var_plan[128];
		char	var_maxday[128];
		get_qsvar(request_info, "id", var_id, sizeof(var_id));
		get_qsvar(request_info, "plan", var_plan, sizeof(var_plan));
		get_qsvar(request_info, "maxday", var_maxday, sizeof(var_maxday));
		int id = g_vm->get_id_from_sip_id(string(var_id));

		bool ret = g_sm->update_device_plan_sql(id,string(var_plan),atoi(var_maxday));
		if(ret)
		{
			root["ret"]="1";
			gbk2utf8(var_temp,"录像计划更新成功,将在下一个整点生效",128);
			root["msgtext"]=var_temp;
		}
		else
		{
			root["ret"]="0";
			gbk2utf8(var_temp,"录像计划更新失败",128);
			root["msgtext"]=var_temp;
		}


	}
	else if(action=="restart")
	{
		g_bnvr_run = false;
	}
	else
	{
		root["ret"]="0";
		gbk2utf8(var_temp,"参数错误",128);
		root["msgtext"]=var_temp;
		//root["msgtext"]="参数错误";
	}
	string str = fast_writer.write(root);
	if(strcmp(var_callback,"")!=0)
	{
		str = string(var_callback)+"(["+str+"])";
		mg_printf(conn,"HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: %d\r\n\r\n",str.size());
		mg_printf(conn,str.c_str());
		mg_printf(conn,"\r\n\r\n");
	}
	else
	{
		mg_printf(conn,"HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: %d\r\n\r\n",str.size());
		mg_printf(conn,str.c_str());
		mg_printf(conn,"\r\n\r\n");
	}
	cout<<str<<endl;
	return ;

}
void nvr_http_server::process_search(struct mg_connection *conn,const struct mg_request_info *request_info)
{
	char	var_1[MAX_ARG_LEN];
	char	var_2[MAX_ARG_LEN];
	char	var_3[MAX_ARG_LEN];
	char	var_4[MAX_ARG_LEN];
	char	size[MAX_NAME_LEN];
	char	callback[MAX_NAME_LEN];
	Json::Value	root;
	Json::FastWriter 		fast_writer;
	get_qsvar(request_info,"name",var_1,sizeof(var_1));
	get_qsvar(request_info,"callback",callback,sizeof(callback));
	string namestr(var_1);
	root["ret"]="1";
	root["msgtext"]=gettext("got it");
	if(namestr=="tag")
	{
		
	}
	string str = fast_writer.write(root);
	if(strcmp(callback,"")!=0)
	{
		str = string(callback)+"(["+str+"])";
		mg_printf(conn,"HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: %d\r\n\r\n",str.size());
		mg_printf(conn,str.c_str());
		mg_printf(conn,"\r\n\r\n");
	}
	else
	{
		mg_printf(conn,"HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: %d\r\n\r\n",str.size());
		mg_printf(conn,str.c_str());
		mg_printf(conn,"\r\n\r\n");
#ifdef	_DEBUG
		cout<<str<<endl;
#endif
	}
}
void nvr_http_server::process_get(struct mg_connection *conn,const struct mg_request_info *request_info)
{
	char	var_name[128];
	char	var_callback[128];
	char	var_temp[128];
	char	var_tmp[128];
	
	Json::Value	root;
	Json::FastWriter 		fast_writer;
	get_qsvar(request_info,"name",var_name,sizeof(var_name));	
	get_qsvar(request_info,"callback",var_callback,sizeof(var_callback));
	string namestr(var_name);
	root["ret"]="1";
	strcpy(var_tmp,"获取成功");
	gbk2utf8(var_temp,var_tmp,128);
	root["msgtext"]=var_temp;
	if(namestr=="system")
	{
		g_ndm->updateinfo();
		Json::Value disk;
		root["nvr_type"]="nvr-h3c for linux";
		root["nvr_version"]="2.0.2";
		root["system"]=g_ndm->str_system_kernel_version;
		root["cpu"]=g_ndm->str_cpu_type;
		root["ip"]=g_ndm->ip_addr;
		root["mask"]=g_ndm->mask_addr;
		root["mac"]=g_ndm->mac_addr;
		root["gateway"]=g_ndm->gateway_addr;
		root["dns"]=g_ndm->dns_addr;
		hdlist::iterator item;
		for(item=g_ndm->m_hdlist.begin();item!=g_ndm->m_hdlist.end();item++)
		{
			disk["device"]=(*item).device;
			disk["mount_point"]=(*item).mount_point;
			disk["total"]=g_ndm->kscale((*item).size,1);
			disk["free"]=g_ndm->kscale((*item).free,1);
			root["disk"].append(disk);
		}

	}
	else if(namestr=="ipctypelist"||namestr=="devtypelist")
	{
		Json::Value	typeValue;
		typeValue["id"] = "0";
		typeValue["name"] = "H3C";
		root[namestr].append(typeValue);
		root["size"] = 1;
	}
	else if(namestr=="ipclist"||namestr=="devlist")
	{
		char	tempstr[64];
		Json::Value	deviceValue;
		stream_manger_map::iterator	item;
		for (item=g_vm->m_manger_map.begin();item!=g_vm->m_manger_map.end();item++)
		{	
			sprintf(tempstr,"%ld",item->second->m_video_id);
			deviceValue["id"] = string(tempstr);
			deviceValue["name"] = item->second->m_device_name;
			sprintf(tempstr,"%ld",item->second->m_device_status);
			deviceValue["status"] = string(tempstr);
			sprintf(tempstr,"%ld",item->second->m_device_type);
			deviceValue["type"] = string(tempstr);
			deviceValue["ip"] = item->second->m_device_addr;
			//sprintf(tempstr,"%ld",item->second->m_device_channel);
			deviceValue["channel"] = item->second->m_device_channel;
			deviceValue["sipid"] = item->second->m_device_sip_id;
			root[namestr].append(deviceValue);
		}
		root["size"] = (int)g_vm->m_manger_map.size();
	}
	else if(namestr=="ipc"||namestr=="dev")
	{
		device_list	dl;
		g_sm->get_device_list_sql(dl);
		char	var_sipid[128];
		char	tempstr[64];
		bool 	bfind = false;
		get_qsvar(request_info,"sipid",var_sipid,sizeof(var_sipid));
		if(dl.size())
		{
			device_list::iterator item;
			for(item=dl.begin();item!=dl.end();item++)
			{
				if(item->sip_id==string(var_sipid))
				{
					Json::Value	deviceValue;
					sprintf(tempstr,"%ld",item->device_id);
					deviceValue["id"] = string(tempstr);
					deviceValue["name"] = item->name;
					sprintf(tempstr,"%ld",item->status);
					deviceValue["status"] = string(tempstr);
					sprintf(tempstr,"%ld",item->type);
					deviceValue["type"] = string(tempstr);
					deviceValue["ip"] = item->ip;
					deviceValue["channel"] = item->channel;
					deviceValue["sipid"] = item->sip_id;
					root[namestr].append(deviceValue);
					bfind = true;
				}
			}
		}
		if(bfind==false)
		{
			root["ret"]="0";
			//gbk2utf8(var_temp,"获取失败",128);
			root["msgtext"]="获取失败";
			cout<<"没有找到该设备"<<endl;
		}
		
	}
	else if(namestr=="videotrack")
	{
		
		track_list	tl;
		char	var_id[128];
		char	var_st[128];
		char	var_et[128];
		char	var_key[128];
		char	tempstr[64];
		get_qsvar(request_info,"id",var_id,sizeof(var_id));
		get_qsvar(request_info,"st",var_st,sizeof(var_id));
		get_qsvar(request_info,"et",var_et,sizeof(var_id));
		get_qsvar(request_info,"key",var_key,sizeof(var_id));
		g_sm->get_track_list_sql(atoi(var_id),tl,atol(var_st),atol(var_et));
		if(tl.size())
		{
			track_list::iterator item;
			for(item=tl.begin();item!=tl.end();item++)
			{
				Json::Value	trackValue;
				sprintf(tempstr,"%ld",(*item).track_id);
				trackValue["id"] =string(tempstr);
				string st,et;
				API_TimeToString(st,item->st);
				trackValue["st"] = st;
				API_TimeToString(et,item->et);
				trackValue["et"] =et;
				trackValue["status"] = item->status;
				root[namestr].append(trackValue);
			}
		}
	}
	else if(namestr=="sipconfig")
	{
		Json::Value		setting;
		setting["sip_server_port"]=g_sipm->m_sport_str;
		setting["sip_server_ip"] = g_sipm->m_sip_server;
		setting["sip_server_id"] = g_sipm->m_sip_id;
		setting["media_server_id"] = g_sipm->m_device_id;
		setting["media_server_port"]= g_sipm->m_lport;
		setting["media_server_pass"] = g_sipm->m_device_pass;
		setting["server_name"] = g_sipm->m_sip_server;
		root[namestr].append(setting);

	}
	string str = fast_writer.write(root);
	if(strcmp(var_callback,"")!=0)
	{
		str = string(var_callback)+"(["+str+"])";
		mg_printf(conn,"HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: %d\r\n\r\n",str.size());
		mg_printf(conn,str.c_str());
		mg_printf(conn,"\r\n\r\n");
	}
	else
	{
		mg_printf(conn,"HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: %d\r\n\r\n",str.size());
		mg_printf(conn,str.c_str());
		mg_printf(conn,"\r\n\r\n");
	}
	return;

}
void nvr_http_server::process_set(struct mg_connection *conn,const struct mg_request_info *request_info)
{
	Json::Value	root;
	Json::FastWriter 		fast_writer;
	char	name[MAX_NAME_LEN];
	char	callback[MAX_NAME_LEN];

	get_qsvar(request_info,"name",name,sizeof(name));
	get_qsvar(request_info,"callback",callback,sizeof(callback));
	string namestr(name);
	root["ret"]="1";
	root["msgtext"]=gettext("done it");
	if(namestr=="mountpoint")
	{

	}
	else if(namestr=="adminpassword")
	{
		char	pass[128];
		get_qsvar(request_info,"pass",pass,sizeof(pass));
		write_conf_value("adminpassword",pass,CONFIG_FILE);
	}
	else if(namestr=="httpport")
	{
		char	port[128];
		get_qsvar(request_info,"port",port,sizeof(port));
		write_conf_value("http_port",port,CONFIG_FILE);
	}
	else if(namestr=="maxconnection")
	{
		char	str[128];
		get_qsvar(request_info,"max",str,sizeof(str));
		g_pnvr_http_server->max_connect = atol(str);
	}
	else if(namestr=="network")
	{
		char	ipstr[64];
		get_qsvar(request_info,"ip",ipstr,sizeof(ipstr));
		char	maskstr[64];
		get_qsvar(request_info,"mask",maskstr,sizeof(maskstr));
		char	gatewaystr[64];
		get_qsvar(request_info,"gateway",gatewaystr,sizeof(gatewaystr));
		char	dnstr[64];
		get_qsvar(request_info,"dns",dnstr,sizeof(dnstr));
		char	stripmask[64];
		sprintf(stripmask,"ifconfig eth0 %s netmask %s",ipstr,maskstr);
		system(stripmask);
#ifdef _DEBUG
		cout<<"ifconfig eth0 "<<endl;
#endif
		char	strgw[64];
		sprintf(strgw,"route add default gw %s",gatewaystr);
		system(strgw);
#ifdef _DEBUG
		cout<<"route add default "<<endl;
#endif
		char	strdns[128];
		sprintf(strdns,"echo -e nameserver %s > /etc/resolv.conf",dnstr);
		system(strdns);
#ifdef	_DEBUG
		cout<<"echo -e nameserver "<<endl;
#endif
		write_conf_value("server_ip",ipstr,CONFIG_FILE);
		write_conf_value("mask_ip",maskstr,CONFIG_FILE);
		write_conf_value("gateway",gatewaystr,CONFIG_FILE);
		write_conf_value("dns",dnstr,CONFIG_FILE);
		//ipc 重连就可以了， 录像不用中断
	}
	else if(namestr=="servertime")
	{
		char	cmdstr[128];
		char	timestr[128];
		get_qsvar(request_info,"time",timestr,128);
		time_t	at = atol(timestr);
		struct tm *p = gmtime(&at);
		sprintf(cmdstr,"date -s \"%04d%02d%02d %02d:%02d:%02d\"",1900+p->tm_year,p->tm_mon+1,p->tm_mday,p->tm_hour,p->tm_min,p->tm_sec);
		cout<<cmdstr<<endl;
		system(cmdstr);
		system("clock -w");
	}
	else if(namestr=="lockrecord")
	{
		root["ret"]="0";
		root["msgtext"]=gettext("can't do it");
	}
	else if(namestr=="unlockrecord")
	{
		root["ret"]="0";
		root["msgtext"]=gettext("can't do it");
	}
	else if(namestr=="sip_udp_port")
	{
		char	port[128];
		get_qsvar(request_info,"port",port,sizeof(port));
		write_conf_value("sip_udp_port",port,CONFIG_FILE);
	}
	else if(namestr=="rtsp_port")
	{
		char	port[128];
		get_qsvar(request_info,"port",port,sizeof(port));
		write_conf_value("rtsp_port",port,CONFIG_FILE);
	}
	else if(namestr=="ntp_server")
	{
		char	cmdstr[128];
		char	addr[128];
		get_qsvar(request_info,"addr",addr,128);
		sprintf(cmdstr,"ntpdate %s",addr);
		system(cmdstr);
		write_conf_value("ntp_server",addr,CONFIG_FILE);
	}
	else if(namestr=="syslog_server")
	{
		char	server[128];
		get_qsvar(request_info,"addr",server,128);
		//    /etc/syslog.conf   and restart syslogd

	}
	string str = fast_writer.write(root);
	if(strcmp(callback,"")!=0)
	{
		str = string(callback)+"(["+str+"])";
		mg_printf(conn,"HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: %d\r\n\r\n",str.size());
		mg_printf(conn,str.c_str());
		mg_printf(conn,"\r\n\r\n");
	}
	else
	{
		mg_printf(conn,"HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: %d\r\n\r\n",str.size());
		mg_printf(conn,str.c_str());
		mg_printf(conn,"\r\n\r\n");
	}
}
void nvr_http_server::process_video(struct mg_connection *conn,const struct mg_request_info *request_info)
{
	Json::Value	cmd;
	Json::FastWriter 		fast_writer;
	channel		cc;
	get_qsvar(request_info,"id",cc.id,16);
	get_qsvar(request_info,"st",cc.st,16);
	get_qsvar(request_info,"et",cc.et,16);
	get_qsvar(request_info,"sipid",cc.sipid,128);
	if(g_pnvr_http_server->m_channelist.size()<=g_pnvr_http_server->max_connect)
	{
		g_pnvr_http_server->m_channelist.insert(map<int ,channel>::value_type(mg_get_sock(conn),cc));
		struct mg_connection *newconn = mg_move_conn(conn,g_pnvr_http_server->m_ctx);
		pthread_t	pid2;
		int ret = pthread_create(&pid2,NULL,send_stream_thread,newconn);
	}
	else
	{
		mg_send_http_error(conn,500, "Max connnection", "Access Forbidden");
	}
}
void *nvr_http_server::send_stream_thread(void *lp)
{
	int		sendlen = 0;
	struct mg_connection *conn = (mg_connection *)lp;
	char	var_1[MAX_ARG_LEN];
	char	var_2[MAX_ARG_LEN];
	char	var_3[MAX_ARG_LEN];
	char	var_4[128];
	channel_map::iterator	item;
	unsigned int length = 0;
	item = g_pnvr_http_server->m_channelist.find(mg_get_sock(conn));
	if(item==g_pnvr_http_server->m_channelist.end())
	{
		return NULL;
	}
	strcpy(var_1,item->second.id);
	strcpy(var_2,item->second.st);
	strcpy(var_3,item->second.et);
	strcpy(var_4,item->second.sipid);
	if(atol(var_1)!=0)
	{
		if(atol(var_2)!=0)//录像
		{
			mg_printf(conn,"HTTP/1.1 200 OK\r\nContent-Type:video/h264\r\nCache-Control:no-cache\r\nPragma:no-cache\r\n\r\n");


		}
		else //实时
		{


		}
	}
	else
	{
		mg_send_http_error(conn, 404, "Bad Request", "");
	}
	mg_close_conn(conn);
	g_pnvr_http_server->m_channelist.erase(item);
#ifdef	_DEBUG
	cout<<"a connection was over"<<endl;
#endif
	return NULL;
}


