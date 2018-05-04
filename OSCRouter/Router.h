// Copyright (c) 2018 Electronic Theatre Controls, Inc., http://www.etcconnect.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once
#ifndef ROUTER_H
#define ROUTER_H

#ifndef NETWORK_UTILS_H
#include "NetworkUtils.h"
#endif

#ifndef EOS_LOG_H
#include "EosLog.h"
#endif

#ifndef OSC_PARSER_H
#include "OSCParser.h"
#endif

#ifndef ITEM_STATE_H
#include "ItemState.h"
#endif

#ifndef NETWORK_UTILS_H
#include "NetworkUtils.h"
#endif

class EosTcp;

////////////////////////////////////////////////////////////////////////////////

class Router
{
public:
	struct sConnection
	{
		QString						label;
		bool						server;
		OSCStream::EnumFrameMode	frameMode;
		EosAddr						addr;
		ItemStateTable::ID			itemStateTableId;
	};

	typedef std::vector<sConnection> CONNECTIONS;

	struct sRoute
	{
		sRoute() {}
		QString				label;
		EosRouteSrc			src;
		ItemStateTable::ID	srcItemStateTableId;
		EosRouteDst			dst;
		ItemStateTable::ID	dstItemStateTableId;
	};

	typedef std::vector<sRoute> ROUTES;
};

////////////////////////////////////////////////////////////////////////////////

class PacketLogger
	: public OSCParserClient
{
public:
	PacketLogger(EosLog::EnumLogMsgType logType, EosLog &log)
		: m_LogType(logType)
		, m_pLog(&log)
	{}

	virtual void SetPrefix(const std::string &prefix) {m_Prefix = prefix;}
	virtual void OSCParserClient_Log(const std::string &message);
	virtual void OSCParserClient_Send(const char *, size_t) {}

protected:
	EosLog::EnumLogMsgType	m_LogType;
	EosLog					*m_pLog;
	std::string				m_Prefix;
	std::string				m_LogMsg;
};

////////////////////////////////////////////////////////////////////////////////

class EosUdpInThread
	: public QThread
{
public:
	struct sRecvPacket
	{
		sRecvPacket(const char *data, int size, unsigned int Ip)
			: packet(data, size)
			, ip(Ip)
		{}
		EosPacket		packet;
		unsigned int	ip;
	};
	typedef std::vector<sRecvPacket> RECV_Q;

	EosUdpInThread();
	virtual ~EosUdpInThread();

	virtual void Start(const EosAddr &addr, ItemStateTable::ID itemStateTableId, unsigned int reconnectDelayMS);
	virtual void Stop();
	const EosAddr& GetAddr() const {return m_Addr;}
	ItemStateTable::ID GetItemStateTableId() const {return m_ItemStateTableId;}
	ItemState::EnumState GetState();
	virtual void Flush(EosLog::LOG_Q &logQ, RECV_Q &recvQ);

protected:
	EosAddr					m_Addr;
	ItemStateTable::ID		m_ItemStateTableId;
	ItemState::EnumState	m_State;
	unsigned int			m_ReconnectDelay;
	bool					m_Run;
	EosLog					m_Log;
	EosLog					m_PrivateLog;
	RECV_Q					m_Q;
	QMutex					m_Mutex;

	virtual void run();
	virtual void UpdateLog();
	virtual void SetState(ItemState::EnumState state);
};

////////////////////////////////////////////////////////////////////////////////

class EosUdpOutThread
	: public QThread
{
public:
	EosUdpOutThread();
	virtual ~EosUdpOutThread();

	virtual void Start(const EosAddr &addr, ItemStateTable::ID itemStateTableId, unsigned int reconnectDelayMS);
	virtual void Stop();
	const EosAddr& GetAddr() const {return m_Addr;}
	ItemStateTable::ID GetItemStateTableId() const {return m_ItemStateTableId;}
	ItemState::EnumState GetState();
	virtual bool Send(const EosPacket &packet);
	virtual void Flush(EosLog::LOG_Q &logQ);

protected:
	EosAddr					m_Addr;
	ItemStateTable::ID		m_ItemStateTableId;
	ItemState::EnumState	m_State;
	unsigned int			m_ReconnectDelay;
	bool					m_Run;
	EosLog					m_Log;
	EosLog					m_PrivateLog;
	EosPacket::Q			m_Q;
	QMutex					m_Mutex;

	virtual void run();
	virtual void UpdateLog();
	virtual void SetState(ItemState::EnumState state);
};

