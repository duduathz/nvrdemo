#ifndef UNITS_H
#define UNITS_H
#include <locale.h>
typedef struct DataOffset
{
	unsigned int start;
	unsigned int end;
	unsigned int unit_type;
	unsigned int slice_type;
	unsigned int offset;
}*DO;


struct sdp_session_level {
	int sdp_version;      /**< protocol version (currently 0) */
	int id;               /**< session ID */
	int version;          /**< session version */
	int start_time;       /**< session start time (NTP time, in seconds),
						  or 0 in case of permanent session */
	int end_time;         /**< session end time (NTP time, in seconds),
						  or 0 if the session is not bounded */
	int ttl;              /**< TTL, in case of multicast stream */
	const char *user;     /**< username of the session's creator */
	const char *src_addr; /**< IP address of the machine from which the session was created */
	const char *src_type; /**< address type of src_addr */
	const char *dst_addr; /**< destination IP address (can be multicast) */
	const char *dst_type; /**< destination IP address type */
	const char *name;     /**< session name (can be an empty string) */
};

size_t _strlcatf(char *dst, size_t size, const char *fmt, ...);
/*
void sdp_write_address(char *buff, int size, const char *dest_addr,
const char *dest_type, int ttl);
void sdp_write_header(char *buff, int size, struct sdp_session_level *s);
*/

int16_t ReadInt16(unsigned char * lp);
int32_t ReadInt24(unsigned char * lp);
int32_t ReadInt32(unsigned char * lp);
int64_t	ReadInt64(unsigned char * lp);
int64_t	ReReadInt64(unsigned char * lp);

char MakeFrameHeader(char Fi,char Fh);

unsigned int UnSignedExpGolomb(const unsigned char * lp,int index,bool bl = false);
unsigned int SearchStartCode(const unsigned char * lp,int limitsize,CodecID id,DataOffset * pdo);
bool IsH264IFrame(const unsigned char * lp,int limitsize);
//sony
int SearchFirstH264StartCode(const unsigned char * lp,int limitsize);
int SearchH264StartCode(const unsigned char * lp,int limitsize,DataOffset * pdo);

int SearchFirstPSStartCode(const unsigned char *lp, int limitsize);
int SearchFirstTSStartCode(const unsigned char *lp, int limitsize);
/*
can't using linux32
int get_time_from_txt(char* txt_time);
int get_time_from_ticks(char* ticks);
int get_time_from_filetime(char* filetime);
long long get_ticks_from_time(int tv_sec,int tv_usec);
void print_time_from_txt(char* txt_time_seconds);
void print_time_from_seconds(int i_time_seconds);
*/
int read_conf_value(const char *key,char *value,const char *file);
int write_conf_value(const char *key,char *value,const char *file);

int gbk2utf8(char *utfStr,const char *srcStr,int maxUtfStrlen);  


#endif




