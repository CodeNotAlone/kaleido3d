#include "Bridge.h"

#include <grpc/grpc.h>
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>

#include "gpu.grpc.pb.h"

namespace k3d
{
    namespace mobi
    {
        namespace android
        {
            class SyncConnection : public SocketChannel
            {
            public:
                SyncConnection(String const& serial) : SocketChannel()
                {
                    if (Connected())
                    {
                        AdbHelper::SetDevice(*this, serial);
                        auto Srv = AdbHelper::FormAdbRequest("sync:");
                        Send(Srv);
                        AdbResponse rsp = AdbHelper::ReadResponse(*this, false);
                        if (!rsp.Succeed)
                        {
                            printf("sync error: %s.\n", *rsp.Message);
                        }
                    }
                }

                ~SyncConnection()
                {}

                bool SendReq(int id, const char* pathMode)
                {

                }
            private:
            };

            static AndroidBridge ADB;

            AndroidBridge::AndroidBridge()
            {
                GetMobileDeviceBridge().Register("Android", this);
            }

            AndroidBridge::~AndroidBridge()
            {
            }

            void AndroidBridge::Start()
            {
            }

            bool AndroidBridge::QueryDevices(mobi::DeviceList & devices)
            {
                SocketChannel sock(ADB_HOST);
                if (!sock.Connected())
                {
                    printf("adb server not existed.\n");
                    return false;
                }
                sock.Write(AdbHelper::FormAdbRequest("host:track-devices"));
                AdbResponse resp = AdbHelper::ReadResponse(sock, false /* readDiagString */);
                if (resp.Succeed == false) 
                {
                    printf("adb refused, %s. \n", *resp.Message);
                    return false;
                }
                else
                {
                    I32 RespLen = 0;
                    ReadLength(sock, RespLen);
                    ProcessIncomingDeviceData(sock, RespLen, devices);
                }
                if (devices.Count() > 0)
                {
                    for (IDevice* device : devices)
                    {
                        auto rDev = static_cast<Device*>(device);
                        String output;
                        rDev->ExecuteCommand("getprop", output);
                        RegEx regex("\\[(?<key>(ro\\.build\\.version\\.release))\\]\\s*:\\s*\\[(?<value>(.*))\\]", RegEx::Default);
                        RegEx::Groups matched;
                        regex.Match(output.Data(), matched);
                        auto ver = matched[0].SubGroup("value");
                        ver.Length();
                        /*
                            [ro.build.version.release]: [8.0.0]
                            [ro.build.version.sdk] : [26]
                            [ro.product.manufacturer]: [Xiaomi]
                            [ro.product.model]: [MI 6]
                        */
                    }
                }
                return true;
            }

            void AndroidBridge::ReadLength(SocketChannel& Channel, I32 & Length)
            {
                char Buffer[4] = { 0 };
                Channel.Read(Buffer, 4);
                sscanf(Buffer, "%04x", &Length);
            }