////////////////////////////////////////////////////////////////////////////////

class EosTcpClientThread
	: public QThread
{
public:
	EosTcpClientThread();
	virtual ~EosTcpClientThread();

	virtual void Start(const EosAddr &addr, ItemStateTable::ID itemStateTableId, OSCStream::EnumFrameMode frameMode, unsigned int reconnectDelayMS);
	virtual void Start(EosTcp *tcp, const EosAddr &addr, ItemStateTable::ID itemStateTableId, OSCStream::EnumFrameMode frameMode, unsigned int reconnectDelayMS);
	virtual void Stop();
	const EosAddr& GetAddr() const {return m_Addr;}
	ItemStateTable::ID GetItemStateTableId() const {return m_ItemStateTableId;}
	ItemState::EnumState GetState();
	virtual bool Send(const EosPacket &packet);
	virtual bool SendFramed(const EosPacket &packet);
	virtual void Flush(EosLog::LOG_Q &logQ, EosUdpInThread::RECV_Q &recvQ);

protected:
	EosTcp						*m_AcceptedTcp;
	EosAddr						m_Addr;
	ItemStateTable::ID			m_ItemStateTableId;
	ItemState::EnumState		m_State;
	OSCStream::EnumFrameMode	m_FrameMode;
	unsigned int				m_ReconnectDelay;
	bool						m_Run;
	EosLog						m_Log;
	EosLog						m_PrivateLog;
	EosUdpInThread::RECV_Q		m_RecvQ;
	EosPacket::Q				m_SendQ;
	QMutex						m_Mutex;

	virtual void run();
	virtual void UpdateLog();
	virtual void SetState(ItemState::EnumState state);
};

////////////////////////////////////////////////////////////////////////////////

class EosTcpServerThread
	: public QThread
{
public:
	struct sConnection
	{
		sConnection()
			: tcp(0)
		{}
		EosTcp	*tcp;
		EosAddr	addr;
	};
	typedef std::vector<sConnection> CONNECTION_Q;

	EosTcpServerThread();
	virtual ~EosTcpServerThread();

	virtual void Start(const EosAddr &addr, ItemStateTable::ID itemStateTableId, OSCStream::EnumFrameMode frameMode, unsigned int reconnectDelayMS);
	virtual void Stop();
	const EosAddr& GetAddr() const {return m_Addr;}
	ItemStateTable::ID GetItemStateTableId() const {return m_ItemStateTableId;}
	ItemState::EnumState GetState();
	OSCStream::EnumFrameMode GetFrameMode() const {return m_FrameMode;}
	virtual void Flush(EosLog::LOG_Q &logQ, CONNECTION_Q &connectionQ);

protected:
	EosAddr						m_Addr;
	ItemStateTable::ID			m_ItemStateTableId;
	ItemState::EnumState		m_State;
	OSCStream::EnumFrameMode	m_FrameMode;
	unsigned int				m_ReconnectDelay;
	bool						m_Run;
	EosLog						m_Log;
	EosLog						m_PrivateLog;
	CONNECTION_Q				m_Q;
	QMutex						m_Mutex;

	virtual void run();
	virtual void UpdateLog();
	virtual void SetState(ItemState::EnumState state);
};

////////////////////////////////////////////////////////////////////////////////

