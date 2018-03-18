#pragma once
#include "./include/stdprj.h"
#include "CppSQLite3.h"
class sql_manger
{
public:
	sql_manger(void);
	~sql_manger(void);
	
public:
	bool init_db();
	bool restore_db();
	bool fill_file_block(string path,unsigned int num,unsigned int offset);
	bool repair_db(time_t t);
private:
	bool create_db();
	CppSQLite3DB	m_db;
	pthread_rwlock_t		m_db_lock;
public:
	bool recycle_video_file_sql(unsigned int num);
	bool recycle_video_track_sql(time_t t);
	bool repair_video_file_sql(time_t t,int file_length);
	bool repair_video_track_sql(time_t t);

	int start_video_track_sql(unsigned int video_id,time_t st,const char *sdp);
	bool start_video_file_sql(unsigned int file_id,time_t st,unsigned int track_id,unsigned int video_id,unsigned int serial);
	bool stop_video_file_sql(unsigned int file_id,time_t et,int file_length);
	bool stop_video_track_sql(unsigned int track_id,time_t et);

	bool get_track_video_list_sql(unsigned int video_id,file_list &fl,time_t st);
	bool get_track_video_list_sql(unsigned int video_id,file_list &fl,time_t st,time_t et);

	bool get_device_list_sql(device_list &dl);
	bool get_track_list_sql(unsigned int video_id,track_list &tl,time_t st,time_t et);

	//故障状态只保存内存中
	int add_device_sql(unsigned int device_type,string device_name,string ip,unsigned int port,string username,string password,string channel,string sip_id);
	bool update_device_sql(unsigned int device_id,unsigned int device_type,string device_name,string ip,unsigned int port,string username,string password,string channel);
	bool update_device_plan_sql(unsigned int device_id,string plan,unsigned int maxday);
	bool start_device_sql(unsigned int device_id);
	bool start_record_device_sql(unsigned int device_id);
	bool stop_record_device_sql(unsigned int device_id);
	bool stop_device_sql(unsigned int device_id);
	bool delete_device_no_record_sql(unsigned int device_id);
	bool delete_device_sql(unsigned int device_id);
	bool add_etag_sql(unsigned int device_id,time_t t,string summary);


	int get_free_block(unsigned int num,file_list &fl);
	int recycle_file_block(unsigned int num);
	int get_file_block_num();

	bool add_video_etag(unsigned int video_id,time_t t,string summary);
	
};
