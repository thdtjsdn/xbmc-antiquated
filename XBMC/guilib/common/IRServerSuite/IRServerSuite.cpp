/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "IRServerSuite.h"
#include "IrssMessage.h"
#include "ButtonTranslator.h"
#include "log.h"
#include "AdvancedSettings.h"

#define IRSS_PORT 24000

CRemoteControl g_RemoteControl;

CRemoteControl::CRemoteControl()
{
  m_socket = INVALID_SOCKET;
  m_bInitialized = false;
  m_isConnecting = false;
  Reset();
}

CRemoteControl::~CRemoteControl()
{
  Close();
}

void CRemoteControl::Disconnect()
{
  StopThread();
  Close();
}

void CRemoteControl::Close()
{
  m_isConnecting = false;
  if (m_socket != INVALID_SOCKET)
  {
    if (m_bInitialized)
    {
      m_bInitialized = false;
      CIrssMessage message(IRSSMT_UnregisterClient, IRSSMF_Request | IRSSMF_ForceNotRespond);
      SendPacket(message);
    }
    shutdown(m_socket, SD_BOTH);
    closesocket(m_socket);
    m_socket = INVALID_SOCKET;
  }
}

void CRemoteControl::Reset()
{
  m_isHolding = false;
  m_button = 0;
}

void CRemoteControl::Initialize()
{
  //trying to connect when there is nothing to connect to is kinda slow so kick it off in a thread.
  Create();
  SetName("CRemoteControl");
}

void CRemoteControl::Process()
{
  int iTries = 1;
  DWORD iMsRetryDelay = 5000;
  DWORD time = timeGetTime() - iMsRetryDelay;
  // try to connect 6 times @ a 5 second interval (30 seconds)
  // multiple tries because irss service might be up and running a little later then xbmc on boot.
  while (!m_bStop && iTries <= 6)
  {
    if (timeGetTime() - time >= iMsRetryDelay)
    {
      time = timeGetTime();
      if (Connect())
        break;
      iTries++;
    }
    Sleep(10);
  }
}

bool CRemoteControl::Connect()
{
  m_socket = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (m_socket == INVALID_SOCKET)
  {
    return false; //Couldn't create the socket
  }  
  // Get the local host information
  hostent* localHost = gethostbyname("");
  char* localIP = inet_ntoa (*(struct in_addr *)*localHost->h_addr_list);

  SOCKADDR_IN target;
  memset(&target, 0, sizeof(SOCKADDR_IN));

  target.sin_family = AF_INET;
  target.sin_addr.s_addr = inet_addr(localIP);
  target.sin_port = htons (IRSS_PORT);

  if (connect(m_socket, (SOCKADDR *)&target, sizeof(target)) == SOCKET_ERROR)
  {
    Close();
    return false; //Couldn't connect, irss not available
  }

  u_long iMode = 1; //non-blocking
  if (ioctlsocket(m_socket, FIONBIO, &iMode) == SOCKET_ERROR)
  {
    CLog::Log(LOGERROR, "IRServerSuite: failed to set socket to non-blocking.");
    Close();
    return false;
  }

  //register
  CIrssMessage mess(IRSSMT_RegisterClient, IRSSMF_Request);
  if (!SendPacket(mess))
  {
    CLog::Log(LOGERROR, "IRServerSuite: failed to send RegisterClient packet.");
    return false;
  }
  m_isConnecting = true;
  return true;
}

bool CRemoteControl::SendPacket(CIrssMessage& message)
{
  int iSize = 0;
  char* bytes = message.ToBytes(iSize);
  char buffer[4];
  uint32_t len = htonl(iSize);
  memcpy(&buffer[0], &len, 4);
  bool bResult = WriteN(&buffer[0], 4);
  if (bResult)
  {
    bResult = WriteN(bytes, iSize);
  }
  delete[] bytes;
  if (!bResult)
  {
    Close();
    return false;
  }
  return true;
}


