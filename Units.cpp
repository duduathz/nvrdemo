#include"stdprj.h"
#include"Units.h"
int16_t ReadInt16(unsigned char * lp)
{		
	return lp[1]+(lp[0]<<8);
}
int32_t ReadInt24(unsigned char * lp)
{
	return lp[2]+(lp[1]<<8)+(lp[0]<<16);
}
int32_t ReadInt32(unsigned char * lp)
{		
	return lp[3]+(lp[2]<<8)+(lp[1]<<16)+(lp[0]<<24);	
}
int64_t	ReadInt64(unsigned char * lp)
{
	int64_t re = 0;
	for(int i=0;i<8;i++)
	{
		re+=lp[i];
		if(i<7)
			re = re<<8;
	}
	return re;
}
int64_t	ReReadInt64(unsigned char * lp)
{
	int64_t re = 0;
	for(int i=0;i<8;i++)
	{
		re+=lp[7-i];
		if(i<7)
			re = re<<8;
	}
	return re;
}
char MakeFrameHeader(char Fi,char Fh)
{
	char Sh = (Fi&0xE0)|(Fh&0x1F);
	return Sh;
}
bool IsH264IFrame(unsigned char * lp,int limitsize)
{
	if(lp[3]==1)
	{
		if((lp[4]&0x1F)==1||(lp[4]&0x1F)==5)
		{
			int16_t t=ReadInt16(lp+5);
			if(t>>10==38)
				return true;
			else if(t>>8==140)
				return true;
			else if(t>>8==143)
				return true;
			else if(t>>6==537)
				return true;
		}
	}
	else if(lp[2]==1)
	{
		if((lp[3]&0x1F)==1||(lp[3]&0x1F)==5)
		{
			int16_t t=ReadInt16(lp+4);
			if(t>>10==38)
				return true;
			else if(t>>8==140)
				return true;
			else if(t>>8==143)
				return true;
			else if(t>>6==537)
				return true;
		}
	}
	return false;
}
unsigned int UnSignedExpGolomb(const unsigned char * lp,int index,bool bl)
{
	register unsigned int tmp=0;
	register unsigned int num =0;
	register unsigned int offset = 0;
	register unsigned int toffset = 0;
	tmp = lp[0]<<24|lp[1]<<16|lp[2]<<8|lp[3];
	for(int m = 0;m<index;m++)
	{
		tmp = tmp<<offset;
		int i = 0;
		for(i=31;i>0;i--)
		{
			if(tmp>>i)
			{
				break;
			}
		}
		// 31-i 前缀0的个数 
		offset = 63-2*i;
		toffset+=offset;
		num = (tmp>>(32-offset))-1;
	}
	if(bl)
	{
		return toffset;
	}
	return num;
}
unsigned int SearchStartCode(const unsigned char * lp,int limitSize,CodecID id,DataOffset * pdo)
{

	int nCode = 0;
	unsigned int nPos = 0;
	bool nStartCode = false;
	if(limitSize<=0)
	{
		return 0;
	}
	if(id==CODEC_ID_H263)
	{
		while(nPos+4<limitSize)
		{
			nCode = ReadInt32((unsigned char *)lp+nPos);			
			//if(nCode==0x01B6)   //MPEG4
			if(nCode>>8>=0x80&&nCode>>8<=0x8F)	  //H263
			{
				pdo->start = nPos;
				pdo->unit_type = 0;
				pdo->slice_type = 0;
				pdo->offset = 0;
				nStartCode = true;
			}
			if(nCode>>8>=0x80&&nCode>>8<=0x8F&&nStartCode)
			{
				pdo->end = nPos;
				return nPos;
			}
			nPos+=1;
		}
	}
	else if(id==CODEC_ID_MPEG2VIDEO)
	{
		while(nPos+4<limitSize)
		{
			nCode = ReadInt24((unsigned char *)lp+nPos);
			if(nCode-0x01E0<0xF&&nCode-0x01E0>-1)					//MPEG2
			{
				pdo->start = nPos;
				pdo->unit_type = 0;
				pdo->slice_type = 0;
				pdo->offset = 0;
				nStartCode = true;
			}
			if(nCode-0x01E0<0xF&&nCode-0x01E0>-1&&nStartCode)					//MPEG2
			{
				pdo->end = nPos;
				return nPos;
			}
			nPos+=1;
		}
	}
	else if(id==CODEC_ID_MPEG4)
	{
		while(nPos+4<limitSize)
		{
			nCode = ReadInt32((unsigned char *)lp+nPos);
			if(nCode==0x01B6||nCode==0x0B0)   //MPEG4
			{
				pdo->start = nPos;
				pdo->unit_type = 0;
				pdo->slice_type = 0;
				pdo->offset = 0;
				nStartCode = true;
			}
			if((nCode==0x01B6||nCode==0x0B0)&&nStartCode)   //MPEG4
			{
				pdo->end = nPos;
				return nPos;
			}
			nPos+=1;
		}
	}
	else if(id==CODEC_ID_MJPEG)
	{

		while(nPos+4<limitSize)
		{
			nCode = ReadInt16((unsigned char *)lp+nPos);	
			if(nCode==0xFFD8)
			{
				pdo->start = nPos;
				pdo->unit_type = 0;
				pdo->slice_type = 0;
				pdo->offset = 0;
				nStartCode = true;

			}
			if(nCode==0xFFD9&&nStartCode)
			{
				pdo->end = nPos;
				return nPos;
			}
			nPos+=1;
		}
	}
	else if(id==CODEC_ID_H264)
	{
		while(nPos+10<limitSize)
		{
			nCode = ReadInt32((unsigned char *)lp+nPos);
			if(nCode==0x00000001&&nStartCode)   //H264
			{				
				pdo->end = nPos;
				return nPos;
			}
			if(nCode==0x00000001&&!nStartCode)   //H264
			{
				int first_mb_in_slice = 0;
				pdo->start = nPos;
				pdo->unit_type = lp[nPos+4]&0x1F;
				if(pdo->unit_type==1||pdo->unit_type==5)
				{
					first_mb_in_slice = UnSignedExpGolomb((unsigned char *)lp+nPos+5,1);
					if(first_mb_in_slice==0)//first_mb_in_slice=0时下面才能正确分析
						pdo->slice_type = UnSignedExpGolomb((unsigned char *)lp+nPos+5,2);
					else
						pdo->slice_type = 10;//slice的一部分
					pdo->offset = first_mb_in_slice;
				}
				else
				{
					pdo->slice_type = 11;
				}
				nStartCode = true;
				nPos+=3; 			//防止24位重读
			}
			else
			{
				if(lp[nPos+3]!=0&&lp[nPos+3]!=1)
				{
					nPos+=3;
				}
			}

			nCode = ReadInt24((unsigned char *)lp+nPos);
			if(nCode==0x000001&&nStartCode)   //H264
			{
				pdo->end = nPos;
				return nPos;
			}
			if(nCode==0x000001&&!nStartCode)   //H264
			{
				int first_mb_in_slice = 0;
				pdo->start = nPos;
				pdo->unit_type = lp[nPos+3]&0x1F;
				if(pdo->unit_type==1||pdo->unit_type==5)
				{
					first_mb_in_slice = UnSignedExpGolomb((unsigned char *)lp+nPos+4,1);
					if(first_mb_in_slice==0)//first_mb_in_slice=0时下面才能正确分析
						pdo->slice_type = UnSignedExpGolomb((unsigned char *)lp+nPos+4,2);
					else
						pdo->slice_type = 10;//slice的一部分
					pdo->offset = first_mb_in_slice;
				}
				else
				{
					pdo->slice_type = 11;
				}
				nStartCode = true;
				nPos+=2; 
			}
			else
			{
				if(lp[nPos+2]!=0)
				{
					nPos+=2;
				}
			}
			nPos+=1;
		}
		return 0;
	}
	else
	{
		return 0;
	}
	return 0;
}
int SearchFirstH264StartCode(const unsigned char * lp,int limitsize)
{
	int nCode = 0;
	unsigned int nPos = 0;
	while(nPos+10<limitsize)
	{
		nCode = ReadInt32((unsigned char *)lp+nPos);
		if(nCode==0x00000001)   //H264
		{				
			return nPos;
		}
		else
		{
			if(lp[nPos+3]>1)
			{
				nPos+=4;
				continue;
			}
			if(lp[nPos+2]>0)
			{
				nPos+=3;
				continue;
			}
			if(lp[nPos+1]>0)
			{
				nPos+=2;
				continue;
			}
			nPos++;
		}
	}
	return -1;
}
int SearchH264StartCode(const unsigned char * lp,int limitsize,DataOffset * pdo)
{
	int npos = 0;
	npos = SearchFirstH264StartCode(lp,limitsize);
	if(npos!=-1)
	{
		pdo->start = npos;
		pdo->unit_type = lp[npos+4]&0x1F;
		npos =  SearchFirstH264StartCode(lp+npos+4,limitsize-npos-4);
		if(npos>0)
		{
			pdo->end = npos;
			pdo->slice_type = 0;
			return pdo->start+npos+4;
		}
	}
	return -1;
}
int SearchFirstPSStartCode(const unsigned char *lp, int limitsize)
{
	int nCode = 0;
	unsigned int nPos = 0;
	while(nPos+10<limitsize)
	{
		nCode = ReadInt32((unsigned char *)lp+nPos);
		if(nCode==0x000001BA)   //PS Header //System Header 0x000001BB //PES Header 0x0000
		{				
			return nPos;
		}
		nPos++;
	}
	return -1;
}
int SearchFirstTSStartCode(const unsigned char *lp, int limitsize)
{
	int sync_byte = 0x47;
	int nPos =0;
	while(nPos+188<limitsize)
	{
		if(lp[nPos]==0x47)
		{
			if(lp[nPos+188]==0x47)
			{
				return nPos;
			}
		}
		nPos++;
	}
	return -1;
}
/*
int get_time_from_txt(char* txt_time){
struct tm t;
memset(&t, 0, sizeof(t));
strptime(txt_time, "%Y%m%d%H%M%S",&t);
return mktime(&t);
}

int get_time_from_ticks(char* ticks){
long long ll_ticks = atoll(ticks);
ll_ticks -= 621355968000000000;
ll_ticks = ll_ticks / 10000000;
ll_ticks -= 8*60*60;
return (int)ll_ticks;
}

int get_time_from_filetime(char* filetime){
long long ll_ticks = atoll(filetime);
ll_ticks -= 116444736000000000;
ll_ticks = ll_ticks / 10000000;
return (int)ll_ticks;
}

void print_time_from_txt(char* txt_time_seconds){
print_time_from_seconds(atoi(txt_time_seconds));
}

void print_time_from_seconds(int i_time_seconds){
struct tm *time_now;  
time_t secs_now = i_time_seconds;   
time_now = localtime(&secs_now);  
char str[80];
strftime(str, 80,"%Y-%m-%d %H:%M:%S",time_now);  
cout<<str<<endl;
}

long long get_ticks_from_time(int tv_sec,int tv_usec){
tv_sec += 8*60*60;
long long ll_ticks = ((long long)tv_sec) * 10000000;
ll_ticks += 621355968000000000;
return ll_ticks;
}
*/
typedef struct item_t {
	char *key;
	char *value;
}ITEM;

