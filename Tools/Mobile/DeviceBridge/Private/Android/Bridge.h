#pragma once
#include "MobileDeviceBridge.h"
#include "Common/LogBuffer.h"
#include "AdbHelper.h"

namespace k3d
{
    namespace mobi
    {
        namespace android
        {
            class AndroidBridge : public IBridgeService
            {
            public:
                AndroidBridge();
                ~AndroidBridge();

                void        Start();
                bool        QueryDevices(mobi::DeviceList& devices) override;

            private:
                static void ReadLength(SocketChannel& Channel, I32& Length);
                void        ProcessIncomingDeviceData(SocketChannel& Channel, I32 Length, mobi::DeviceList& devices);
            };

            struct DeviceImpl;
            class Client;
            using ClientPtr = SharedPtr<Client>;
            using ClientList = DynArray<ClientPtr>;

            class Device : public IDevice
            {
            public:
                explicit Device(AndroidBridge* InBridge, String const& InSerialId);
                ~Device() override;
                
                bool            IsConnected() const override;
                void            Connect() override;
                void            StartLogService() override;
                EPlatform       GetPlatform() const override;
                String          GetSerialId() const override;
                void            InstallLogger(ILogger* InLogger) override;
                bool            InstallApp(String const& AppHostPath) override;
                bool            Upload(String const& HostPath, String const& TargetPath) override;
                bool            Download(String const& TargetPath, String const& HostPath) override;

                Processes       ListAllRunningProcess() override;
                ProcessorInfo   GetProcessorInfo() override;
                IProfiler*      CreateProfiler(EProfileCategory const&) override;

                void            AddClient(ClientPtr client);
                void            ExecuteCommand(String const& cmd, String&out);
            private:
                String          m_SerialId;
                AndroidBridge*  m_Bridge;
                DeviceImpl*     d;
            };

        }
    }
}