void CRemoteControl::Update()
{
  if ((!m_bInitialized && !m_isConnecting) || (m_socket == INVALID_SOCKET))
  {
    return;
  }

  CIrssMessage mess;
  if (!ReadPacket(mess))
  {
    return;
  }
  switch (mess.GetType())
  {
  case IRSSMT_RegisterClient:
    m_isConnecting = false;
    if ((mess.GetFlags() & IRSSMF_Success) != IRSSMF_Success)
    {
      //uh oh, it failed to register
      Close();
      CLog::Log(LOGERROR, "IRServerSuite: failed to register XBMC as a client.");
    }
    else
    {
      m_bInitialized = true;
      //request info about receivers
      CIrssMessage mess(IRSSMT_DetectedReceivers, IRSSMF_Request);
      if (!SendPacket(mess))
      {
        CLog::Log(LOGERROR, "IRServerSuite: failed to send AvailableReceivers packet.");
      }
      mess.SetType(IRSSMT_AvailableReceivers);
      if (!SendPacket(mess))
      {
        CLog::Log(LOGERROR, "IRServerSuite: failed to send AvailableReceivers packet.");
      }
      mess.SetType(IRSSMT_ActiveReceivers);
      if (!SendPacket(mess))
      {
        CLog::Log(LOGERROR, "IRServerSuite: failed to send AvailableReceivers packet.");
      }
    }
    break;
  case IRSSMT_RemoteEvent:
    HandleRemoteEvent(mess);
    break;
  case IRSSMT_Error:
    //I suppose the errormessage is in the packet somewhere...
    CLog::Log(LOGERROR, "IRServerSuite: we got an error message.");
    break;
  case IRSSMT_ServerShutdown:
    Close();
    break;
  case IRSSMT_ServerSuspend:
    //should we do something?
    break;
  case IRSSMT_ServerResume:
    //should we do something?
    break;
  case IRSSMT_AvailableReceivers:
    {
      uint32_t size = mess.GetDataSize();
      if (size > 0)
      {
        char* data = mess.GetData();
        char* availablereceivers = new char[size + 1];
        memcpy(availablereceivers, data, size);
        availablereceivers[size] = '\0';
        CLog::Log(LOGINFO, "IRServerSuite: Available receivers: %s", availablereceivers);
        delete[] availablereceivers;
      }
    }
    break;
  case IRSSMT_DetectedReceivers:
    {
      uint32_t size = mess.GetDataSize();
      if (size > 0)
      {
        char* data = mess.GetData();
        char* detectedreceivers = new char[size + 1];
        memcpy(detectedreceivers, data, size);
        detectedreceivers[size] = '\0';
        CLog::Log(LOGINFO, "IRServerSuite: Detected receivers: %s", detectedreceivers);
        delete[] detectedreceivers;
      }
    }
    break;
  case IRSSMT_ActiveReceivers:
    {
      uint32_t size = mess.GetDataSize();
      if (size > 0)
      {
        char* data = mess.GetData();
        char* activereceivers = new char[size + 1];
        memcpy(activereceivers, data, size);
        activereceivers[size] = '\0';
        CLog::Log(LOGINFO, "IRServerSuite: Active receivers: %s", activereceivers);
        delete[] activereceivers;
      }
    }
    break;
  }
}

