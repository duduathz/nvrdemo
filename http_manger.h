#pragma once
#include "stdprj.h"
#include "mongoose.h"
#define MAX_ACTION_LEN		20
#define MAX_COMMAND_LEN		20
#define	MAX_ARG_LEN			20
#define MAX_REQUEST_SIZE	15
#define MAX_SEARCH_LEN		128
#define MAX_NAME_LEN		128
#define MAX_TEXT_LEN		2048

static const char *options[] = {
	"document_root", "html",
	"listening_ports", "9000,9001s",
	"ssl_certificate", "ssl_cert.pem",
	"num_threads", "5",
	0
};
static const char *login_url = "/login.html";
static const char *index_url = "/index.html";
static pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
typedef map<time_t ,unsigned int>	identitymap;
static pthread_rwlock_t ffmpeg_rwlock = PTHREAD_RWLOCK_INITIALIZER;
struct channel
{
	char	id[16];
	char	st[16];
	char	et[16];
	char	sipid[128];
};
typedef map<int,channel>	channel_map;

class nvr_http_server
{
public:
	nvr_http_server(void);
	~nvr_http_server(void);
public:
	bool load_config();
	bool start_server(bool bv=false);
	bool stop_server();
private:
	char				*m_options[16];
	struct				mg_context *m_ctx;
	bool				m_bverification;
	pthread_rwlock_t	m_rwlock;
	int					m_nvideostream;
	int					m_nvr_status;
	channel_map			m_channelist;
	unsigned int		max_connect;
public:
	static void redirect_to_login(struct mg_connection *conn,const struct mg_request_info *request_info);
	static void redirect_to_index(struct mg_connection *conn,const struct mg_request_info *request_info);


	static void my_strlcpy(char *dst, const char *src, size_t len);
	static void get_qsvar(const struct mg_request_info *request_info,const char *name, char *dst, size_t dst_len);

	static void *event_handler(enum mg_event event,struct mg_connection *conn,const struct mg_request_info *request_info);
	static void process_login(struct mg_connection *conn,const struct mg_request_info *request_info);
	static void process_cmd(struct mg_connection *conn,const struct mg_request_info *request_info);
	static void process_search(struct mg_connection *conn,const struct mg_request_info *request_info);
	static void process_get(struct mg_connection *conn,const struct mg_request_info *request_info);
	static void process_set(struct mg_connection *conn,const struct mg_request_info *request_info);
	static void process_video(struct mg_connection *conn,const struct mg_request_info *request_info);
	static bool process_identity(struct mg_connection *conn,const struct mg_request_info *request_info);
	static void *send_stream_thread(void *lp);

	static void process_shot(struct mg_connection *conn,const struct mg_request_info *request_info);

};
