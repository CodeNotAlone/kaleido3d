#include "DeviceMonitor.h"
#include "Bridge.h"

namespace k3d
{
    namespace mobi
    {
        namespace android
        {
            Client::Client(IDevice * InDevice, int pid)
                : m_Device(InDevice)
                , m_PID(pid)
                , m_ReadBuffer(INIT_BUFFER_SIZE)
            {
                m_Status = Stat_Init;
                // create pass through
                m_JdwpSock = AdbHelper::CreatePassThroughConnection(m_Device->GetSerialId(), pid);
            }
            Client::~Client()
            {
            }
            bool Client::SendHandShake()
            {
                int Sent = m_JdwpSock->Send(JdwpHandShake::HANDSHAKEBUFFER, 14);
                return Sent == 14;
            }
            void Client::SendHello()
            {
                if (SendHandShake())
                {
                    char buffer[14] = { 0 };
                    int RecvLen = m_JdwpSock->Read(buffer, 14);
                    if (RecvLen == 14)
                    {
                        for (int i = 0; i < 14; i++)
                        {
                            if (buffer[i] != JdwpHandShake::HANDSHAKEBUFFER[i])
                                return;
                        }
                    }
                    else
                    {
                        return;
                    }
                    m_JdwpSock->SetBlocking(false);
                    auto& rawBuf = ChunkHandler::AllocateBuffer(4);
                    JdwpPacket packet(rawBuf);
                    auto& buf = packet.GetPayload();
                    buf.Put(0, (I32)1);
                    ChunkHandler::FinishChunkPacket(packet, ID_HELO, 4);
                    m_JdwpSock->SendJdwpPacket(packet);
                    /*char newBuf[1024] = { 0 };
                    m_JdwpSock->Read(newBuf, 1024);*/
                    //m_JdwpSock->Read() Reply Packet & buffer
                }
            }
            bool Client::Read()
            {
                if (m_ReadBuffer.Position() == m_ReadBuffer.Capacity()) 
                {
                    // Expand?
                }
                m_JdwpSock->Read(m_ReadBuffer);

                return false;
            }
            JdwpPacket Client::getPacket()
            {
                if (m_Status == Stat_AwaitShake)
                {
                    int Result = JdwpHandShake::FindHandShake(m_ReadBuffer);
                    switch (Result)
                    {
                    case JdwpHandShake::HANDSHAKE_GOOD:
                    {
                        m_Status = Stat_NeedDDMPkt;
                        SendHello();
                        return getPacket();
                    }
                    case JdwpHandShake::HANDSHAKE_BAD:
                        break;
                    case JdwpHandShake::HANDSHAKE_NOTYET:
                        break;
                    }
                }
                else if (m_Status == Stat_NeedDDMPkt ||
                    m_Status == Stat_NotDDM ||
                    m_Status == Stat_Ready)
                {
                    return JdwpPacket::FindPacket(m_ReadBuffer);
                }
                else
                {

                }

                return JdwpPacket::NullPacket;
            }
            DeviceMonitor::DeviceMonitor()
            {
            }
            DeviceMonitor::~DeviceMonitor()
            {
            }
            void DeviceMonitor::SendHello(String const& serial, int pid)
            {
            }
        }
    }
}
