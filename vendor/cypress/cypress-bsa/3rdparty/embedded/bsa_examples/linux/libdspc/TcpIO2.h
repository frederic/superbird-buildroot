/****************************************************************************
 *
 *		AudioWeaver Support
 *		-------------------
 *
 ****************************************************************************
 *	TcpIO2.h
 ****************************************************************************
 *
 *	Description:	TCP I/O API
 *
 *	Copyright:		DSP Concepts, 2006-2012, All rights reserved.
 *	Owner:			DSP Concepts
 *					568 E. Weddell Drive, Suite 3
 *					Sunnyvale, CA 94089
 *
 ***************************************************************************/

/**
 * @addtogroup TcpIO
 * @{
 */

/**
 * @file
 * @brief TCP I/O API
 */

#ifndef _TCPIO2_H_INCLUDED_
#define _TCPIO2_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef WIN32
#include <afx.h>
#include <afxwin.h>
#include <afxmt.h>

#ifndef _WINSOCKAPI_
#include <winsock2.h>
#endif

#include <assert.h>

#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <assert.h>

#include "utiltypes.h"

/** Windows type definition for INVALID_SOCKET. */
#define INVALID_SOCKET	(SOCKET)(~0)

/** Windows type definition for SOCKET_ERROR. */
#define SOCKET_ERROR	(-1)

/** Windows type definition for LPHOSTENT. */
typedef struct hostent* LPHOSTENT;

/** Windows type definition for SOCKET. */
typedef int				SOCKET;

#endif

#include "basehelpers.h"

/** Maximum receive buffer size. */
#define MAX_RECEIVE_BYTES (1024*1024)

/** Structure for binary messages. */
struct SReceiveMessage
{
	/** Payload data. */
	vector<BYTE> m_data;

	/**
	 * @brief Get the length of the data
	 * @return						its length
	 */
	size_t size() const
	{
		return m_data.size();
	}

	/**
	 * @brief Get a byte from the data
	 * @param [in] i				index to get
	 * @return						value at that index
	 */
	unsigned char GetByte(size_t i) const
	{
		return m_data[i];
	}

	/**
	 * @brief Get a pointer to the message
	 * @return						the pointer
	 */
	unsigned char *GetData()
	{
		return &m_data[0];
	}
};

/** Magic TCP binary message marker. */
#define TCP_MAGIC		0x07ff0003

#ifdef WIN32
/** Windows notification message for message receipt. */
#define WM_TCPIP_MESSAGE			0x7e7e

/** Windows notification message for binary message receipt. */
#define WM_TCPIP_BINARYMESSAGE		0x7e7f
#endif

/** TCP I/O class. */
class CTcpIO2
{
	/** Socket used to listen on. */
	SOCKET m_hServerSocket;

	/** Socket for binary messages. */
	SOCKET m_hBinaryListenSocket;

	/** Socket currently active for send and receive. */
	SOCKET m_active_socket;

	/** The socket we'll listen on. */
	SOCKET m_hListenSock;

	/** True wnen connected. */
	bool m_connected;

	/** Set to make the listener thread exit. */
	bool m_bTerminateListenerThread;

	/** True while the listener thread is running. */
	bool m_listenThreadRunning;

	/** Port to listen on. */
	WORD m_nProtocolPort;

	LPHOSTENT m_lpHostEnt;

	/** The owner's this pointer for notifications. */
	void *m_pNotifyData;

	/** The owner's this pointer for connection lost notifications. */
	void *m_pLoseConnectData;

	/** Message received notification callback. */
	void (*m_pNotifyFunction)(void *pData, int count);

	/** Connection lost notification callback. */
	void (*m_pLoseConnectFunction)(void *pData);

	/** The message queue. */
	string_vector m_queue;

	/** The binary message queue. */
	vector<SReceiveMessage *> m_BinaryMsgQueue;

#ifdef WIN32
	/** The window we'll notify. */
	HWND m_hWindow;

	/** The user message we'll use. */
	unsigned int m_message;

	/** Protects the queue. */
	CCriticalSection m_queue_critical_section;

#else
	/** Protects the queue. */
	pthread_mutex_t m_queue_critical_section;
#endif

#ifdef WIN32
	/** The thread object. */
	CWinThread *m_pListenerThread;
#else
	/** The thread object. */
	pthread_t m_pListenerThread;
#endif

#ifdef WIN32
	/** The binary thread object. */
	CWinThread *m_pBinaryListenerThread;
#else
	/** The binary thread object. */
	pthread_t m_pBinaryListenerThread;
#endif

#ifndef WIN32
	pthread_attr_t m_tuning_attr;

	struct sched_param m_tuning_schedParam;
#endif

	/** Input read buffer. */
	char m_read_buffer[MAX_RECEIVE_BYTES];

	/** Binary message header. */
	SReceiveMessage m_CurrentReplyMsg;

	/** Binary message input buffer. */
	char m_BinaryReadBuffer[MAX_RECEIVE_BYTES];

	/** Remaining unread characters. */
	size_t m_in_read_buffer;

	/**
	 * @brief Set the host name and port
	 * @param [in] server			host name
	 * @param [in] port				host port
	 */
	void SetHostEnt(const char *server, int port);

	/** Create a send socket. */
	void MakeSocket();

	/**
	 * @brief Connect to the server.
	 * @param [in] nIPAddress		address to connect to
	 */
	void ConnectSocket(unsigned int nIPAddress = 0);

	/**
	 * @brief Spawn the listener thread.
	 * @param [in] pParam			our this pointer
	 */
#ifdef WIN32
	static unsigned int StartListenThread(void *pParam);
#else
	static void *StartListenThread(void *pParam);
#endif

