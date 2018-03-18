#include "sql_manger.h"
#include "bits.h"
#include "Units.h"
sql_manger::sql_manger(void)
{
	pthread_rwlock_init(&m_db_lock,NULL);
}

sql_manger::~sql_manger(void)
{
	pthread_rwlock_destroy(&m_db_lock);
}

bool sql_manger::init_db()
{
	m_db.open("/etc/GVR/gvr.db");
	if(m_db.tableExists("device")==false)
	{
		create_db();
	}
	restore_db();
	return true;
}
bool sql_manger::repair_db(time_t t)
{
	try
	{
		m_db.execDML("begin transaction;");
		CppSQLite3Buffer bufSQL;
		bufSQL.format("update video_file set file_status=%d where file_status=%d",FILE_ST_NEW,FILE_ST_PRE);
		m_db.execDML(bufSQL);
		cout<<"重置预分配文件块"<<endl;
		//bufSQL.format("update video_file set file_status=%d,end_time=%d where file_status=%d",FILE_ST_VIDEO,t,FILE_ST_ING);
		//m_db.execDML(bufSQL);
		//bufSQL.format("update video_track set end_time=%d,track_status=%d where track_status=%d",t,TRACK_ST_ED,TRACK_ST_ING);
		//m_db.execDML(bufSQL);
		m_db.execDML("commit transaction;");
START_REPAIR:
		int m = 0;
		cout<<"重新查询修复列表"<<endl;
		bufSQL.format("select * from video_file where file_status=%d",FILE_ST_ING);
		CppSQLite3Query query = m_db.execQuery(bufSQL);
		int size = sizeof(unsigned int)+sizeof(unsigned int)+sizeof(unsigned int)+MAX_FILE_INDEX*(sizeof(time_t)+sizeof(unsigned int));
		char * header = (char *)malloc(size);
		bits_buffer_s	bbs;
		bits_initwrite(&bbs,size,header);
		//m_db.execDML("begin transaction;");
		time_t	ti_st=0;
		while(query.eof()==false)
		{
			//读取对应设备的索引文件 找出文件尾的位置并更新数据库
			int len = 0;
			char	str[128];
			sprintf(str,"/etc/GVR/%d.idx",query.getIntField("video_id"));
			FILE	*fp= fopen(str,"rb+");
			if(fp)
			{
				int vid = 0;
				int tid = 0;
				int serial = 0;
				time_t	ti;
				int		offset = 0;
				fread(&vid,sizeof(int),1,fp);
				bits_write(&bbs,sizeof(int)*8,vid);
				fread(&tid,sizeof(int),1,fp);
				bits_write(&bbs,sizeof(int)*8,tid);
				fread(&serial,sizeof(int),1,fp);
				bits_write(&bbs,sizeof(int)*8,serial);
				cout<<"read "<<vid<<"---"<<tid<<"----"<<serial<<endl;
				for(int i=0;i<MAX_FILE_INDEX;i++)
				{

					int count = fread(&ti,sizeof(time_t),1,fp);
					//cout<<"read time "<<ti;
					if(count)
					{
						bits_write(&bbs,sizeof(time_t)*8,ti);
						fread(&offset,sizeof(int),1,fp);
						//cout<<"read offset "<<offset<<"--idx:"<<i<<endl;
						bits_write(&bbs,sizeof(int)*8,offset);
						ti_st = ti;
					}
					else
					{
						cout<<endl;
						break;
					}
				}
				cout<<"修复文件"<<query.getStringField("file_name")<<endl;
				FILE	*fpw = fopen(query.getStringField("file_name"),"rb+");
				if(fpw)
				{
					fseek(fpw,0,0);
					fwrite(header,size,1,fpw);
					fclose(fpw);
					fpw = NULL;
					bufSQL.format("update video_file set file_status=%d,end_time=%d,file_length=%d where file_id=%d",FILE_ST_VIDEO,ti,offset,query.getIntField("file_id"));
					cout<<"修复录像文件块数据表1"<<(const char *)bufSQL<<endl;
					m_db.execDML(bufSQL);
				}
				else
				{
					bufSQL.format("update video_file set file_status=%d,end_time=%d,file_length=0 where file_id=%d",FILE_ST_VIDEO,ti,query.getIntField("file_id"));
					cout<<"修复录像文件块数据表2"<<(const char *)bufSQL<<endl;
					m_db.execDML(bufSQL);
				}
				fclose(fp);
				fp = NULL;
				bufSQL.format("update video_track set end_time=%d,track_status=%d where track_id=%d",ti,TRACK_ST_ED,tid);
				cout<<"修复录像块数据表1号"<<(const char *)bufSQL<<endl;
				m_db.execDML(bufSQL);
			}
			m++;
			if(m>5)
			{
				cout<<"跳跃重新查询"<<endl;
				free(header);
				query.finalize();
				goto START_REPAIR;
			}
			query.nextRow();
		}
		if(ti_st==0)
		{
			ti_st = time(NULL)+TIME_OFFSET;
		}
		//m_db.execDML("commit transaction;");
		bufSQL.format("update video_track set end_time=%d, track_status=%d where track_status=%d",ti_st,TRACK_ST_ED,TRACK_ST_ING);
		cout<<"修复录像块数据表2号"<<(const char *)bufSQL<<endl;
		m_db.execDML(bufSQL);
		free(header);
		query.finalize();

	}
	catch (CppSQLite3Exception& e)
	{
		cout << "repari_db: " << e.errorCode() << ":" << e.errorMessage() << endl;
		return false;
	}
}
bool sql_manger::restore_db()
{
	//CopyFile(L"gvr.db",L"gvr_bak.db",false);
	return true;
}
bool sql_manger::create_db()
{
	try
	{
		m_db.execDML("begin transaction;");
		m_db.execDML("Create  TABLE [device]([device_id] integer PRIMARY KEY AUTOINCREMENT NOT NULL		\
					 ,[device_sip_id] char(128)			\
					 ,[device_name] char(128)			\
					 ,[type] smallint NOT NULL			\
					 ,[ip] char(128) NOT NULL \
					 ,[port] smallint		\
					 ,[channel] char(128)	\
					 ,[user] char(128)		\
					 ,[pass] char(128)		\
					 ,[video_plan] char(256)	\
					 ,[max_day] smallint		\
					 ,[status] smallint NOT NULL DEFAULT 0	\
					 ,[time] bigint			\
					 );");
		m_db.execDML("Create  TABLE [video_file]([file_id] integer PRIMARY KEY AUTOINCREMENT NOT NULL			\
					 ,[file_name] char(256) NOT NULL		\
					 ,[start_time] bigint				\
					 ,[end_time] bigint					\
					 ,[file_status] int					\
					 ,[video_id] int						\
					 ,[track_id] int						\
					 ,[serial_num] int					\
					 ,[file_sdp] char(256)				\
					 ,[file_length] int					\
					 );");
		m_db.execDML("Create  TABLE [video_track]([track_id] integer PRIMARY KEY AUTOINCREMENT NOT NULL			\
					 ,[video_id] int			\
					 ,[start_time] bigint	\
					 ,[end_time] bigint		\
					 ,[last_update_time] bigint \
					 ,[track_status] int		\
					 );");
		m_db.execDML("Create TABLE[video_etag]([tag_id] integer PRIMARY KEY AUTOINCREMENT NOT NULL			\
					 ,[video_id] int	\
					 ,[time] bigint		\
					 ,[summary] char[256])");
		m_db.execDML("commit transaction;");
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "create_db: " << e.errorCode() << ":" << e.errorMessage() << endl;
		return false;
	}
	return true;
}
bool sql_manger::fill_file_block(string path,unsigned int num,unsigned int offset)
{
	try
	{
		m_db.execDML("begin transaction;");
		CppSQLite3Buffer bufSQL;
		for(unsigned int i=offset;i<num+offset;i++)
		{
			bufSQL.format("insert into video_file(file_name,start_time,end_time,file_status,video_id,track_id,serial_num,file_sdp,file_length)values('%s/%06d.dat',0,0,0,0,0,0,'',0)",path.c_str(),i);
			m_db.execDML(bufSQL);
		}
		m_db.execDML("commit transaction;");
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "fill_file_block: " << e.errorCode() << ":" << e.errorMessage() << endl;
		return false;
	}
	return true;
}
bool sql_manger::recycle_video_file_sql(unsigned int num)
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁1"<<endl;
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("update video_file  set file_status=0 where in (select * from video_file order by start_time desc limit %d)",num);
		m_db.execDML(bufSQL);

	}
	catch (CppSQLite3Exception& e)
	{
		cout << "recycle_video_file_sql: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁1"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return false;
	}
	//cout<<"出锁1"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return true;
}
bool sql_manger::recycle_video_track_sql(time_t t)
{
		
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁2"<<endl;
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("delete from video_track where start_time<%d",t);
		m_db.execDML(bufSQL);
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "recycel_video_track_sql: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁2"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return false;
	}
	//cout<<"出锁2"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return true;
}
bool sql_manger::repair_video_file_sql(time_t t,int file_length)
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁3"<<endl;
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("update video_file  set end_time=%d ,file_length = %d where file_status=3",t,file_length);
		m_db.execDML(bufSQL);
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "repair_video_file_sql: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁3"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return false;
	}
	//cout<<"出锁3"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return true;
}
bool sql_manger::repair_video_track_sql(time_t t)
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁4"<<endl;
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("update video_track  set end_time=%d where track_status=0",t);
		m_db.execDML(bufSQL);
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "repair_video_track_sql: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁4"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return false;
	}
	//cout<<"出锁4"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return true;
}

