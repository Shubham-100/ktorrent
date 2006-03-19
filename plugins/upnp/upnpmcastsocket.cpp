/***************************************************************************
 *   Copyright (C) 2005 by Joris Guisson                                   *
 *   joris.guisson@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ***************************************************************************/
#include <kurl.h>
#include <qstringlist.h>
#include <ksocketaddress.h>
#include <util/log.h>
#include <torrent/globals.h>
#include <qfile.h>
#include <qtextstream.h>
#include "upnpmcastsocket.h"



using namespace KNetwork;
using namespace bt;

namespace kt
{
	
	UPnPMCastSocket::UPnPMCastSocket(bool verbose) : verbose(verbose)
	{
		routers.setAutoDelete(true);
		QObject::connect(this,SIGNAL(readyRead()),this,SLOT(onReadyRead()));
		QObject::connect(this,SIGNAL(gotError(int)),this,SLOT(onError(int)));
		setAddressReuseable(true);
		setFamily(KNetwork::KResolver::IPv4Family);
		setBlocking(true);
		for (Uint32 i = 0;i < 10;i++)
		{
			if (!bind(QString::null,QString::number(1900 + i)))
				Out() << "Cannot bind to UDP port 1900" << endl;
			else
				break;
		}	
		setBlocking(false);
	}
	
	
	UPnPMCastSocket::~UPnPMCastSocket()
	{
		close();
	}
	
	void UPnPMCastSocket::discover()
	{
		Out() << "Trying to find UPnP devices on the local network" << endl;
		
		// send a HTTP M-SEARCH message to 239.255.255.250:1900
		const char* data = "M-SEARCH * HTTP/1.1\r\n" 
				"HOST: 239.255.255.250:1900\r\n"
				"ST:urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n"
				"MAN:\"ssdp:discover\"\r\n"
				"MX:3\r\n"
				"\r\n\0";
		
		if (verbose)
		{
			Out() << "Sending : " << endl;
			Out() << data << endl;
		}
		
		KDatagramSocket::send(KNetwork::KDatagramPacket(data,strlen(data),KInetSocketAddress("239.255.255.250",1900)));
	}
	
	void UPnPMCastSocket::onReadyRead()
	{
		KNetwork::KDatagramPacket p = KDatagramSocket::receive();
		if (p.isNull())
			return;
		
		if (verbose)
		{
			Out() << "Received : " << endl;
			Out() << QString(p.data()) << endl;
		}
		
		// try to make a router of it
		UPnPRouter* r = parseResponse(p.data());
		if (r)
		{
			// download it's xml file
			bool ret = r->downloadXMLFile();
			if (!ret)
			{
				// we couldn't download and parse the XML file so 
				// get rid of it
				delete r;
			}
			else
			{
				// add it to the list and emit the signal
				routers.insert(r->getServer(),r);
				discovered(r);
			}
		}
		/*else
		{
			Out() << "UPnPMCastSocket : got packet" << endl;
			Out() << "Sender : " << p.address().toString() << endl;
			Out() << "Data (" << p.length() << ") : " << endl;
			Out() << QString(p.data()) << endl;
			Out() << endl;
			Out() << "Please send the log file to the KTorrent development if you see this message" << endl;
	}*/
	}
	
	UPnPRouter* UPnPMCastSocket::parseResponse(const QByteArray & arr)
	{
		QStringList lines = QStringList::split("\r\n",QString(arr),false);
		QString server;
		KURL location;
		
		// first read first line and see if contains a HTTP 200 OK message
		QString line = lines.first();
		if (!line.contains("HTTP") && !line.contains("200"))
		{
			return 0;
		}
		
		// quick check that the response being parsed is valid 
		bool validDevice = false; 
		for (Uint32 idx = 0;idx < lines.count() && !validDevice; idx++) 
		{ 
			line = lines[idx]; 
			if (line.contains("ST:") && line.contains("InternetGatewayDevice")) 
			{
				Out() << "Valid Internet Gateway Device has responded, parsing response...." << endl; 
				validDevice = true; 
			}
		} 
		if (!validDevice)
		{
			Out() << "Not a valid Internet Gateway Device" << endl;
			return 0; 
		}
		
		// read all lines and try to find the server and location fields
		for (Uint32 i = 1;i < lines.count();i++)
		{
			line = lines[i];
			if (line.startsWith("Location") || line.startsWith("LOCATION") || line.startsWith("location"))
			{
				location = line.mid(line.find(':') + 1).stripWhiteSpace();
				if (!location.isValid())
				{
					Out() << "Invalid URL" << endl;
					return 0;
				}
				Out() << "Location : " << location << endl;
			}
			else if (line.startsWith("Server") || line.startsWith("server") || line.startsWith("SERVER"))
			{
				server = line.mid(line.find(':') + 1).stripWhiteSpace();
				if (server.length() == 0)
					return 0;
				Out() << "Server : " << server << endl;
			}
		}
		
		if (routers.contains(server))
			return 0;
		
		// everything OK, make a new UPnPRouter
		return new UPnPRouter(server,location,verbose);
	}
	
	void UPnPMCastSocket::onError(int)
	{
		Out() << "UPnPMCastSocket Error : " << errorString() << endl;
	}
	
	void UPnPMCastSocket::saveRouters(const QString & file)
	{
		QFile fptr(file);
		if (!fptr.open(IO_WriteOnly))
		{
			Out() << "Cannot open file " << file << " : " << fptr.errorString() << endl;
			return;
		}
		
		// file format is simple : 2 lines per router, 
		// one containing the server, the other the location
		QTextStream fout(&fptr);
		bt::PtrMap<QString,UPnPRouter>::iterator i = routers.begin();
		while (i != routers.end())
		{
			UPnPRouter* r = i->second;
			fout << r->getServer() << endl;
			fout << r->getLocation().prettyURL() << endl;
			i++;
		}
	}
	
	void UPnPMCastSocket::loadRouters(const QString & file)
	{
		QFile fptr(file);
		if (!fptr.open(IO_ReadOnly))
		{
			Out() << "Cannot open file " << file << " : " << fptr.errorString() << endl;
			return;
		}
		
		// file format is simple : 2 lines per router, 
		// one containing the server, the other the location
		QTextStream fin(&fptr);
		
		while (!fin.atEnd())
		{
			QString server, location;
			server = fin.readLine();
			location = fin.readLine();
			if (!routers.contains(server))
			{
				UPnPRouter* r = new UPnPRouter(server,location);
				// download it's xml file
				if (!r->downloadXMLFile())
				{
				// we couldn't download and parse the XML file so 
				// get rid of it
					delete r;
				}
				else
				{
					routers.insert(server,r);
					discovered(r);
				}
			}
		}
	}
}



#include "upnpmcastsocket.moc"