/*
*去除字符串右端空格
*/
char *strtrimr(char *pstr)
{
	int i;
	i = strlen(pstr) - 1;
	while (isspace(pstr[i]) && (i >= 0))
		pstr[i--] = '\0';
	return pstr;
}
/*
*去除字符串左端空格
*/
char *strtriml(char *pstr)
{
	int i = 0,j;
	j = strlen(pstr) - 1;
	while (isspace(pstr[i]) && (i <= j))
		i++;
	if (0<i)
		strcpy(pstr, &pstr[i]);
	return pstr;
}
/*
*去除字符串两端空格
*/
char *strtrim(char *pstr)
{
	char *p;
	p = strtrimr(pstr);
	return strtriml(p);
}


/*
*从配置文件的一行读出key或value,返回item指针
*line--从配置文件读出的一行
*/
int  get_item_from_line(char *line, ITEM *item)
{
	char *p = strtrim(line);
	int len = strlen(p);
	if(len <= 0){
		return 1;//空行
	}
	else if(p[0]=='#'){
		return 2;
	}else{
		char *p2 = strchr(p, '=');
		*p2++ = '\0';
		item->key = (char *)malloc(strlen(p) + 1);
		item->value = (char *)malloc(strlen(p2) + 1);
		strcpy(item->key,p);
		strcpy(item->value,p2);

	}
	return 0;//查询成功
}

