#ifndef _STD_PRJ
#define _STD_PRJ
//c
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <dlfcn.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/utsname.h>
#include <sys/vfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <signal.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <semaphore.h>
#include <errno.h>
#include <locale.h>
#include <libintl.h>//翻译
#ifdef    _64_
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#define __USE_FILE_OFFSET64

#endif
#include <mntent.h>
#include <sys/statfs.h>

//c++
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <exception>
#include <zmq.h>
using namespace std;
#include <msgpack.hpp>
using namespace msgpack;
#include <json/json.h>

#include <eXosip2/eXosip.h>

#define current_version			"2.0.2"

#define _MULTI_THREADED 
#define G						1073741824
#define M						1048576
#define K						1024

#define MAX_TYPE				100
#define	MAX_CHANNEL				200 

#define CACHE_SIZE				524288//512*1024
#define	BLOCK_CACHE_SIZE		204800
#define IDX_SIZE				200
#define FILE_BLOCK_SIZE			104857600//536870912
#define	MAX_FILE_INDEX			2000


#define DEFAULT_HTTPPORT				80
#define DEFAULT_RTSPPORT				554
#define DEFAULT_SIPPORT					5060


#define	TIME_OFFSET						0//28800
#define SYNC_INTERVAL					60

#define	MAX_TRANSFER_REAL				64
#define MAX_DAY_SECOND					5184000 //60 days	

#define	LIVE_TIME	1800

#define CONFIG_FILE				"/etc/GVR/nvr.conf"
#define SHOT_FILE_DIR			"/etc/GVR/html/images/"
//IPC ACTION
#define TIME_SYNC				0x0001

#define PTZ_ACTION				0x0C00
#define PTZ_ACTION_STOP			PTZ_ACTION+1
#define PTZ_ACTION_ZOOMIN		PTZ_ACTION+2
#define PTZ_ACTION_ZOOMOUT		PTZ_ACTION+3
#define PTZ_ACTION_UP			PTZ_ACTION+4
#define PTZ_ACTION_DOWN			PTZ_ACTION+5
#define PTZ_ACTION_LEFT			PTZ_ACTION+6
#define PTZ_ACTION_RIGHT		PTZ_ACTION+7



enum RecvType{
	TYPE_TCP = 0,
	TYPE_UDP ,
};


enum FileST{
	FILE_ST_NEW = 0,//未使用，或已回收
	FILE_ST_VIDEO,//正常普通录像 ,
	FILE_ST_IVIDEO,//重要录像,不能被回收
	FILE_ST_ING,//正在使用
	FILE_ST_PRE,//预分配
};

enum IPCST{
	IPC_ST_NEW = 0,//未启用，或停用
	IPC_ST_ING,	//正在接受数据
	IPC_ST_RCING, //正在录像
	IPC_ST_DEL,
	IPC_ST_LOST,//设备丢失，收不到数据
};
enum TRACKST{
	TRACK_ST_ING = 0,//正在录像
	TRACK_ST_ED,//录像段已经完成
	TRACK_ST_LOCK,//锁定录像段
};

struct tag_file_block
{
	unsigned int file_id;
	string		 file_path;
	unsigned int file_lenth;
	unsigned int file_status;
	time_t		 file_st;
	time_t		 file_et;
};
typedef map<time_t,int>						offset_map;
typedef list<tag_file_block>				file_list;
typedef map<unsigned int,tag_file_block>	file_map;

struct tag_device
{
	unsigned int	device_id;
	string			name;
	unsigned int	type;
	string			ip;
	unsigned short	port;
	string			user;
	string			pass;
	string			video_plan;
	unsigned int	status;
	string			channel;
	string			sip_id;
};
typedef list<tag_device>					device_list;
typedef map<unsigned int ,tag_device>		device_map;

struct tag_video_track
{
	unsigned int track_id;
	time_t		 st;
	time_t		 et;
	unsigned int status;
};
typedef list<tag_video_track>					track_list;
typedef map<unsigned int ,tag_video_track>		track_map;


struct video_plan
{
	unsigned int sthour;
	unsigned int ethour;
};
typedef list<video_plan>		planlist;

enum CodecID {
	CODEC_ID_NONE=0,