            void AndroidBridge::ProcessIncomingDeviceData(SocketChannel& Channel, I32 Length, mobi::DeviceList & devices)
            {
                if (Length > 0)
                {
                    String Data(Length + 1, true);
                    Channel.Read(Data.Data(), Length);
                    auto Pos = Data.FindFirstOf("\n");
                    if (Pos != String::npos)
                    {
                        String RemainStr(Move(Data));
                        while (Pos != String::npos)
                        {
                            String OneDevLine = RemainStr.SubStr(0, Pos);
                            auto Sep = OneDevLine.FindFirstOf("\t");
                            String SerialId = OneDevLine.SubStr(0, Sep);
                            devices.Append(new android::Device(this, SerialId));
                            if (Pos != Length - 1)
                            {
                                RemainStr = RemainStr.SubStr(Pos + 1, RemainStr.Length() - Pos - 1);
                                Pos = RemainStr.FindFirstOf("\n");
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                }
            }

            class CPUProfiler : public ICPUProfiler
            {
            public:
                CPUProfiler();
                void        StartMonitor(ProcessInfo const& Process) override;
                void        StopMonitor(ProcessInfo const& Process) override;
                void        StartProfile(ProcessInfo const& Process) override;
                void        StopProfile(ProcessInfo const& Process) override;
                CPUDatas    GetData() override;
            };

            class GPUProfiler : public IGPUProfiler
            {
            public:
                GPUProfiler(std::shared_ptr<grpc::Channel>);
                void        StartMonitor(ProcessInfo const& Process) override;
                void        StopMonitor(ProcessInfo const& Process) override;
                void        StartProfile(ProcessInfo const& Process) override;
                void        StopProfile(ProcessInfo const& Process) override;
                GPUDatas    GetData() override;

            private:
                std::shared_ptr<profiler::proto::GpuService::Stub> m_Stub;
            };

            using ClientList = DynArray<SharedPtr<Client>>;

            struct DeviceImpl
            {
                DeviceImpl(const char* InSerial) 
                    : Serial(InSerial)
                    , Logger(nullptr)
                    , Log(this, &DeviceImpl::OnNewLogLine)
                {
                }

                void StartLog()
                {
                    LoggerThread = MakeShared<os::Thread>([=]() {
                        AdbHelper::RunLogService(Serial, "events", LogCallback, this);
                    }, "AndroidLog");
                    LoggerThread->Start();
                }

                static void LogCallback(char C, void *UserData)
                {
                    DeviceImpl* Device = reinterpret_cast<DeviceImpl*>(UserData);
                    Device->ReceiveLog(C);
                }

                void ReceiveLog(char C)
                {
                    Log << C;
                }

                void OnNewLogLine(String && LogLine)
                {
                    // Parse log line
                    if (Logger)
                    {
                        auto Pos = LogLine.FindFirstOf("DIWE");
                        if (Pos != String::npos)
                        {
                            ELogLevel Level = ELogLevel::Info;
                            switch (LogLine[Pos])
                            {
                            case 'I':
                                break;
                            case 'D':
                                Level = ELogLevel::Debug;
                                break;
                            case 'W':
                                Level = ELogLevel::Warn;
                                break;
                            case 'E':
                                Level = ELogLevel::Error;
                                break;
                            default:
                                break;
                            }
                            Logger->Log(Level, ".", *LogLine);
                        }
                    }
                }

                void CreateProfilerChannel()
                {
                    if (!Channel)
                    {
                        Channel = grpc::CreateChannel("tcp:12389",
                            grpc::InsecureChannelCredentials());
                    }
                }

                const char*             Serial;
                mutable ILogger*        Logger;
                SharedPtr<os::Thread>   LoggerThread;
                LogBuffer<DeviceImpl>   Log;
                std::shared_ptr<grpc::Channel>Channel;
                ClientList              Clients;
            };

            Device::Device(AndroidBridge* InBridge, String const& InSerialId)
                : m_Bridge(InBridge)
                , m_SerialId(InSerialId)
            {
                d = new DeviceImpl(*m_SerialId);
            }
            Device::~Device()
            {
                if (d)
                {
                    delete d;
                }
            }
            bool Device::IsConnected() const
            {
                return false;
            }
            void Device::Connect()
            {
            }
            void Device::StartLogService()
            {
                d->StartLog();
            }
            EPlatform Device::GetPlatform() const
            {
                return EPlatform::Android;
            }
            String Device::GetSerialId() const
            {
                return m_SerialId;
            }
            void Device::InstallLogger(ILogger * InLogger)
            {
                __intrinsics__::AtomicCASPointer((void**)&d->Logger, InLogger, d->Logger);
            }
            bool Device::InstallApp(String const & AppHostPath)
            {
                return false;
            }
            bool Device::Upload(String const & HostPath, String const & TargetPath)
            {
                return false;
            }
            bool Device::Download(String const & TargetPath, String const & HostPath)
            {
                return false;
            }
            Processes Device::ListAllRunningProcess()
            {
                Processes procs;
                DynArray<int> pids;
                AdbHelper::ListJDWPProcesses(this, [&pids](DynArray<int>& outPids) 
                {
                    pids = outPids;
                });
                return procs;
            }
            ProcessorInfo Device::GetProcessorInfo()
            {
                return ProcessorInfo();
            }
            IProfiler * Device::CreateProfiler(EProfileCategory const &)
            {
                return nullptr;
            }

            void Device::AddClient(ClientPtr client)
            {
                d->Clients.Append(client);
            }

            void Device::ExecuteCommand(String const & cmd, String&out)
            {
                AdbHelper::ExecuteRemoteCommand(m_SerialId, cmd, out, 500);
            }

            using namespace profiler::proto;

            CPUProfiler::CPUProfiler()
            {}
            void CPUProfiler::StartMonitor(ProcessInfo const & Process)
            {
            }
            void CPUProfiler::StopMonitor(ProcessInfo const & Process)
            {
            }
            void CPUProfiler::StartProfile(ProcessInfo const & Process)
            {
            }
            void CPUProfiler::StopProfile(ProcessInfo const & Process)
            {
            }
            CPUDatas CPUProfiler::GetData()
            {
                return CPUDatas();
            }
            GPUProfiler::GPUProfiler(std::shared_ptr<grpc::Channel> Channel)
                : m_Stub(profiler::proto::GpuService::NewStub(Channel))
            {
            }
            void GPUProfiler::StartMonitor(ProcessInfo const & Process)
            {
                grpc::ClientContext context;
                //m_Stub->StartMonitoringApp(&context,)
            }
            void GPUProfiler::StopMonitor(ProcessInfo const & Process)
            {
                grpc::ClientContext context;
            }
            void GPUProfiler::StartProfile(ProcessInfo const & Process)
            {
                grpc::ClientContext context;
            }
            void GPUProfiler::StopProfile(ProcessInfo const & Process)
            {
                grpc::ClientContext context;
            }
            GPUDatas GPUProfiler::GetData()
            {
                grpc::ClientContext context;
                GpuDataRequest request;
                GpuDataResponse response;
                m_Stub->GetData(&context, request, &response);
                return GPUDatas();
            }
}
    }
}