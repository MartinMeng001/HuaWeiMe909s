/*
 * Me9094G.cpp
 *
 *  Created on: 2019年2月23日
 *      Author: dell
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include "esm6800_drivers.h"
#include "Me9094G.h"
#define PRODUCT_FOR_EC

#define ME909_VENDORID	"0x12d1"
#define ME909_PRODUCTID	"0x15c1"
#define EC20_VERDORID	"0x2c7c"
#define EC20_PRODUCTID	"0x0125"
#define USBSERIAL_CMD	"usbserial"
#define PPP0_CMD	"ppp0"
#define INSMOD_CMD	"insmod /lib/modules/4.1.15-g4f1387e/kernel/drivers/usb/serial/usbserial.ko vendor=%s product=%s"
#define MODPROBE_CMD	"modprobe usbserial vendor=%s product=%s"
#define LSMOD_CMD	"lsmod"
#define RMMOD_CMD	"rmmod %s"
#define DIAL4G_CMD	"pppd call lte-connect-script"
#define DIAL3G_CMD	"pppd call wcdma-connect-script"
#define ROUTE_CMD	"route add default gw %s"
#define ROUTE_CMD_FOR_MULTINETWORKS	"ip route add default via %s dev ppp0 metric 100"
#define ROUTE_CMD_FOR_SPECIALIPS	"route add -net %s netmask 255.255.255.0 gw %s"
#define INET_ADDR_CMD "ip addr show | grep \"inet\""
Me909_4G::Me909_4G() {
	// TODO Auto-generated constructor stub
	conn_status=0;
	alive_status=0;
	m_thread=0;
	memset(ipAddr,0,sizeof(ipAddr));
	memset(gwAddr, 0, sizeof(gwAddr));
	memset(aimIps, 0, sizeof(aimIps));
}

Me909_4G::~Me909_4G() {
	// TODO Auto-generated destructor stub
}
void Me909_4G::Connect4G()
{
	int res;
	pthread_attr_t attr;

	res=pthread_attr_init(&attr);
	if(res!=0) printf("Creat attribute for 4G failed\r\n");
	res=pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	res+=pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if(res!=0)
	{
		printf("setting attribute for 4G failed\r\n");
		return;
	}
	res=pthread_create(&m_thread, &attr, (void*(*)(void*))&Me909ConnectionFunc, (void*)this);
	if(res!=0) return;
	pthread_attr_destroy(&attr);
	return;
}
void Me909_4G::Connect3G()
{

}
int Me909_4G::getIP4Conn(char* ip, int maxlen)
{
	if(conn_status!=CONN_STATUS_SUCCESS) return 0;
	if(maxlen<(int)strlen(ipAddr)) return 0;
	strcpy(ip, ipAddr);
	return 1;
}
int Me909_4G::getGW4Conn(char* gw, int maxlen)
{
	if(conn_status!=CONN_STATUS_SUCCESS) return 0;
	if(maxlen<(int)strlen(gwAddr)) return 0;
	strcpy(gw, gwAddr);
	return 1;
}
int Me909_4G::setRouteIps(char* ips, int maxlen)
{
	if(maxlen>(int)sizeof(aimIps)) return 0;
	int length=strlen(ips);
	if(length>maxlen) return 0;
	memset(aimIps, 0, sizeof(aimIps));
	int breakcount=3;
	for(int i=0;i<length;i++)
	{
		aimIps[i]=ips[i];
		if(ips[i]=='.')breakcount--;
		if(breakcount==0)
		{
			aimIps[i+1]='0';
			break;
		}
	}
	return 1;
}
void Me909_4G::KeepAlive()
{
	//if(flow_DialChk(2)==0)
	{
		int res;
		pthread_attr_t attr;

		printf(" 4G keepAlive thread begin\r\n");
		res=pthread_attr_init(&attr);
		if(res!=0) printf("Creat attribute for 4G keepAlive failed\r\n");
		res=pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
		res+=pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		if(res!=0)
		{
			printf("setting attribute for 4G keepAlive failed\r\n");
			return;
		}
		res=pthread_create(&m_thread, &attr, (void*(*)(void*))&checkConnectionFunc, (void*)this);
		if(res!=0) return;
		pthread_attr_destroy(&attr);
		printf(" 4G keepAlive thread end\r\n");
	}
}
int Me909_4G::checkNetwork()
{
	if(flow_DialChk(2)==0) return 0;
	if(getIP()==1)
	{
		Route();
		return 1;
	}
	return 0;
}
int Me909_4G::checkConnectionFunc(void* lparam)
{
	Me909_4G* pme909=(Me909_4G*)lparam;
	if(pme909->flow_DialChk(2)==0)
	{
		pme909->alive_status=0;
		if(pme909->flow_DrvChk(2)==0)
		{
			pme909->LoadDrv();sleep(1);
			if(pme909->flow_DrvChk(3)==0) return 0;
		}
		pme909->Dial();
		if(pme909->flow_DialChk(20)==0) return 0;
		sleep(10);	// wait 10s to build the connection
		return pme909->checkNetwork();
	}
	return 1;
}
int Me909_4G::checkConnection()
{
	if(flow_DrvChk(2)==0)
	{
		LoadDrv();sleep(1);
		if(flow_DrvChk(3)==0) return 0;
	}
	Dial();
	if(flow_DialChk(20)==0) return 0;
	sleep(10);	// wait 10s to build the connection
	return checkNetwork();
}
int Me909_4G::checkModule()
{
	if(powerResetMe909()==0) return 0;
	sleep(10);
	return checkConnection();
}
void Me909_4G::Me909ConnectionFunc(void* lparam)
{
	Me909_4G* pme909=(Me909_4G*)lparam;
	pme909->conn_status=CONN_STATUS_FAILED;
	if(pme909->checkNetwork()==1)
	{
		pme909->conn_status=CONN_STATUS_SUCCESS;
		return;
	}
	printf("network check failed, check Connection\r\n");
	if(pme909->checkConnection()==1)
	{
		pme909->conn_status=CONN_STATUS_SUCCESS;
		return;
	}
	printf("Connection check failed, check Module\r\n");
	if(pme909->checkModule()==1)
	{
		pme909->conn_status=CONN_STATUS_SUCCESS;
		return;
	}
	pme909->conn_status=CONN_STATUS_FAILED;
	printf("Module check failed, Quit\r\n");
}
int Me909_4G::flow_Failed()
{
	conn_status=CONN_STATUS_FAILED;
	return 0;
}
int Me909_4G::flow_DrvChk(int retrynum)
{
	int ret=0;
	while(retrynum--)
	{
		if(ChkDrv()==1)
		{
			ret=1;
			break;
		}
		sleep(1);
	}
	return ret;
}
int Me909_4G::flow_DialChk(int retrynum)
{
	int ret=0;
	while(retrynum--)
	{
		if(ChkPPP0pid()==1)
		{
			printf("ppp0.pid finded\r\n");
			ret=1;
			break;
		}
		sleep(1);
	}
	return ret;
}
int Me909_4G::powerResetMe909()
{
	int fd=OpenIO();
	if(fd==0) return 0;
	GPIO_OutEnable(fd, GPIO6 );
	//mdelay(200);
	GPIO_OutClear(fd, GPIO6);		// GPRS_STD = 0
	mdelay(7000);
	GPIO_OutSet(fd, GPIO6);			// GPRS_STD = 1
	mdelay(200);
	//GPIO_OutDisable(fd, GPIO6);

	close(fd);
	return 1;
}
int Me909_4G::resetMe909()
{
	int fd=OpenIO();
	if(fd==0) return 0;
	GPIO_OutEnable(fd, GPIO23 );
	//mdelay(200);
	GPIO_OutClear(fd, GPIO23);		// GPRS_STD = 0
	mdelay(80);
	GPIO_OutSet(fd, GPIO23);			// GPRS_STD = 1
	mdelay(200);
	GPIO_OutDisable(fd, GPIO23);

	close(fd);
	return 1;
}
// should delay 1s after done
int Me909_4G::LoadDrv()
{
	char cmdstr[200];
	memset(cmdstr, 0, sizeof(cmdstr));
	//sprintf(cmdstr, INSMOD_CMD, ME909_VENDORID, ME909_PRODUCTID);
#ifdef PRODUCT_FOR_EC
	sprintf(cmdstr, MODPROBE_CMD, EC20_VERDORID, EC20_PRODUCTID);
#else
	sprintf(cmdstr, MODPROBE_CMD, ME909_VENDORID, ME909_PRODUCTID);
#endif
	system(cmdstr);
	conn_status=CONN_STATUS_LOADDRV;
	return 0;
}
int Me909_4G::UnloadDrv()
{
	char cmdstr[200];
	memset(cmdstr, 0, sizeof(cmdstr));
	sprintf(cmdstr, RMMOD_CMD, USBSERIAL_CMD);
	system(cmdstr);
	conn_status=CONN_STATUS_UNLOADDRV;
	return 0;
}
int Me909_4G::ChkDrv()
{
	char cmdstr[10], retbuf[200];
	memset(cmdstr, 0, sizeof(cmdstr));
	memset(retbuf, 0, sizeof(retbuf));
	sprintf(cmdstr, LSMOD_CMD);

	int status=systemWithRet(cmdstr, retbuf, sizeof(retbuf), USBSERIAL_CMD);
	if(status==1)
	{
		return 1;
	}
	return 0;
}
int Me909_4G::Dial()
{
	char cmdstr[50];
	memset(cmdstr, 0, sizeof(cmdstr));
	sprintf(cmdstr, DIAL4G_CMD);
	system(cmdstr);
	conn_status=CONN_STATUS_DIAL;
	return 0;
}
int Me909_4G::Dial3G()
{
	char cmdstr[50];
	memset(cmdstr, 0, sizeof(cmdstr));
	sprintf(cmdstr, DIAL3G_CMD);
	system(cmdstr);
	conn_status=CONN_STATUS_DIAL;
	return 0;
}
int Me909_4G::ChkPPP0pid()
{
	char FileName[40];
	memset(FileName, 0, sizeof(FileName));
	strcpy(FileName, "/var/run/ppp0.pid");
	//conn_status++;
	if(FileIsExist(FileName)==1) return 1;
	//conn_status=CONN_STATUS_PPPIDNONE;
	return 0;
}
int Me909_4G::getIP()
{
	char ip[20],gw[20];
	char cmdstr[100], retbuf[200];
	memset(cmdstr, 0, sizeof(cmdstr));
	memset(retbuf, 0, sizeof(retbuf));
	memset(ipAddr, 0, sizeof(ipAddr));
	memset(gwAddr, 0, sizeof(gwAddr));

	sprintf(cmdstr, INET_ADDR_CMD);
	conn_status=CONN_STATUS_IPCHK;
	int status=systemWithRet(cmdstr, retbuf, sizeof(retbuf), PPP0_CMD);
	if(status==1)
	{
		//conn_status++;
		printf("ipbuf=%s\r\n", retbuf);
		if(findIP(retbuf, "inet", ip, sizeof(ip))==1)
		{
			//conn_status++;
			strcpy(ipAddr, ip);
			if(strlen(ip)<5) return 0;
			if(findIP(retbuf, "peer", gw, sizeof(gw))==1)
			{
				strcpy(gwAddr, gw);
				printf("ip:%s,gw:%s\r\n", ip, gw);
				return 1;
			}
		}
	}
	return 0;
}
int Me909_4G::Route()
{
	char cmdstr[200];
	memset(cmdstr, 0, sizeof(cmdstr));
	//sprintf(cmdstr, ROUTE_CMD, gwAddr);
	if(strlen(aimIps)==0)sprintf(cmdstr, ROUTE_CMD_FOR_MULTINETWORKS, gwAddr);
	else sprintf(cmdstr, ROUTE_CMD_FOR_SPECIALIPS, aimIps, gwAddr);
	int status = system(cmdstr);
	if(status<0)
	{
		printf("cmd:%s\t error:%s", cmdstr, strerror(errno));
	}
	else
	{
		if(WIFEXITED(status))
		{
		    printf("normal termination, exit status = %d\n", WEXITSTATUS(status)); //取得cmdstring执行结果
		}
		else if(WIFSIGNALED(status))
		{
			printf("abnormal termination,signal number =%d\n", WTERMSIG(status));
		}
		else if(WIFSTOPPED(status))
		{
			printf("process stopped, signal number =%d\n", WSTOPSIG(status));
		}
		alive_status=1;
	}
	conn_status=CONN_STATUS_ROUTE;
	return 0;
}
int Me909_4G::Route2SpecialIps(char* ip)
{
	char cmdstr[200];
	memset(cmdstr, 0, sizeof(cmdstr));
	//sprintf(cmdstr, ROUTE_CMD, gwAddr);
	sprintf(cmdstr, ROUTE_CMD_FOR_SPECIALIPS, aimIps, gwAddr);
	int status = system(cmdstr);
	if(status<0)
	{
		printf("cmd:%s\t error:%s", cmdstr, strerror(errno));
	}
	else
	{
		if(WIFEXITED(status))
		{
			printf("normal termination, exit status = %d\n", WEXITSTATUS(status)); //取得cmdstring执行结果
		}
		else if(WIFSIGNALED(status))
		{
			printf("abnormal termination,signal number =%d\n", WTERMSIG(status));
		}
		else if(WIFSTOPPED(status))
		{
			printf("process stopped, signal number =%d\n", WSTOPSIG(status));
		}
		alive_status=1;
	}
	conn_status=CONN_STATUS_ROUTE;
	return 0;
}
int Me909_4G::findIP(char* source, const char* begin, char* ips, int len)
{
	int ret=0;
	memset(ips, 0, len);
	if(len<16) return 0;
	char* p=strstr(source, begin);
	if(p==NULL) return 0;
	int index=0;
	for(int i=0;i<(int)strlen(p);i++)
	{
		if(index>0)
		{
			if(p[i]==0x20 || p[i]=='/')
			{
				ret=1;
				break;	// end
			}
			else
			{
				ips[index]=p[i];
				index++;
			}
		}
		else
		{
			if(p[i]==0x20)	// begin
			{
				ips[index]=p[i];
				index++;
			}
		}
	}
	return ret;
}
int Me909_4G::systemWithRet(char* cmdstring, char* buf, int len, const char* chkstr)
{
	int ret=0;
	FILE *fstream=NULL;
	if(NULL==(fstream=popen(cmdstring, "r")))
	{
		printf("execute command failed:%s\r\n", cmdstring);
		return -1;
	}
	while(NULL!=fgets(buf, len, fstream))
	{
		printf("%s", buf);
		if(strstr(buf, chkstr)!=NULL)
		{
			ret=1;
			break;
		}
	}
	pclose(fstream);
	return ret;
}
int	Me909_4G::FileIsExist( char* FileName )
{
	FILE *fp;
	fp=fopen(FileName, "rt");
	if(fp!=NULL)
	{
		fclose(fp);
		return 1;
	}
	return 0;
}
int Me909_4G::OpenIO()
{
	int fd = open("/dev/esm6800_gpio", O_RDWR);
	if(fd < 0)
	{
		printf("Can not open /dev/esm6800_gpio, fd=%d\n", fd);
		return 0;
	}
	if(fd==0)
	{
		close(fd);
		return 0;
	}
	return fd;
}
int Me909_4G::GPIO_OutEnable(int fd, unsigned int dwEnBits)
{
	int 				rc;
	struct double_pars	dpars;

	dpars.par1 = ESM6800_GPIO_OUTPUT_ENABLE;		// 0
	dpars.par2 = dwEnBits;

	rc = write(fd, &dpars, sizeof(struct double_pars));
	return rc;
}

int Me909_4G::GPIO_OutDisable(int fd, unsigned int dwDisBits)
{
	int 				rc;
	struct double_pars	dpars;

	dpars.par1 = ESM6800_GPIO_OUTPUT_DISABLE;	// 1
	dpars.par2 = dwDisBits;

	rc = write(fd, &dpars, sizeof(struct double_pars));
	return rc;
}

int Me909_4G::GPIO_OutSet(int fd, unsigned int dwSetBits)
{
	int 				rc;
	struct double_pars	dpars;

	dpars.par1 = ESM6800_GPIO_OUTPUT_SET;	// 2
	dpars.par2 = dwSetBits;

	rc = write(fd, &dpars, sizeof(struct double_pars));
	return rc;
}

int Me909_4G::GPIO_OutClear(int fd, unsigned int dwClearBits)
{
	int 				rc;
	struct double_pars	dpars;

	dpars.par1 = ESM6800_GPIO_OUTPUT_CLEAR;	// 3
	dpars.par2 = dwClearBits;

	rc = write(fd, &dpars, sizeof(struct double_pars));
	return rc;
}

int Me909_4G::GPIO_PinState(int fd, unsigned int* pPinState)
{
	int 				rc;
	struct double_pars	dpars;

	dpars.par1 = ESM6800_GPIO_INPUT_STATE;	// 5
	dpars.par2 = *pPinState;

	rc = read(fd, &dpars, sizeof(struct double_pars));
	if(!rc)
	{
		*pPinState = dpars.par2;
	}
	return rc;
}
void Me909_4G::mdelay(int milliseconds)
{
	struct timeval time;

	time.tv_sec  = milliseconds / 1000;				// unit = second
	time.tv_usec = (milliseconds % 1000)*1000;		// unit = micro-second

	select(0,NULL,NULL,NULL,&time);
}
