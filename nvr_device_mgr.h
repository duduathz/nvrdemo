#ifndef NVR_DEVICE_MGR
#define NVR_DEVICE_MGR

#include"stdprj.h"
static const char *	path_cpu = "/proc/cpuinfo"; //cpu
static const char *	path_mem = "/proc/meminfo"; //内存
static const char *	path_partitions = "/proc/partitions";  //硬盘分区
static const char *	path_sys = "/proc/version"; //操作系统版本
static const char * path_mnt = "/etc/mtab";

struct	hd
{
	string	device;
	string  mount_point;
	unsigned long	size;
	unsigned long	free;
};

typedef list<hd>		hdlist;

class nvr_device_mgr
{
public:
	nvr_device_mgr();
	~nvr_device_mgr();
private:
	char	m_str[20];
public:
	string 	str_cpu_type;
	string	str_system_kernel_version;
	long	lpartitions_size;
	long	lpartitions_used;
	long    lmem_size;
	long	lmem_used;
	float	fcpu_used;
	long	lnet_size;
	long    lnet_used;
	hdlist	m_hdlist;
	string	mac_addr;
	string	ip_addr;
	string	mask_addr;
	string  gateway_addr;
	string	dns_addr;
public:
	bool	load_system_info();
	char *kscale(unsigned long b, unsigned long bs);
	bool updateinfo();
	bool update_disk_info();
	
};
#endif