bool CRemoteControl::HandleRemoteEvent(CIrssMessage& message)
{
  try
  {
    //flag should be notify, maybe check it?
    char* data = message.GetData();
    uint32_t datalen = message.GetDataSize();
    char* deviceName;
    char* keycode;
    uint32_t devicenamelength;
    uint32_t keycodelength;
    if (datalen == 0)
    {
      CLog::Log(LOGERROR, "IRServerSuite: no data in remote message.");
      return false;
    }
    if (datalen <= 8)
    {
      //seems to be version 1.0.4.1, only keycode is sent, use Microsoft MCE mapping??
      devicenamelength = 13;
      deviceName = new char[devicenamelength + 1];
      sprintf(deviceName, "Microsoft MCE"); 
      keycodelength = datalen;
      keycode = new char[keycodelength + 1];
      memcpy(keycode, data, keycodelength);
    }
    else
    {
      //first 4 bytes is devicename length
      memcpy(&devicenamelength, data, 4);
      //devicename itself
      if (datalen < 4 + devicenamelength)
      {
        CLog::Log(LOGERROR, "IRServerSuite: invalid data in remote message (size: %u).", datalen);
        return false;
      }
      deviceName = new char[devicenamelength + 1];
      memcpy(deviceName, data + 4, devicenamelength);
      if (datalen < 8 + devicenamelength)
      {
        CLog::Log(LOGERROR, "IRServerSuite: invalid data in remote message (size: %u).", datalen);
        return false;
      }
      //next 4 bytes is keycode length
      memcpy(&keycodelength, data + 4 + devicenamelength, 4);
      //keycode itself
      if (datalen < 8 + devicenamelength + keycodelength)
      {
        CLog::Log(LOGERROR, "IRServerSuite: invalid data in remote message (size: %u).", datalen);
        delete[] deviceName;
        return false;
      }
      keycode = new char[keycodelength + 1];
      memcpy(keycode, data + 8 + devicenamelength, keycodelength);
    }
    deviceName[devicenamelength] = '\0';
    keycode[keycodelength] = '\0';
    //translate to a buttoncode xbmc understands
    m_button = CButtonTranslator::GetInstance().TranslateLircRemoteString(deviceName, keycode);
    if (g_advancedSettings.m_logLevel == LOG_LEVEL_DEBUG_FREEMEM)
    {
      CLog::Log(LOGINFO, "IRServerSuite, RemoteEvent: %s %s", deviceName, keycode);
    }
    delete[] deviceName;
    delete[] keycode;
    return true;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "IRServerSuite: exception while processing RemoteEvent.");
    return false;
  }
}

int CRemoteControl::ReadN(char *buffer, int n)
{
  int nOriginalSize = n;
  memset(buffer, 0, n);
  char *ptr = buffer;
  while (n > 0)
  {
    int nBytes = 0;
    nBytes = recv(m_socket, ptr, n, 0);

    if (WSAGetLastError() == WSAEWOULDBLOCK)
    {
      return nOriginalSize - n;
    }
    if (nBytes < 0)
    {
      if (!m_isConnecting)
      {
        CLog::Log(LOGERROR, "%s, IRServerSuite recv error %d", __FUNCTION__, GetLastError());
      }
      Close();
      return -1;
    }
    
    if (nBytes == 0)
    {
      CLog::Log(LOGDEBUG,"%s, IRServerSuite socket closed by server", __FUNCTION__);
      Close();
      break;
    }
    
    n -= nBytes;
    ptr += nBytes;
  }

  return nOriginalSize - n;
}

bool CRemoteControl::WriteN(const char *buffer, int n)
{
  const char *ptr = buffer;
  while (n > 0)
  {
    int nBytes = send(m_socket, ptr, n, 0);
    if (nBytes < 0)
    {
      CLog::Log(LOGERROR, "%s, IRServerSuite send error %d (%d bytes)", __FUNCTION__, GetLastError(), n);
      Close();
      return false;
    }
    
    if (nBytes == 0)
      break;
    
    n -= nBytes;
    ptr += nBytes;
  }

  return n == 0;
}

bool CRemoteControl::ReadPacket(CIrssMessage &message)
{
  try
  {
    char sizebuf[4];
    int iRead = ReadN(&sizebuf[0], 4);
    if (iRead <= 0) return false; //nothing to read
    if (iRead != 4)
    {
      CLog::Log(LOGERROR, "IRServerSuite: failed to read packetsize.");
      return false;
    }
    uint32_t size = 0;
    memcpy(&size, &sizebuf[0], 4);
    size = ntohl(size);
    char* messagebytes = new char[size];
    if (ReadN(messagebytes, size) != size)
    {
      CLog::Log(LOGERROR, "IRServerSuite: failed to read packet.");
      return false;
    }
    if (!CIrssMessage::FromBytes(messagebytes, size, message))
    {
      CLog::Log(LOGERROR, "IRServerSuite: invalid packet received (size: %u).", size);
      return false;
    }
    delete[] messagebytes;
    return true;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "IRServerSuite: exception while processing packet.");
    return false;
  }
}

WORD CRemoteControl::GetButton()
{
  return m_button;
}

bool CRemoteControl::IsHolding()
{
  return m_isHolding;
}