	/* video codecs */
	CODEC_ID_MPEG1VIDEO,
	CODEC_ID_MPEG2VIDEO, ///< preferred ID for MPEG-1/2 video decoding
	CODEC_ID_MPEG2VIDEO_XVMC,
	CODEC_ID_H261,
	CODEC_ID_H263,
	CODEC_ID_RV10,
	CODEC_ID_RV20,
	CODEC_ID_MJPEG,
	CODEC_ID_MJPEGB,
	CODEC_ID_LJPEG,
	CODEC_ID_SP5X,
	CODEC_ID_JPEGLS,
	CODEC_ID_MPEG4,
	CODEC_ID_RAWVIDEO,
	CODEC_ID_MSMPEG4V1,
	CODEC_ID_MSMPEG4V2,
	CODEC_ID_MSMPEG4V3,
	CODEC_ID_WMV1,
	CODEC_ID_WMV2,
	CODEC_ID_H263P,
	CODEC_ID_H263I,
	CODEC_ID_FLV1,
	CODEC_ID_SVQ1,
	CODEC_ID_SVQ3,
	CODEC_ID_DVVIDEO,
	CODEC_ID_HUFFYUV,
	CODEC_ID_CYUV,
	CODEC_ID_H264,
	CODEC_ID_INDEO3,
	CODEC_ID_VP3,
	CODEC_ID_THEORA,
	CODEC_ID_ASV1,
	CODEC_ID_ASV2,
	CODEC_ID_FFV1,
	CODEC_ID_4XM,
	CODEC_ID_VCR1,
	CODEC_ID_CLJR,
	CODEC_ID_MDEC,
	CODEC_ID_ROQ,
	CODEC_ID_INTERPLAY_VIDEO,
	CODEC_ID_XAN_WC3,
	CODEC_ID_XAN_WC4,
	CODEC_ID_RPZA,
	CODEC_ID_CINEPAK,
	CODEC_ID_WS_VQA,
	CODEC_ID_MSRLE,
	CODEC_ID_MSVIDEO1,
	CODEC_ID_IDCIN,
	CODEC_ID_8BPS,
	CODEC_ID_SMC,
	CODEC_ID_FLIC,
	CODEC_ID_TRUEMOTION1,
	CODEC_ID_VMDVIDEO,
	CODEC_ID_MSZH,
	CODEC_ID_ZLIB,
	CODEC_ID_QTRLE,
	CODEC_ID_SNOW,
	CODEC_ID_TSCC,
	CODEC_ID_ULTI,
	CODEC_ID_QDRAW,
	CODEC_ID_VIXL,
	CODEC_ID_QPEG,
	CODEC_ID_PNG,
	CODEC_ID_PPM,
	CODEC_ID_PBM,
	CODEC_ID_PGM,
	CODEC_ID_PGMYUV,
	CODEC_ID_PAM,
	CODEC_ID_FFVHUFF,
	CODEC_ID_RV30,
	CODEC_ID_RV40,
	CODEC_ID_VC1,
	CODEC_ID_WMV3,
	CODEC_ID_LOCO,
	CODEC_ID_WNV1,
	CODEC_ID_AASC,
	CODEC_ID_INDEO2,
	CODEC_ID_FRAPS,
	CODEC_ID_TRUEMOTION2,
	CODEC_ID_BMP,
	CODEC_ID_CSCD,
	CODEC_ID_MMVIDEO,
	CODEC_ID_ZMBV,
	CODEC_ID_AVS,
	CODEC_ID_SMACKVIDEO,
	CODEC_ID_NUV,
	CODEC_ID_KMVC,
	CODEC_ID_FLASHSV,
	CODEC_ID_CAVS,
	CODEC_ID_JPEG2000,
	CODEC_ID_VMNC,
	CODEC_ID_VP5,
	CODEC_ID_VP6,
	CODEC_ID_VP6F,
	CODEC_ID_TARGA,
	CODEC_ID_DSICINVIDEO,
	CODEC_ID_TIERTEXSEQVIDEO,
	CODEC_ID_TIFF,
	CODEC_ID_GIF,
	CODEC_ID_FFH264,
	CODEC_ID_DXA,
	CODEC_ID_DNXHD,
	CODEC_ID_THP,
	CODEC_ID_SGI,
	CODEC_ID_C93,
	CODEC_ID_BETHSOFTVID,
	CODEC_ID_PTX,
	CODEC_ID_TXD,
	CODEC_ID_VP6A,
	CODEC_ID_AMV,
	CODEC_ID_VB,
	CODEC_ID_PCX,
	CODEC_ID_SUNRAST,
	CODEC_ID_INDEO4,
	CODEC_ID_INDEO5,
	CODEC_ID_MIMIC,
	CODEC_ID_RL2,
	CODEC_ID_8SVX_EXP,
	CODEC_ID_8SVX_FIB,
	CODEC_ID_ESCAPE124,
	CODEC_ID_DIRAC,
	CODEC_ID_BFI,
	CODEC_ID_CMV,
	CODEC_ID_MOTIONPIXELS,
	CODEC_ID_TGV,
	CODEC_ID_TGQ,
	CODEC_ID_TQI,
	CODEC_ID_AURA,
	CODEC_ID_AURA2,
	CODEC_ID_V210X,
	CODEC_ID_TMV,
	CODEC_ID_V210,
	CODEC_ID_DPX,
	CODEC_ID_MAD,
	CODEC_ID_FRWU,
	CODEC_ID_FLASHSV2,
	CODEC_ID_CDGRAPHICS,
	CODEC_ID_R210,
	CODEC_ID_ANM,
	CODEC_ID_BINKVIDEO,
	CODEC_ID_IFF_ILBM,
	CODEC_ID_IFF_BYTERUN1,
	CODEC_ID_KGV1,
	CODEC_ID_YOP,
	CODEC_ID_VP8,
	CODEC_ID_PICTOR,
	CODEC_ID_ANSI,
	CODEC_ID_A64_MULTI,
	CODEC_ID_A64_MULTI5,
	CODEC_ID_R10K,
	CODEC_ID_MXPEG,
	CODEC_ID_LAGARITH,
	CODEC_ID_PRORES,
	CODEC_ID_JV,
	CODEC_ID_DFA,
	CODEC_ID_8SVX_RAW,
	CODEC_ID_G2M,