	/**
	 * @brief Spawn the binary listener thread.
	 * @param [in] pParam			our this pointer
	 */
#ifdef WIN32
	static unsigned int StartBinaryListenThread(void *pParam);
#else
	static void *StartBinaryListenThread(void *pParam);
#endif

	/** Listen on a socket for a connection, and handle the connection. */
	void ListenThread();

	/** Listen on a binary socket for a connection, and handle the connection. */
	void BinaryListenThread();

	/**
	 * @brief Handle a connection from a talker.
	 * @param [in]
	 */
	bool OnSocketAccept(const in_addr &, SOCKET sock);

	/** Handle binary message from talker. */
	bool OnBinarySocketAccept();

	/**
	 * @brief Translate any type of IP address into the proper structure.
	 * @param [in] name				URL or dotted IP address
	 * @return						address object
	 */
	static LPHOSTENT TranslateAddress(const char *name);

	/**
	 * @brief Read a message from a socket
	 * @param [in] sock				socket to read from
	 * @return						the message read
	 */
	std::string ReadMessage(SOCKET sock);

	/**
	 * @brief Send a message string
	 * @param [in] str				string to send
	 * @return						0 or socket error number
	 */
	int SendString(SOCKET sock, const char *str);

public:
#ifdef WIN32
	/** Event for message receipt. */
	HANDLE m_hEvent;

	/** Event for binary message recipt. */
	HANDLE m_hEvent_Binary;
#endif

	/** Kill the send socket. */
	void KillSocket();

	int IsConnected() const
	{
		return m_connected;
	}

	/**
	 * @brief Get the IP Address of the indicated network card ( 1 - first card, 2 - 2nd card, etc.)
	 * @param [in] nWhichNetworkCard	network card to get from
	 * @return						IP address or ""
	 */
	std::string GetIPAddress(int nWhichNetworkCard);

	/** Default constructor. */
	CTcpIO2();

	/** Destructor. */
	~CTcpIO2();

	/** Kill the listener thread. */
	void KillListener();

	/** Clear the message queue. */
	void ClearQueue()
	{
		m_queue.clear();
	}

	/**
	 * @brief Get the queue size
	 * @return						the size
	 */
	size_t QueueSize() const
	{
		return m_queue.size();
	}

	/**
	 * @brief Get the first queue item
	 * @return						the titme
	 */
	const char *QueueItem() const
	{
		if (m_queue.size())
		{
			return m_queue[0].c_str();
		}
		return 0;
	}

	/** Erase the first item. */
	void EraseFirstItem()
	{
		if (m_queue.size())
		{
			m_queue.erase(m_queue.begin());
		}
	}

#ifdef WIN32
	/**
	 * @brief Set the HWND and message we'll notify with on Windows.
	 * @param [in] hWnd				window to notify
	 * @param [in] message			message to use
	 */
	void SetNotifyWindow(HWND hWnd, DWORD message)
	{
		m_hWindow = hWnd;
		m_message = message;
	}
#endif

	/**
	 * @brief Set a message notification callback function
	 * @param [in] pData			caller's this pointer
	 * @param [in] pNotifyFunction	callback
	 */
	void SetNotifyFunction(void *pData, void (*pNotifyFunction)(void *pData, int count))
	{
		m_pNotifyData = pData;
		m_pNotifyFunction = pNotifyFunction;
	}

	/**
	 * @brief Set a connection lost notification callback function
	 * @param [in] pData			caller's this pointer
	 * @param [in] pLoseConnectFunction callback
	 */
	void SetConnectLossFunction(void *pData, void (*pLoseConnectFunction)(void *pData))
	{
		m_pLoseConnectData = pData;
		m_pLoseConnectFunction = pLoseConnectFunction;
	}

	/**
	 * @brief Create a listener socket.
	 * @param [in] port				port to listen on
	 * @return						0 or socket error number
	 */
	int CreateListenerSocket(int port);

	/**
	 * @brief Create a binary listener socket.
	 * @param [in] port				port to listen on
	 * @return						0 or socket error number
	 */
	int CreateBinaryListenerSocket(int port);

	/**
	 * @brief Get the top message from the queue.
	 * @return						the message
	 */
	std::string GetMessage();

	/**
	 * @brief Get a message that might be binary.
	 * @param [out] textMessage		message value if text
	 * @param [out] pData			message value if binary
	 * @param [out] len				message length
	 * @return						0=test, 1=binary, <0 error
	 */
	int GetBinaryMessage(std::string &textMessage, void *&pData, int &len);

	/**
	 * @brief Get the top binary message from the queue.
	 * @return						the message object
	 */
	SReceiveMessage *BinaryGetMessage();

	/**
	 * @brief Get the number of waiting messages.
	 * @return						count of waiting messages
	 */
	size_t GetMessageCount();

	/**
	 * @brief Get the number of waiting binary messages.
	 * @return						count of waiting binary messages
	 */
	size_t GetBinaryMessageCount();

	/**
	 * @brief Send a message to the given server, does not attempt to wait for a reply.
	 * @param [in] server			server to send to
	 * @param [in] port				port to send on
	 * @param [in] message			message to send
	 * @return						0 or error code
	 */
	int SendTcpMessage(const char *server, int port, const char *message);

	/**
	 * @brief Send a binary message to the given server, does not attempt to wait for a reply.
	 * @param [in] server			server to send to
	 * @param [in] port				port to send on
	 * @param [in] message			message to send
	 * @param [in] nMsgLen			length of message
	 * @return						0 or error code
	 */
	int SendBinaryMessage(const char *server, int port, const BYTE *message, int nMsgLen);
};



#endif	// _TCPIO2_H_INCLUDED_

/**
 * @}
 * End of file
 */
