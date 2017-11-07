#pragma once 
#include "AdbHelper.h"

namespace k3d
{
    namespace mobi
    {
        class IDevice;
        namespace android
        {
            class JdwpAgent
            {
            public:
                virtual ~JdwpAgent() {}
            };

            class Client : public JdwpAgent
            {
            public:
                enum EStatus
                {
                    Stat_Init,
                    Stat_NotJDWP,
                    Stat_AwaitShake,
                    Stat_NeedDDMPkt,
                    Stat_NotDDM,
                    Stat_Ready,
                    Stat_Error,
                    Stat_Disconnected
                };
                static const int INIT_BUFFER_SIZE = 2 * 1024;

                Client(IDevice* InDevice, int pid);
                ~Client() override;

                bool SendHandShake();
                void SendHello();

                bool Read();

                JdwpPacket getPacket();

            private:
                IDevice*            m_Device;
                int                 m_PID;
                SocketChannelPtr    m_JdwpSock;
                EStatus             m_Status;
                ByteBuffer          m_ReadBuffer;
            };

            class DeviceMonitor
            {
            public:
                DeviceMonitor();
                ~DeviceMonitor();
                    
                void SendHello(String const& serial, int pid);
            };
        }
    }
}