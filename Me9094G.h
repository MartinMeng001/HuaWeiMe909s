/*
 * Me9094G.h
 *
 *  Created on: 2019Äê2ÔÂ23ÈÕ
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
class Me909_4G {
private:
	static void Me909ConnectionFunc(void* lparam);
public:	// interface
	void Connect4G();
	int getConnectStatus();
	int getIP4Conn(char* ip, int maxlen);
protected:
	void flashLed();
	int UnloadDrv();
	int LoadDrv();
	int ChkDrv();
	int Dial();
	int ChkPPP0pid()
	int Route();
	int getIP();
private:
	int conn_status;
	char ipAddr[20];
public:
	Me909_4G();
	virtual ~Me909_4G();
};

#endif /* ME9094G_H_ */
