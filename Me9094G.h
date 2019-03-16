/*
 * Me9094G.h
 *
 *  Created on: 2019年2月23日
 *      Author: dell
 */

#ifndef ME9094G_H_
#define ME9094G_H_
#define CONN_STATUS_FAILED	0
#define CONN_STATUS_SUCCESS	1
#define CONN_STATUS_UNLOADDRV	2
#define CONN_STATUS_LOADDRV	3
#define CONN_STATUS_DIAL	4
#define CONN_STATUS_ROUTE	5
#define CONN_STATUS_PPPIDCHK	6
#define CONN_STATUS_PPPIDNONE	7
#define CONN_STATUS_IPCHK	19

#include <pthread.h>

class Me909_4G {
private:
	static void Me909ConnectionFunc(void* lparam);
	static int checkConnectionFunc(void* lparam);
public:	// interface
	void Connect4G();
	void Connect3G();
	void KeepAlive();
	int getConnectStatus(){return conn_status;};
	int getKeepAliveStatus(){return alive_status;}
	int getIP4Conn(char* ip, int maxlen);
	int getGW4Conn(char* gw, int maxlen);
	int setRouteIps(char* ips, int maxlen);
	int powerResetMe909();	// must be called when initialized
protected:
	int resetMe909();

	int UnloadDrv();
	int LoadDrv();
	int ChkDrv();
	int Dial();
	int Dial3G();
	int ChkPPP0pid();
	int Route();
	int Route2SpecialIps(char* ip);
	int getIP();

	int findIP(char* source, const char* begin, char* ips, int len);

	int systemWithRet(char* cmdstring, char* buf, int len, const char* chkstr);
	int	FileIsExist( char* FileName );
	int OpenIO();
	int GPIO_OutEnable(int fd, unsigned int dwEnBits);
	int GPIO_OutDisable(int fd, unsigned int dwDisBits);
	int GPIO_OpenDrainEnable(int fd, unsigned int dwODBits);
	int GPIO_OutSet(int fd, unsigned int dwSetBits);
	int GPIO_OutClear(int fd, unsigned int dwClearBits);
	int GPIO_PinState(int fd, unsigned int* pPinState);
	void mdelay(int milliseconds);

	int checkNetwork();
	int checkConnection();
	int checkModule();
protected:
	int flow_DrvChk(int retrynum);
	int flow_DialChk(int retrynum);
	int flow_Failed();
private:
	int conn_status;
	int alive_status;
	char ipAddr[20], gwAddr[20], aimIps[20];
	pthread_t m_thread;
public:
	Me909_4G();
	virtual ~Me909_4G();
};

#endif /* ME9094G_H_ */
