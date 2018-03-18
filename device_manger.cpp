#include "device_manger.h"
device_manger::device_manger(void)
{
}

device_manger::~device_manger(void)
{
}
bool device_manger::device_login(string user,string pass,string ip,unsigned short port)
{
	bool blogin = false;
	m_device_sock = socket(AF_INET,SOCK_STREAM,0);
	inet_aton(ip.c_str(),&m_device_addr.sin_addr);  
	m_device_addr.sin_family = AF_INET;  
	m_device_addr.sin_port = htons(port);
	int	sock_buf_size = 512000;
	cout<<"登录"<<ip<<":"<<port<<endl;
	int error=-1, len;
	int ret  = setsockopt(m_device_sock, SOL_SOCKET, SO_RCVBUF,(char *)&sock_buf_size, sizeof(sock_buf_size));
	struct timeval tv_out;
	tv_out.tv_sec=2;
	tv_out.tv_usec=0;
	fd_set set;
	unsigned long ul = 1;
	/*
	ioctl(m_device_sock, FIONBIO, &ul); //设置为非阻塞模式
	if(connect(m_device_sock, (struct sockaddr *)&m_device_addr, sizeof(sockaddr))==-1)
	{
			FD_ZERO(&set);
			FD_SET(m_device_sock, &set);
			if( select(m_device_sock+1, NULL, &set, NULL, &tv_out) > 0)
			{
				getsockopt(m_device_sock, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
				if(error == 0)
				{
					cout<<"---1"<<endl;
					ret = -1;
				}
				else
				{
					cout<<"---2"<<endl;
					 ret = 0;
				}
			}
			else
			{
				cout<<"---3"<<endl;
				 ret = 0;
			}
	}
	else
	{
		cout<<"---4"<<endl;
		 ret = -1;
	}
	if(ret!=0)
	{
		cout<<"链接失败"<<endl;
	}
	ul = 0;
	ioctl(m_device_sock, FIONBIO, &ul); //设置为阻塞模式
	*/
	setsockopt(m_device_sock,SOL_SOCKET,SO_RCVTIMEO,&tv_out,sizeof(tv_out));
	ret = connect(m_device_sock,(sockaddr*)&m_device_addr,sizeof(sockaddr));
	char send_request[10240]={0};
	char recv_request[10240]={0};
	if(ret==0)
	{
		m_user = user;
		m_pass = pass;
		m_device_ipaddr = ip;
		m_device_port = port;
		sprintf(send_request,"POST / HTTP/1.1 \r\n\
Host: %s:%d\r\n\
User-Agent: gSOAP/2.7\r\n\
Content-Type: text/xml; charset=utf-8\r\n\
Content-Length: 614\r\n\
Connection: close\r\n\
SOAPAction:\"\"\r\n\r\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?> \
<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" \
xmlns:xop=\"http://www.w3.org/2004/08/xop/include\" xmlns:xmime4=\"http://www.w3.org/2004/11/xmlmime\" xmlns:ns=\"urn:calc\">\
<SOAP-ENV:Body><SDK-Login>\
<szUserLoginName>%s</szUserLoginName>\
<szPassword>%s</szPassword>\
<szIPAddress>http://%s:%d</szIPAddress></SDK-Login></SOAP-ENV:Body></SOAP-ENV:Envelope>",ip.c_str(),port,user.c_str(),pass.c_str(),ip.c_str(),port);
		int send_len = send(m_device_sock,send_request,strlen(send_request),0);
		/*
		while(send_len<0)
		{
			cout<<send_request<<endl<<send_len<<"  "<<errno<<endl;
			send_len = send(m_device_sock,send_request,strlen(send_request),0);
			sleep(2);
		}
		*/

		sleep(1);
		int recv_len = recv(m_device_sock,recv_request+recv_len,10240,0);
		string str = string(recv_request);
		//cout<<str<<endl;
		int pos1 = str.find("<szUserCode");
		int pos2 = str.find("</szUserIpAddress>");
		if(pos1!=string::npos&&pos2!=string::npos)
		{
			m_login_str = str.substr(pos1,pos2+18-pos1);
			blogin = true;
		}
		//cout<<m_login_str<<endl;
	}
	close(m_device_sock);
	return blogin;
}
bool device_manger::start_video_stream(string dest_ip,unsigned short dest_port,string req_channel)
{
	bool bstart = false;
	m_device_sock = socket(AF_INET,SOCK_STREAM,0);
	inet_aton(m_device_ipaddr.c_str(),&m_device_addr.sin_addr);  
	m_device_addr.sin_family = AF_INET;  
	m_device_addr.sin_port = htons(m_device_port);
	int	sock_buf_size = 512000;
	struct timeval tv_out;
	tv_out.tv_sec=2;
	tv_out.tv_usec=0;
	setsockopt(m_device_sock,SOL_SOCKET,SO_RCVTIMEO,&tv_out,sizeof(tv_out));
	int ret  = setsockopt(m_device_sock, SOL_SOCKET, SO_RCVBUF,(char *)&sock_buf_size, sizeof(sock_buf_size));
	ret = connect(m_device_sock,(sockaddr*)&m_device_addr,sizeof(sockaddr));  
	if(ret==0)
	{
		char send_request[10240]={0};
		char recv_request[10240]={0};
		string dest_ip_str;
		char	tempstr[32]={0};
		for(int i=0;i<64;i++)
		{
			if(i<dest_ip.length())
			{
				sprintf(tempstr,"%d",dest_ip.at(i));
				dest_ip_str+=("<item>"+string(tempstr)+"</item>");
			}
			else
				dest_ip_str+="<item>0</item>";
		}
		sprintf(send_request,"POST / HTTP/1.1 \r\n\
Host: %s:%d\r\n\
User-Agent: gSOAP/2.7\r\n\
Content-Type: text/xml; charset=utf-8\r\n\
Content-Length: 614\r\n\
Connection: close\r\n\
SOAPAction:\"\"\r\n\r\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?> \
<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" \
xmlns:xop=\"http://www.w3.org/2004/08/xop/include\" xmlns:xmime4=\"http://www.w3.org/2004/11/xmlmime\" xmlns:ns=\"urn:calc\">\
<SOAP-ENV:Body><SDK-StartVideoStream>\
<pstUserLogIDInfo>%s</pstUserLogIDInfo>\
<pstVideoStream> \
<ulChannelIndex>%s</ulChannelIndex> \
<ulStreamType>0</ulStreamType>\
<szDestIp xsi:type=\"SOAP-ENC:Array\" SOAP-ENC:arrayType=\"CHAR[64]\">%s</szDestIp> \
<usPort>%d</usPort><usReserved>0</usReserved></pstVideoStream></SDK-StartVideoStream></SOAP-ENV:Body></SOAP-ENV:Envelope> \
",m_device_ipaddr.c_str(),m_device_port,m_login_str.c_str(),req_channel.c_str(),dest_ip_str.c_str(),dest_port);
		int send_len = send(m_device_sock,send_request,strlen(send_request),0);
		int recv_len = recv(m_device_sock,recv_request,10240,0);
		//cout<<recv_request<<endl;
		bstart = true;
		
	}
	close(m_device_sock);
	return bstart;
}
bool device_manger::device_keepalive()
{
	bool bkeep = false;
	m_device_sock = socket(AF_INET,SOCK_STREAM,0);
	inet_aton(m_device_ipaddr.c_str(),&m_device_addr.sin_addr);  
	m_device_addr.sin_family = AF_INET;  
	m_device_addr.sin_port = htons(m_device_port);
	int	sock_buf_size = 512000;
	struct timeval tv_out;
	tv_out.tv_sec=2;
	tv_out.tv_usec=0;
	setsockopt(m_device_sock,SOL_SOCKET,SO_RCVTIMEO,&tv_out,sizeof(tv_out));
	int ret  = setsockopt(m_device_sock, SOL_SOCKET, SO_RCVBUF,(char *)&sock_buf_size, sizeof(sock_buf_size));
	ret = connect(m_device_sock,(sockaddr*)&m_device_addr,sizeof(sockaddr));  
	if(ret==0)
	{
		char send_request[10240]={0};
		char recv_request[10240]={0};
		sprintf(send_request,"POST / HTTP/1.1 \r\n\
Host: %s:%d\r\n\
User-Agent: gSOAP/2.7\r\n\
Content-Type: text/xml; charset=utf-8\r\n\
Content-Length: 614\r\n\
Connection: close\r\n\
SOAPAction:\"\"\r\n\r\n\
<?xml version=\"1.0\" encoding=\"UTF-8\"?> \
<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" \
xmlns:xop=\"http://www.w3.org/2004/08/xop/include\" xmlns:xmime4=\"http://www.w3.org/2004/11/xmlmime\" xmlns:ns=\"urn:calc\">\
<SOAP-ENV:Body><SDK-UserKeepAlive>\
<pstUserLogIDInfo>%s</pstUserLogIDInfo>\
</SDK-UserKeepAlive></SOAP-ENV:Body></SOAP-ENV:Envelope> \
",m_device_ipaddr.c_str(),m_device_port,m_login_str.c_str());
		int send_len = send(m_device_sock,send_request,strlen(send_request),0);
		int recv_len = recv(m_device_sock,recv_request,10240,0);
		bkeep = true;
	}
	close(m_device_sock);
	return bkeep;
}
bool device_manger::stop_video_stream()
{
	return true;
}
bool device_manger::device_logout()
{
	return true;
}