class RouterThread
	: public QThread
{
public:
	RouterThread(const Router::ROUTES &routes, const Router::CONNECTIONS &tcpConnections, const ItemStateTable &itemStateTable, unsigned int reconnectDelayMS);
	virtual ~RouterThread();

	virtual void Stop();
	virtual void Flush(EosLog::LOG_Q &logQ, ItemStateTable &itemStateTable);

protected:
	struct sRouteDst
	{
		EosRouteDst			dst;
		ItemStateTable::ID	itemStateTableId;
	};

	typedef std::vector<sRouteDst> ROUTE_DESTINATIONS;

	typedef std::map<QString,ROUTE_DESTINATIONS> ROUTES_BY_PATH;
	typedef std::pair<QString,ROUTE_DESTINATIONS> ROUTES_BY_PATH_PAIR;
	typedef std::pair<ROUTES_BY_PATH::const_iterator, ROUTES_BY_PATH::const_iterator> ROUTES_BY_PATH_RANGE;

	struct sRoutesByIp
	{
		ROUTES_BY_PATH	routesByPath;
		ROUTES_BY_PATH	routesByWildcardPath;
	};

	typedef std::map<unsigned int,sRoutesByIp> ROUTES_BY_IP;
	typedef std::pair<unsigned int,sRoutesByIp> ROUTES_BY_IP_PAIR;
	typedef std::pair<ROUTES_BY_IP::const_iterator, ROUTES_BY_IP::const_iterator> ROUTES_BY_IP_RANGE;

	typedef std::map<unsigned short,ROUTES_BY_IP> ROUTES_BY_PORT;
	typedef std::pair<unsigned short,ROUTES_BY_IP> ROUTES_BY_PORT_PAIR;
	typedef std::pair<ROUTES_BY_PORT::const_iterator, ROUTES_BY_PORT::const_iterator> ROUTES_BY_PORT_RANGE;

	typedef std::map<EosAddr,EosUdpInThread*> UDP_IN_THREADS;
	typedef std::map<EosAddr,EosUdpOutThread*> UDP_OUT_THREADS;

	typedef std::map<EosAddr,EosTcpClientThread*> TCP_CLIENT_THREADS;
	typedef std::map<EosAddr,EosTcpServerThread*> TCP_SERVER_THREADS;

	typedef std::vector<const ROUTE_DESTINATIONS*> DESTINATIONS_LIST;

	bool				m_Run;
	unsigned int		m_ReconnectDelay;
	Router::ROUTES		m_Routes;
	Router::CONNECTIONS	m_TcpConnections;
	EosLog				m_Log;
	EosLog				m_PrivateLog;
	ItemStateTable		m_ItemStateTable;
	QMutex				m_Mutex;

	virtual void run();
	virtual void BuildRoutes(ROUTES_BY_PORT &routesByPort, UDP_IN_THREADS &udpInThreads, UDP_OUT_THREADS &udpOutThreads, TCP_CLIENT_THREADS &tcpClientThreads, TCP_SERVER_THREADS &tcpServerThreads);
	virtual EosUdpOutThread* CreateUdpOutThread(const EosAddr &addr, ItemStateTable::ID itemStateTableId, UDP_OUT_THREADS &udpOutThreads);
	virtual void AddRoutingDestinations(bool isOSC, const QString &path, const sRoutesByIp &routesByIp, DESTINATIONS_LIST &destinations);
	virtual void ProcessRecvQ(ROUTES_BY_PORT &routesByPort, DESTINATIONS_LIST &routingDestinationList, UDP_OUT_THREADS &udpOutThreads, TCP_CLIENT_THREADS &tcpClientThreads, const EosAddr &addr, EosUdpInThread::RECV_Q &recvQ);
	virtual void ProcessRecvPacket(ROUTES_BY_PORT &routesByPort, DESTINATIONS_LIST &routingDestinationList, UDP_OUT_THREADS &udpOutThreads, TCP_CLIENT_THREADS &tcpClientThreads, const EosAddr &addr, EosUdpInThread::sRecvPacket &recvPacket);
	virtual bool MakeOSCPacket(const QString &srcPath, const EosRouteDst &dst, OSCArgument *args, size_t argsCount, EosPacket &packet);
	virtual void ProcessTcpConnectionQ(TCP_CLIENT_THREADS &tcpClientThreads, OSCStream::EnumFrameMode frameMode, EosTcpServerThread::CONNECTION_Q &tcpConnectionQ);
	virtual bool ApplyTransform(OSCArgument &arg, const EosRouteDst &dst, OSCPacketWriter &packet);
	virtual void MakeSendPath(const QString &srcPath, const QString &dstPath, QString &sendPath);
	virtual void UpdateLog();
	virtual void SetItemState(ItemStateTable::ID id, ItemState::EnumState state);
	virtual void SetItemActivity(ItemStateTable::ID id);
};

////////////////////////////////////////////////////////////////////////////////

#endif