int sql_manger::start_video_track_sql(unsigned int video_id,time_t st,const char *sdp)
{
	int track_id = -1;
	
	cout<<"进锁5"<<endl;
	pthread_rwlock_wrlock(&m_db_lock);
	cout<<"已经进锁5"<<endl;
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("insert into video_track(video_id,start_time,end_time,track_status)values(%d,%d,9999999999,0)",video_id,st);
		//bufSQL.format("insert into video_track(video_id,start_time,end_time,last_update_time,track_status)values(%d,%d,%d,9999999999,0)",video_id,st,st);
		m_db.execDML(bufSQL);

		bufSQL.format("select track_id from  video_track where start_time=%d and video_id=%d",st,video_id);
		CppSQLite3Query query = m_db.execQuery(bufSQL);
		
		if(query.eof()==false)
		{
			track_id = query.getIntField(0);
		}
		query.finalize();
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "start_video_track_sql: " << e.errorCode() << ":" << e.errorMessage() << endl;
		cout<<"出锁5"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return -1;
	}
	cout<<"出锁5"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return track_id;
}
bool sql_manger::start_video_file_sql(unsigned int file_id,time_t st,unsigned int track_id,unsigned int video_id,unsigned int serial)
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁6"<<endl;
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("update video_file set start_time=%d,track_id=%d,video_id=%d,serial_num=%d,file_status=%d,file_length=0 where file_id=%d",st,track_id,video_id,serial,FILE_ST_ING,file_id);
		m_db.execDML(bufSQL);
		//bufSQL.format("update video_track set last_update_time=%d,where track_id=%d",st,track_id);
		//m_db.execDML(bufSQL);
		cout<<"开始录像文件,更新文件状态"<<(const char *)bufSQL<<endl;
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "start_video_file_sql: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁6"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return false;
	}
	//cout<<"出锁6"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return true;
}
bool sql_manger::stop_video_file_sql(unsigned int file_id,time_t et,int file_length)
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁7"<<endl;
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("update video_file set end_time=%d,file_status=%d,file_length=%d where file_id=%d",et,FILE_ST_VIDEO,file_length,file_id);
		m_db.execDML(bufSQL);
		
		cout<<"结束录像文件,更新文件状态"<<(const char *)bufSQL<<endl;
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "stop_video_file_sql: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁7"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return false;
	}
	//cout<<"出锁7"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return true;
}
bool sql_manger::stop_video_track_sql(unsigned int track_id,time_t et)
{
	
	//cout<<"进锁8"<<endl;
	pthread_rwlock_wrlock(&m_db_lock);
	
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("update video_track set end_time=%d,track_status=%d where track_id=%d and track_status<>%d",et,TRACK_ST_ED,track_id,TRACK_ST_ED);
		m_db.execDML(bufSQL);
		cout<<"结束录像段,更新录像段状态"<<(const char *)bufSQL<<endl;
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "stop_video_file_sql: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁8"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return false;
	}
	//cout<<"出锁8"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return true;
}

