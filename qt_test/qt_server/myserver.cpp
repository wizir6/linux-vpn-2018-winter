// myudp.cpp

#include "myserver.h"

MyServer::MyServer(QObject *parent) :
    QObject(parent)
{
    // create a QUDP socket
    mySocket = new QUdpSocket(this);

    int j = 2;
    for (int i = 0; i < 254; i++)
    {
        ipPool.enqueue("10.0.0." + std::to_string(j));
        j++;
    }

    // The most common way to use QUdpSocket class is
    // to bind to an address and port using bind()
    // bool QAbstractSocket::bind(const QHostAddress & address,
    //     quint16 port = 0, BindMode mode = DefaultForPlatform)
    mySocket->bind(QHostAddress::AnyIPv4, 1234);
    connect(mySocket, SIGNAL(readyRead()), this, SLOT(readyRead()));
    publicKey = 321;
    interface = get_interface("tun44");
}

void MyServer::disconnect(QString ip)
{
    ipPool.enqueue(std::string(ip.toUtf8()));
    clients.remove(ip);
}

void MyServer::readyRead()
{

    // when data comes in
    QByteArray buffer;
    QByteArray newbuffer;
    buffer.resize(mySocket->pendingDatagramSize());
    newbuffer.resize(1500);
    QHostAddress sender;
    quint16 senderPort;
    QString type;
    // qint64 QUdpSocket::readDatagram(char * data, qint64 maxSize,
    //                 QHostAddress * address = 0, quint16 * port = 0)
    // Receives a datagram no larger than maxSize bytes and stores it in data.
    // The sender's host address and port is stored in *address and *port
    // (unless the pointers are 0).

    mySocket->readDatagram(buffer.data(), buffer.size(),
                         &sender, &senderPort);
    //check on length
    if(buffer.length()>0)
    {
       if(int(buffer[0])==0)
       {
           handshake((buffer.remove(0,1)),sender,senderPort);
           type = "handshake";
       }
       else
       {
           int wCount = 0;
           int rCount = 0;
           wCount = write(interface, buffer, buffer.size());
           rCount = read(interface, newbuffer.data(), newbuffer.size());
           qDebug() << "r" << rCount << " w" << wCount;

           if (rCount > 0)
           {
               struct iphdr *ip  = reinterpret_cast<iphdr*>(newbuffer.data());

               if (ip->version == 4)
               {
                   uint32_t ip_addr = ntohl(ip->daddr);
                   char buf[24];
                   inet_ntop(AF_INET, &ip->daddr, buf, sizeof(buf));
                   QString res = QString::fromLocal8Bit(buf);
                   auto it = clients.find(res);
                   if (it != clients.end())
                   {
                       mySocket->writeDatagram(newbuffer, it.value().realIpAddress, senderPort);
                       qDebug() << res;
                       type = "traffic";
                   }
                   else
                   {
                       qDebug() << "No user with this IP";
                   }

               }
               else
               {
                   qDebug() << "non IPv4 packet";
               }
           }
       }
        qDebug() << sender.toString() << "  " << senderPort << "  " << type;

    }
    buffer.clear();
    newbuffer.clear();

}
void MyServer::handshake(QString str,QHostAddress sender,quint16 senderPort)
{
 if(str=="NewClient")
 {
     QByteArray Data;
     Data.push_back(char(0));
     Data.push_back('i');
     Data.push_back(',');
     int i = createIdentificator();
     Data.push_back(QByteArray::number(i));
     for(int i =0; i<3; i++)
     {
        mySocket->writeDatagram(Data, sender, senderPort);
        qDebug()<<Data;
     }
     Data.clear();
}
 else
 {
     try{
     QStringList strings = str.split(' ');
     if(strings.size()==2)
     {
     QStringList paramId = strings[0].split(',');
     QStringList paramKey = strings[1].split(',');
     if(paramId.size()==2&&paramKey.size()==2)
     {
        if(paramId[0]=="i"&&paramKey[0]=="k")
        {
            //TODO:modificate public key firstly
            QString key  = paramKey[1];
            QString localIP = giveIPAddress();
            auto myClient = clients.insert(localIP, Client(key, sender));
            connect(myClient->timer, SIGNAL(void timeout()), this, SLOT(void disconnect(myClient.key())));
            QByteArray Data = buildParameters(localIP);
            for(int i =0; i<3; i++)
             {  mySocket->writeDatagram(Data, sender, senderPort);
                  qDebug()<<Data;
            }
            myClient->timer->start();
        }
        else throw std::runtime_error("Bad argument fo aurhorization");
     }
     }
     }
     catch(std::runtime_error& ex)
     {

         qDebug()<<ex.what();
     }
 }
}
int MyServer::createIdentificator()
{
return 123;
}
QString MyServer::giveIPAddress()
{

    return QString::fromStdString(ipPool.dequeue());
}

QByteArray MyServer:: buildParameters(QString ipAddress)
{
    /* "  -m <MTU> for the maximum transmission unit\n"
               "  -a <address> <prefix-length> for the private address\n"
               "  -r <address> <prefix-length> for the forwarding route\n"
               "  -d <address> for the domain name server\n"
               "  -s <domain> for the search domain\n"*/
    //

   short mtu = 1500;
   QByteArray route ="0.0.0.0";
   QByteArray dns ="8.8.8.8";

 QByteArray parametres;
 parametres.push_back(char(0));
 parametres.push_back('p');
 parametres.push_back("m,");
 parametres.push_back(QByteArray::number(mtu));
 parametres.push_back(" ");
 parametres.push_back("a,");
 parametres.push_back(ipAddress.toUtf8());
 parametres.push_back(",24");
 parametres.push_back(" ");
 parametres.push_back("r,");
 parametres.push_back(route);
 parametres.push_back(",0");
 parametres.push_back(" ");
 parametres.push_back("d,");
 parametres.push_back(dns);
 parametres.push_back(" ");
// parametres.push_back("s,");parametres.push_back(" ");
 //Now we just return empty publickey
 parametres.push_back("k,");parametres.push_back(QByteArray::number(publicKey));

 return parametres;

}
int MyServer::get_interface(char *name)
{
    int interface = open("/dev/net/tun", O_RDWR | O_NONBLOCK);
        ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
        strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
        if (ioctl(interface, TUNSETIFF, &ifr)) {
            perror("Cannot get TUN interface");
            exit(1);
        }
	else {
       system("ip address add 10.0.0.1/8 dev tun44");
	   system("ip link set up dev tun44");
	}
        return interface;
}