	/* various PCM "codecs" */
	CODEC_ID_PCM_S16LE= 0x10000,
	CODEC_ID_PCM_S16BE,
	CODEC_ID_PCM_U16LE,
	CODEC_ID_PCM_U16BE,
	CODEC_ID_PCM_S8,
	CODEC_ID_PCM_U8,
	CODEC_ID_PCM_MULAW,
	CODEC_ID_PCM_ALAW,
	CODEC_ID_PCM_S32LE,
	CODEC_ID_PCM_S32BE,
	CODEC_ID_PCM_U32LE,
	CODEC_ID_PCM_U32BE,
	CODEC_ID_PCM_S24LE,
	CODEC_ID_PCM_S24BE,
	CODEC_ID_PCM_U24LE,
	CODEC_ID_PCM_U24BE,
	CODEC_ID_PCM_S24DAUD,
	CODEC_ID_PCM_ZORK,
	CODEC_ID_PCM_S16LE_PLANAR,
	CODEC_ID_PCM_DVD,
	CODEC_ID_PCM_F32BE,
	CODEC_ID_PCM_F32LE,
	CODEC_ID_PCM_F64BE,
	CODEC_ID_PCM_F64LE,
	CODEC_ID_PCM_BLURAY,
	CODEC_ID_PCM_LXF,
	CODEC_ID_S302M,

	/* various ADPCM codecs */
	CODEC_ID_ADPCM_IMA_QT= 0x11000,
	CODEC_ID_ADPCM_IMA_WAV,
	CODEC_ID_ADPCM_IMA_DK3,
	CODEC_ID_ADPCM_IMA_DK4,
	CODEC_ID_ADPCM_IMA_WS,
	CODEC_ID_ADPCM_IMA_SMJPEG,
	CODEC_ID_ADPCM_MS,
	CODEC_ID_ADPCM_4XM,
	CODEC_ID_ADPCM_XA,
	CODEC_ID_ADPCM_ADX,
	CODEC_ID_ADPCM_EA,
	CODEC_ID_ADPCM_G726,
	CODEC_ID_ADPCM_CT,
	CODEC_ID_ADPCM_SWF,
	CODEC_ID_ADPCM_YAMAHA,
	CODEC_ID_ADPCM_SBPRO_4,
	CODEC_ID_ADPCM_SBPRO_3,
	CODEC_ID_ADPCM_SBPRO_2,
	CODEC_ID_ADPCM_THP,
	CODEC_ID_ADPCM_IMA_AMV,
	CODEC_ID_ADPCM_EA_R1,
	CODEC_ID_ADPCM_EA_R3,
	CODEC_ID_ADPCM_EA_R2,
	CODEC_ID_ADPCM_IMA_EA_SEAD,
	CODEC_ID_ADPCM_IMA_EA_EACS,
	CODEC_ID_ADPCM_EA_XAS,
	CODEC_ID_ADPCM_EA_MAXIS_XA,
	CODEC_ID_ADPCM_IMA_ISS,
	CODEC_ID_ADPCM_G722,