bool sql_manger::get_track_video_list_sql(unsigned int video_id,file_list &fl,time_t st)
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁9"<<endl;
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("select * from video_track where start_time<=%d and end_time>%d and video_id=%d",st,st,video_id);
		
		CppSQLite3Query query = m_db.execQuery(bufSQL);
		int track_id = -1;
		if(query.eof()==false)
		{
			track_id = query.getIntField("track_id");
			//cout<<"查找到录像段 id="<<track_id<<endl;
			CppSQLite3Buffer bufSQL2;
			bufSQL2.format("select * from video_file where end_time>%d and track_id=%d and video_id=%d order by serial_num",st,track_id,video_id);
			//cout<<(const char *)bufSQL2<<endl;
			CppSQLite3Query qr = m_db.execQuery(bufSQL2);
			while(qr.eof()==false)
			{
				tag_file_block		tfb;
				tfb.file_id = qr.getIntField("file_id");
				tfb.file_lenth = qr.getIntField("file_length");
				tfb.file_path = qr.getStringField("file_name");
				tfb.file_status = qr.getIntField("file_status");
				//cout<<"file :"<<tfb.file_path<<"  "<<tfb.file_lenth<<"  "<<tfb.file_id<<endl;
				fl.push_back(tfb);
				qr.nextRow();
			}
			qr.finalize();
		}
		query.finalize();
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "get_track_video_list_sql: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁9"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return false;
	}
	//cout<<"出锁9"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return true;
}
bool sql_manger::get_track_video_list_sql(unsigned int video_id,file_list &fl,time_t st,time_t et)
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁10"<<endl;
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("select * from video_track where video_id=%d and start_time<%d and end_time>%d order by start_time",video_id,et,st);
		//bufSQL.format("select * from video_track where video_id=%d and ((start_time<%d and end_time>%d) or (start_time>%d and end_time<%d) or (start_time<%d and end_time>%d) or (start_time<%d and end_time>%d)) order by start_time",video_id,et,et,st,et,st,st,st,et);
		cout<<(const char *)bufSQL<<endl;
		CppSQLite3Query query = m_db.execQuery(bufSQL);
		int track_id = -1;
		while(query.eof()==false)
		{
			track_id = query.getIntField(0);
			CppSQLite3Buffer bufSQL2;
			bufSQL2.format("select * from video_file where start_time<%d and end_time>%d and track_id=%d and video_id=%d  order by serial_num",et,st,track_id,video_id);
			cout<<(const char *)bufSQL2<<endl;
			CppSQLite3Query qr = m_db.execQuery(bufSQL2);
			while(qr.eof()==false)
			{
				tag_file_block		tfb;
				tfb.file_id = qr.getIntField("file_id");
				tfb.file_lenth = qr.getIntField("file_length");
				tfb.file_path = qr.getStringField("file_name");
				//cout<<"加入录像播放列表"<<tfb.file_path<<endl;
				tfb.file_status = qr.getIntField("file_status");
				tfb.file_st = qr.getIntField("start_time");
				tfb.file_et = qr.getIntField("end_time");
				fl.push_back(tfb);
				qr.nextRow();
			}
			qr.finalize();
			query.nextRow();
		}
		query.finalize();
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "get_track_video_list_sql2 --: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁10"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return false;
	}
	//cout<<"出锁10"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return true;
}
bool sql_manger::get_device_list_sql(device_list &dl)
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁11"<<endl;
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("select * from device");
		CppSQLite3Query query = m_db.execQuery(bufSQL);
		while(query.eof()==false)
		{
			tag_device		td;
			td.device_id = query.getIntField("device_id");
			td.name = query.getStringField("device_name");
			td.ip = query.getStringField("ip");
			td.port = query.getIntField("port");
			td.status = query.getIntField("status");
			td.user=query.getStringField("user");
			td.pass=query.getStringField("pass");
			td.video_plan=query.getStringField("video_plan");
			td.type = query.getIntField("type");
			td.channel = query.getStringField("channel");
			td.sip_id = query.getStringField("device_sip_id");
			dl.push_back(td);
			query.nextRow();
		}
		query.finalize();
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "get_device_list_sql --: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁11"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return false;
	}
	//cout<<"出锁11"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return true;
}
bool sql_manger::get_track_list_sql(unsigned int video_id,track_list &tl,time_t st,time_t et)
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁12"<<endl;
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("delete from video_track where start_time=end_time");
		m_db.execDML(bufSQL);
		//bufSQL.format("select * from video_track where video_id=%d and ((start_time<%d and end_time>%d) or (start_time>%d and end_time<%d) or (start_time<%d and end_time>%d) or (start_time<%d and end_time>%d)) order by start_time",video_id,et,et,st,et,st,st,st,et);
		bufSQL.format("select * from video_track where video_id=%d and start_time<%d and end_time>%d order by start_time",video_id,et,st);

		CppSQLite3Query query = m_db.execQuery(bufSQL);
		cout<<"录像查询语句"<<(const char *)bufSQL<<endl;
		while(query.eof()==false)
		{
			tag_video_track	vt;
			vt.track_id = query.getIntField("track_id");
			vt.st = query.getInt64Field("start_time");
			vt.et = query.getInt64Field("end_time");
			vt.status = query.getIntField("track_status");
			tl.push_back(vt);
			query.nextRow();
		}
		query.finalize();

	}
	catch (CppSQLite3Exception& e)
	{
		cout << "get_track_list_sql --: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁12"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return false;
	}
	//cout<<"出锁12"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return true;
}
int sql_manger::add_device_sql(unsigned int device_type,string device_name,string ip,unsigned int port,string username,string password,string channel,string sip_id)
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁13"<<endl;
	int device_id = -1;
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("select * from  device where device_sip_id='%s'",sip_id.c_str());
		CppSQLite3Query query2 = m_db.execQuery(bufSQL);
		if(query2.eof()==false)
		{
			//cout<<"出锁13"<<endl;
			pthread_rwlock_unlock(&m_db_lock);
			cout<<"设备编号重复"<<endl;
			return -1;
		}
		time_t	t = time(NULL)+TIME_OFFSET;
		bufSQL.format("insert into device(type,device_name,ip,port,channel,user,pass,video_plan,max_day,status,time,device_sip_id)values(%d,'%s','%s',%d,'%s','%s','%s','000167',60,1,%d,'%s')",device_type,device_name.c_str(),ip.c_str(),port,channel.c_str(),username.c_str(),password.c_str(),t,sip_id.c_str());
		m_db.execDML(bufSQL);
		sleep(1);
		bufSQL.format("select device_id from  device where device_sip_id='%s'",sip_id.c_str());
		CppSQLite3Query query = m_db.execQuery(bufSQL);
		cout<<(const char *)bufSQL<<endl;
		if(query.eof()==false)
		{
			device_id = query.getIntField(0);
		}
		else
		{
			cout<<"数据操作失败"<<endl;
		}
		query.finalize();
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "add_device_sql --: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁13"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return -1;
	}
	//cout<<"出锁13"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return device_id;
}
bool sql_manger::update_device_sql(unsigned int device_id,unsigned int device_type,string device_name,string ip,unsigned int port,string username,string password,string channel)
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁14"<<endl;
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("update device set device_name=%s,type=%d, ip=%s,port=%d,user=%s,pass=%s,channel=%s where device_id=%d",device_name.c_str(),device_type,ip.c_str(),port,username.c_str(),password.c_str(),channel.c_str(),device_id);
		m_db.execDML(bufSQL);	
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "add_device_sql --: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁14"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return false;
	}
	//cout<<"出锁14"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return true;
}
bool sql_manger::update_device_plan_sql(unsigned int device_id,string plan,unsigned int maxday)
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁15"<<endl;
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("update device set video_plan='%s',max_day=%d where device_id=%d",plan.c_str(),maxday,device_id);
		m_db.execDML(bufSQL);	
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "add_device_sql --: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁15"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return false;
	}
	//cout<<"出锁15"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return true;
}
bool sql_manger::start_device_sql(unsigned int device_id)
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁16"<<endl;
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("update device set status=1 where device_id=%d",device_id);
		m_db.execDML(bufSQL);	
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "start_device_sql --: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁16"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return false;
	}
	//cout<<"出锁16"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return true;
}
bool sql_manger::start_record_device_sql(unsigned int device_id)
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁17"<<endl;
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("update device set status=2 where device_id=%d",device_id);
		m_db.execDML(bufSQL);	
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "start_record_device_sql --: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁17"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return false;
	}
	//cout<<"出锁17"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return true;
}
bool sql_manger::stop_record_device_sql(unsigned int device_id)
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁18"<<endl;
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("update device set status=1 where device_id=%d",device_id);
		m_db.execDML(bufSQL);	
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "stop_record_device_sql --: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁18"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return false;
	}
	//cout<<"出锁18"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return true;
}
bool sql_manger::stop_device_sql(unsigned int device_id)
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁19"<<endl;
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("update device set status=0 where device_id=%d",device_id);
		m_db.execDML(bufSQL);	
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "stop_device_sql --: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁19"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return false;
	}
	//cout<<"出锁19"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return true;
}
bool sql_manger::delete_device_no_record_sql(unsigned int device_id)
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁20"<<endl;
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("update device set status=3 where device_id=%d",device_id);
		m_db.execDML(bufSQL);	
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "stop_device_sql --: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁20"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return false;
	}
	//cout<<"出锁20"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return true;
}
bool sql_manger::delete_device_sql(unsigned int device_id)
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁21"<<endl;
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("delete from device  where device_id=%d",device_id);
		m_db.execDML(bufSQL);	
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "delete_device_sql --: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁21"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return false;
	}
	//cout<<"出锁21"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return true;
}
bool sql_manger::add_etag_sql(unsigned int device_id,time_t t,string summary)
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁22"<<endl;
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("insert into etag(video_id,time,summary)values(%d,%d,%s)",device_id,t,summary.c_str());
		m_db.execDML(bufSQL);	
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "add_etag_sql --: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁22"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return false;
	}
	//cout<<"出锁22"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return true;
}
int sql_manger::get_free_block(unsigned int num,file_list &fl)
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁23"<<endl;
	int row = 0;
	try
	{
		CppSQLite3Buffer bufSQL;
		CppSQLite3Buffer bufSQL2;
		bufSQL.format("select * from video_file where file_status=%d order by start_time limit %d",FILE_ST_NEW,num);
		CppSQLite3Query query = m_db.execQuery(bufSQL);
		m_db.execDML("begin transaction;");
		while(query.eof()==false)
		{
			tag_file_block		tfb;
			tfb.file_id = query.getIntField("file_id");
			tfb.file_lenth = query.getIntField("file_length");
			tfb.file_path = query.getStringField("file_name");
			tfb.file_status = query.getIntField("file_status");
			fl.push_back(tfb);
			bufSQL2.format("update video_file set file_status=%d where file_id=%d",FILE_ST_PRE,query.getIntField("file_id"));
			m_db.execDML(bufSQL2);
			query.nextRow();
			row++;
		}
		m_db.execDML("commit transaction;");
		query.finalize();
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "get_free_block --: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁23"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return -1;
	}
	//cout<<"出锁23"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return row;
}
int sql_manger::recycle_file_block(unsigned int num)
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁24"<<endl;
	int row = 0;
	try
	{
		CppSQLite3Buffer bufSQL;
		bufSQL.format("select * from video_file where file_status=1 order by start_time limit %d",num);
		CppSQLite3Query query = m_db.execQuery(bufSQL);

		m_db.execDML("begin transaction;");
		CppSQLite3Buffer bufSQL2;
		while(query.eof()==false)
		{
			bufSQL2.format("update video_file set file_status=0 where file_id=%d",query.getIntField("file_id"));
			m_db.execDML(bufSQL2);
			bufSQL2.format("update video_track set start_time=%d where track_id=%d",query.getInt64Field("end_time"),query.getIntField("track_id"));
			m_db.execDML(bufSQL2);
			query.nextRow();
			row++;
		}
		bufSQL2.format("delete from video_track where start_time=end_time");
		m_db.execDML("commit transaction;");
		query.finalize();
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "recycle_file_block --: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁24"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return -1;
	}
	//cout<<"出锁24"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return row;
}
int sql_manger::get_file_block_num()
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁25"<<endl;
	int row = 0;
	try
	{
		CppSQLite3Query query = m_db.execQuery("select count(*) from video_file");
		if (query.eof()==false)
		{
			row = query.getIntField(0);
		}
		query.finalize();
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "get_file_block_num --: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁25"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return -1;
	}
	//cout<<"出锁25"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return row;

}
bool sql_manger::add_video_etag(unsigned int video_id,time_t t,string summary)
{
	
	pthread_rwlock_wrlock(&m_db_lock);
	//cout<<"进锁26"<<endl;
	int row = 0;
	try
	{
		CppSQLite3Query query = m_db.execQuery("select count(*) from video_file");
		if (query.eof()==false)
		{
			row = query.getIntField(0);
		}
		query.finalize();
	}
	catch (CppSQLite3Exception& e)
	{
		cout << "get_file_block_num --: " << e.errorCode() << ":" << e.errorMessage() << endl;
		//cout<<"出锁26"<<endl;
		pthread_rwlock_unlock(&m_db_lock);
		return -1;
	}
	//cout<<"出锁26"<<endl;
	pthread_rwlock_unlock(&m_db_lock);
	return row;
}