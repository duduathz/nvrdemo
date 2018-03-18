#include"nvr_device_mgr.h"
nvr_device_mgr::nvr_device_mgr()
{

}
nvr_device_mgr::~nvr_device_mgr()
{

}
bool nvr_device_mgr::load_system_info()
{
	return updateinfo();
}
char *nvr_device_mgr::kscale(unsigned long b, unsigned long bs)
{
	unsigned long long size = b * (unsigned long long)bs;
    if (size > G)
    {
        sprintf(m_str, "%0.2f GB", size/(G*1.0));
        return m_str;
    }
    else if (size > M)
    {
        sprintf(m_str, "%0.2f MB", size/(1.0*M));
        return m_str;
    }
    else if (size > K)
    {
        sprintf(m_str, "%0.2f K", size/(1.0*K));
        return m_str;
    }
    else
    {
        sprintf(m_str, "%0.2f B", size*1.0);
        return m_str;
    }
}
bool nvr_device_mgr::updateinfo()
{
	char str[256];
	ifstream fs(path_sys);
	fs.getline(str,255,'(');
	str_system_kernel_version=string(str);
	fs.close();
	system("ulimit -n 65535");
	system("ifconfig eth0 |grep 'inet addr:'|awk '{print $2 }'|awk -F '[:]' '{print $2}' >/etc/nvr_tmp");
	system("route -n |grep 'U[ \t]' | head -n 1 | awk '{print $3}' >>/etc/nvr_tmp");
	system("route -n | grep 'UG[ \t]' | awk '{print $2}' >>/etc/nvr_tmp");
	system("ifconfig eth0|grep 'HWaddr'|awk '{print $5}' >>/etc/nvr_tmp");
	system("cat /etc/resolv.conf | grep nameserver | awk '{print $2}' >>/etc/nvr_tmp");
	fs.open("/etc/nvr_tmp");
	fs.getline(str,255,'\n');
	ip_addr = string(str);
	fs.getline(str,255,'\n');
	mask_addr = string(str);
	fs.getline(str,255,'\n');
	gateway_addr = string(str);
	fs.getline(str,255,'\n');
	mac_addr = string(str);
	fs.getline(str,255,'\n');
	dns_addr = string(str);
	fs.close();
	fs.open(path_cpu);
	while(!fs.eof())
	{
		fs.getline(str,255,'\n');
		if(strstr(str,"model name"))
		{
			str_cpu_type = string(strchr(str,':'));
			str_cpu_type = str_cpu_type.substr(1,str_cpu_type.length());
			break;
		}
	}
	fs.close();
	int sock_get_ip;  
	char ipaddr[50];  

	struct   sockaddr_in *sin;  
	struct   ifreq ifr_ip;     

	if ((sock_get_ip=socket(AF_INET, SOCK_STREAM, 0)) == -1)  
	{  
		printf("socket create failse...GetLocalIp!/n");  
		return "";  
	}  

	memset(&ifr_ip, 0, sizeof(ifr_ip));     
	strncpy(ifr_ip.ifr_name, "eth0", sizeof(ifr_ip.ifr_name) - 1);     

	if( ioctl( sock_get_ip, SIOCGIFADDR, &ifr_ip) < 0 )     
	{     
		return "";     
	}       
	sin = (struct sockaddr_in *)&ifr_ip.ifr_addr;     
	strcpy(ipaddr,inet_ntoa(sin->sin_addr));         

	ip_addr = string(ipaddr);      
	close(sock_get_ip);
	return update_disk_info();
}
bool nvr_device_mgr::update_disk_info()
{
	FILE* mount_table;
    struct mntent *mount_entry;
    struct statfs s;
    unsigned long blocks_used;
    unsigned blocks_percent_used;
    const char *disp_units_hdr = NULL;
    mount_table = NULL;
    mount_table = setmntent("/etc/mtab", "r");
    if (!mount_table)
    {
        fprintf(stderr, "set mount entry error\n");
        return -1;
    }
	m_hdlist.clear();
    while (1) {
        const char *device;
        const char *mount_point;
        if (mount_table) 
		{
            mount_entry = getmntent(mount_table);
            if (!mount_entry) {
                endmntent(mount_table);
                break;
            }
        }
        else
            continue;
        device = mount_entry->mnt_fsname;
        mount_point = mount_entry->mnt_dir;
        //fprintf(stderr, "mount info: device=%s mountpoint=%s\n", device, mount_point);
        if (statfs(mount_point, &s) != 0)
        {
            fprintf(stderr, "statfs failed!\n");   
            continue;
        }
        if ((s.f_blocks > 0) || !mount_table )
        {
			/*
            blocks_used = s.f_blocks - s.f_bfree;
            blocks_percent_used = 0;
            if (blocks_used + s.f_bavail)
            {
                blocks_percent_used = (blocks_used * 100ULL
                        + (blocks_used + s.f_bavail)/2
                        ) / (blocks_used + s.f_bavail);
            }
            // GNU coreutils 6.10 skips certain mounts, try to be compatible.  
            if (strcmp(device, "rootfs") == 0)
                continue;
            if (printf("\n%-20s" + 1, device) > 20)
                    printf("\n%-20s", "");
            char s1[20];
            char s2[20];
            char s3[20];
            strcpy(s1, kscale(s.f_blocks, s.f_bsize));
            strcpy(s2, kscale(s.f_blocks - s.f_bfree, s.f_bsize));
            strcpy(s3, kscale(s.f_bavail, s.f_bsize));
            printf(" %9s %9s %9s %3u%% %s\n",
                    s1,
                    s2,
                    s3,
                    blocks_percent_used, mount_point);
			*/
			hd	hds;
			hds.device = string(device);
			hds.mount_point = string(mount_point);
			hds.size = s.f_blocks*s.f_bsize;
			hds.free = s.f_bavail*s.f_bsize;
			m_hdlist.push_back(hds);


        }

	}
	return true;
}