	/* AMR */
	CODEC_ID_AMR_NB= 0x12000,
	CODEC_ID_AMR_WB,

	/* RealAudio codecs*/
	CODEC_ID_RA_144= 0x13000,
	CODEC_ID_RA_288,

	/* various DPCM codecs */
	CODEC_ID_ROQ_DPCM= 0x14000,
	CODEC_ID_INTERPLAY_DPCM,
	CODEC_ID_XAN_DPCM,
	CODEC_ID_SOL_DPCM,

	/* audio codecs */
	CODEC_ID_MP2= 0x15000,
	CODEC_ID_MP3, ///< preferred ID for decoding MPEG audio layer 1, 2 or 3
	CODEC_ID_AAC,
	CODEC_ID_AC3,
	CODEC_ID_DTS,
	CODEC_ID_VORBIS,
	CODEC_ID_DVAUDIO,
	CODEC_ID_WMAV1,
	CODEC_ID_WMAV2,
	CODEC_ID_MACE3,
	CODEC_ID_MACE6,
	CODEC_ID_VMDAUDIO,
	CODEC_ID_SONIC,
	CODEC_ID_SONIC_LS,
	CODEC_ID_FLAC,
	CODEC_ID_MP3ADU,
	CODEC_ID_MP3ON4,
	CODEC_ID_SHORTEN,
	CODEC_ID_ALAC,
	CODEC_ID_WESTWOOD_SND1,
	CODEC_ID_GSM, ///< as in Berlin toast format
	CODEC_ID_QDM2,
	CODEC_ID_COOK,
	CODEC_ID_TRUESPEECH,
	CODEC_ID_TTA,
	CODEC_ID_SMACKAUDIO,
	CODEC_ID_QCELP,
	CODEC_ID_WAVPACK,
	CODEC_ID_DSICINAUDIO,
	CODEC_ID_IMC,
	CODEC_ID_MUSEPACK7,
	CODEC_ID_MLP,
	CODEC_ID_GSM_MS, /* as found in WAV */
	CODEC_ID_ATRAC3,
	CODEC_ID_VOXWARE,
	CODEC_ID_APE,
	CODEC_ID_NELLYMOSER,
	CODEC_ID_MUSEPACK8,
	CODEC_ID_SPEEX,
	CODEC_ID_WMAVOICE,
	CODEC_ID_WMAPRO,
	CODEC_ID_WMALOSSLESS,
	CODEC_ID_ATRAC3P,
	CODEC_ID_EAC3,
	CODEC_ID_SIPR,
	CODEC_ID_MP1,
	CODEC_ID_TWINVQ,
	CODEC_ID_TRUEHD,
	CODEC_ID_MP4ALS,
	CODEC_ID_ATRAC1,
	CODEC_ID_BINKAUDIO_RDFT,
	CODEC_ID_BINKAUDIO_DCT,
	CODEC_ID_AAC_LATM,
	CODEC_ID_QDMC,
	CODEC_ID_CELT,

	/* subtitle codecs */
	CODEC_ID_DVD_SUBTITLE= 0x17000,
	CODEC_ID_DVB_SUBTITLE,
	CODEC_ID_TEXT,  ///< raw UTF-8 text
	CODEC_ID_XSUB,
	CODEC_ID_SSA,
	CODEC_ID_MOV_TEXT,
	CODEC_ID_HDMV_PGS_SUBTITLE,
	CODEC_ID_DVB_TELETEXT,
	CODEC_ID_SRT,
	CODEC_ID_MICRODVD,

	/* other specific kind of codecs (generally used for attachments) */
	CODEC_ID_TTF= 0x18000,

	CODEC_ID_PROBE= 0x19000, ///< codec_id is not known (like CODEC_ID_NONE) but lavf should attempt to identify it

	CODEC_ID_MPEG2TS= 0x20000, /**< _FAKE_ codec to indicate a raw MPEG-2 TS
							   * stream (only used by libavformat) */
							   CODEC_ID_FFMETADATA=0x21000,   ///< Dummy codec for streams containing only metadata information.
};


int get_free_port();
#endif