int file_to_items(const char *file, ITEM *items, int *num)
{
	char line[1024];
	FILE *fp;
	fp = fopen(file,"r");
	if(fp == NULL)
		return 1;
	int i = 0;
	while(fgets(line, 1023, fp)){
		char *p = strtrim(line);
		int len = strlen(p);
		if(len <= 0){
			continue;
		}
		else if(p[0]=='#'){
			continue;
		}else{
			char *p2 = strchr(p, '=');
			/*这里认为只有key没什么意义*/
			if(p2 == NULL)
				continue;
			*p2++ = '\0';
			items[i].key = (char *)malloc(strlen(p) + 1);
			items[i].value = (char *)malloc(1024);
			strcpy(items[i].key,p);
			strcpy(items[i].value,p2);
			i++;
		}
		if(i==50)
		{
			break;
		}
	}
	(*num) = i;
	fclose(fp);
	return 0;
}

/*
*读取value
*/
int read_conf_value(const char *key,char *value,const char *file)
{
	char line[1024];
	FILE *fp;
	fp = fopen(file,"r");
	if(fp == NULL)
		return 1;//文件打开错误
	while (fgets(line, 1023, fp))
	{
		ITEM item;
		if(get_item_from_line(line,&item)==0)
		{
			if(!strcmp(item.key,key))
			{
				strcpy(value,item.value);
				fclose(fp);
				free(item.key);
				free(item.value);
				return 0;
			}
			free(item.key);
			free(item.value);
		}
	}
	return 2;//成功

}
int write_conf_value(const char *key,char *value,const char *file)
{
	bool bOK=true;
	ITEM items[50];// 假定配置项最多有50个
	int num;//存储从文件读取的有效数目
	file_to_items(file, items, &num);
	bool bfind = false;
	int i=0;
	//查找要修改的项
	for(i=0;i<num;i++){
		if(!strcmp(items[i].key, key))
		{
			memset(items[i].value,0,1024);
			sprintf(items[i].value,"%s",value);
			bfind = true;
			break;
		}
	}

	// 更新配置文件,应该有备份，下面的操作会将文件内容清除
	FILE *fp;
	fp = fopen(file, "w");
	if(fp != NULL)
	{
		if(!bfind)
		{
			fprintf(fp,"%s=%s\n",key,value);
		}
		for(i=0;i<num;i++){
			fprintf(fp,"%s=%s\n",items[i].key, items[i].value);
		}
		fclose(fp);
		bOK = false;
	}
	for(i=0;i<num;i++){
		free(items[i].key);
		free(items[i].value);
	}
	return bOK;

}
size_t _strlcatf(char *dst, size_t size, const char *fmt, ...)
{
	int len = strlen(dst);
	va_list vl;

	va_start(vl, fmt);
	len += vsnprintf(dst + len, size > len ? size - len : 0, fmt, vl);
	va_end(vl);

	return len;
}
/*
void sdp_write_address(char *buff, int size, const char *dest_addr,
const char *dest_type, int ttl)
{
if (dest_addr) {
if (!dest_type)
dest_type = "IP4";
if (ttl > 0 && !strcmp(dest_type, "IP4")) {
// The TTL should only be specified for IPv4 multicast addresses,
// not for IPv6. 
_strlcatf(buff,size,"c=IN %s %s/%d\r\n",, dest_type, dest_addr, ttl);
} else {
_strlcatf(buff,size,"c=IN %s %s\r\n", dest_type, dest_addr);
}
}
}
void sdp_write_header(char *buff, int size, struct sdp_session_level *s)
{
_strlcatf(buff, size, "v=%d\r\n"
"o=- %d %d IN %s %s\r\n"
"s=%s\r\n",
s->sdp_version,
s->id, s->version, s->src_type, s->src_addr,
s->name);
sdp_write_address(buff, size, s->dst_addr, s->dst_type, s->ttl);
_strlcatf(buff, size, "t=%d %d\r\n"
"a=tool:libavformat " AV_STRINGIFY(LIBAVFORMAT_VERSION) "\r\n",
s->start_time, s->end_time);
}
void sdp_write_media(char *buff, int size, AVCodecContext *c, const char *dest_addr, const char *dest_type, int port, int ttl, AVFormatContext *fmt)
{
const char *type;
int payload_type;

payload_type = ff_rtp_get_payload_type(c);
if (payload_type < 0) {
payload_type = RTP_PT_PRIVATE + (c->codec_type == AVMEDIA_TYPE_AUDIO);
}

switch (c->codec_type) {
case AVMEDIA_TYPE_VIDEO   : type = "video"      ; break;
case AVMEDIA_TYPE_AUDIO   : type = "audio"      ; break;
case AVMEDIA_TYPE_SUBTITLE: type = "text"       ; break;
default                 : type = "application"; break;
}

av_strlcatf(buff, size, "m=%s %d RTP/AVP %d\r\n", type, port, payload_type);
sdp_write_address(buff, size, dest_addr, dest_type, ttl);
if (c->bit_rate) {
av_strlcatf(buff, size, "b=AS:%d\r\n", c->bit_rate / 1000);
}

sdp_write_media_attributes(buff, size, c, payload_type, fmt);
}
static int sdp_get_address(char *dest_addr, int size, int *ttl, const char *url)
{
int port;
return port;
}
int sdp_create(char *buf, int size)
{
struct sdp_session_level s;
int i, j, port, ttl, is_multicast;
sdp_write_header(buf, size, &s);

return 0;
}
*/
int gbk2utf8(char *utfStr,const char *srcStr,int maxUtfStrlen)  
{  
	if(NULL==srcStr)  
	{  
		printf("Bad Parameter\n");  
		return -1;  
	}  

	//首先先将gbk编码转换为unicode编码  
	if(NULL==setlocale(LC_ALL,"zh_CN.gbk"))//设置转换为unicode前的码,当前为gbk编码  
	{  
		printf("Bad Parameter\n");  
		return -1;  
	}  

	int unicodeLen=mbstowcs(NULL,srcStr,0);//计算转换后的长度  
	if(unicodeLen<=0)  
	{  
		printf("Can not Transfer!!!\n");  
		return -1;  
	}  
	wchar_t *unicodeStr=(wchar_t *)calloc(sizeof(wchar_t),unicodeLen+1);  
	mbstowcs(unicodeStr,srcStr,strlen(srcStr));//将gbk转换为unicode  

	//将unicode编码转换为utf8编码  
	if(NULL==setlocale(LC_ALL,"zh_CN.utf8"))//设置unicode转换后的码,当前为utf8  
	{  
		printf("Bad Parameter\n");  
		return -1;  
	}  
	int utfLen=wcstombs(NULL,unicodeStr,0);//计算转换后的长度  
	if(utfLen<=0)  
	{  
		printf("Can not Transfer!!!\n");  
		return -1;  
	}  
	else if(utfLen>=maxUtfStrlen)//判断空间是否足够  
	{  
		printf("Dst Str memory not enough\n");  
		return -1;  
	}  
	wcstombs(utfStr,unicodeStr,utfLen);  
	utfStr[utfLen]=0;//添加结束符  
	free(unicodeStr);  
	return utfLen;  
} 
