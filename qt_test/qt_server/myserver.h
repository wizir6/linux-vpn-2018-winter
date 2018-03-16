
#ifndef MYSERVER_H
#define MYSERVER_H
#include <QObject>
#include <QtNetwork/QUdpSocket>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include "QMap"
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <QQueue>
#include <QTimer>
#include <iterator>


struct Client
{
    QString publicKey;
    QHostAddress realIpAddress;
    QTimer *timer = new QTimer();
    Client(QString pKey,QHostAddress &realIP)
    {
        publicKey = pKey;
        realIpAddress = realIP;
        timer->setInterval(1000);
    }
};

class MyServer : public QObject
{
    Q_OBJECT
public:
    explicit MyServer(QObject *parent = 0);

     QByteArray buildParameters(QString ipAddress);
     int run (int argc,char**argv);
     int createIdentificator();
     QString giveIPAddress();
     void handshake(QString str,QHostAddress sender,quint16 senderPort);
     int get_interface(char *name);
     ~MyServer(){ close(interface);}

signals:

public slots:
    void readyRead();
    void disconnect(QString ip);
private:

    QUdpSocket *mySocket;
    QMap<QString,Client> clients;
    int publicKey;
    int interface;
    QQueue<std::string> ipPool;
};

#endif // MYSERVER_H
