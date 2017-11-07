#pragma once
#include "Core/CoreMinimal.h"
#include <functional>
#include <MobileDeviceBridge.h>

#define htoll(x) (x)
#define ltohl(x) (x)
#define MKID(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))

#define ID_STAT MKID('S','T','A','T')
#define ID_LIST MKID('L','I','S','T')
#define ID_ULNK MKID('U','L','N','K')
#define ID_SEND MKID('S','E','N','D')
#define ID_RECV MKID('R','E','C','V')
#define ID_DENT MKID('D','E','N','T')
#define ID_DONE MKID('D','O','N','E')
#define ID_DATA MKID('D','A','T','A')
#define ID_OKAY MKID('O','K','A','Y')
#define ID_FAIL MKID('F','A','I','L')
#define ID_QUIT MKID('Q','U','I','T')
#define ID_HELO MKID('H','E','L','O')
#define ID_FEAT MKID('F','E','A','T')

#define ADB_HOST ("127.0.0.1:5037")

namespace k3d
{
    namespace mobi
    {
        namespace android
        {
            constexpr size_t MAX_PAYLOAD_V1 = 4 * 1024;
            constexpr size_t MAX_PAYLOAD_V2 = 256 * 1024;
            constexpr size_t MAX_PAYLOAD = MAX_PAYLOAD_V2;

            class SocketChannel;

            struct AdbResponse
            {
                bool    Succeed;
                String  Message;
            };

            struct ByteBuffer
            {
                ByteBuffer(int Capacity)
                    : m_Position(0)
                    , m_Capacity(Capacity)
                {
                    if (Capacity > 0)
                    {
                        m_Data = new U8[Capacity];
                    }
                    else
                    {
                        m_Data = nullptr;
                    }
                }

                ~ByteBuffer()
                {
                    if (m_Data)
                    {
                        delete[] m_Data;
                        m_Data = nullptr;
                    }
                }
                
                void Position(int InPos)
                {
                    m_Position = InPos;
                }
                int Position()const {
                    return m_Position;
                }

                int Capacity() const
                {
                    return m_Capacity;
                }

                void Put(int Offset, int Value)
                {
                    *(I32*)(m_Data + Offset) = Value;
                }

                void Put(int Offset, U8 Value)
                {
                    m_Data[Offset] = Value;
                }

                I32 GetInt(int Offset) const
                {
                    return *(I32*)(m_Data + Offset);
                }

                U8  Get(int Offset) const
                {
                    return m_Data[Offset];
                }
                friend class SocketChannel;
            private:
                U8* m_Data;
                int m_Position;
                int m_Capacity;
            };

            struct JdwpPacket
            {
                static const int HEADER_LEN = 11;
                static const int REPLY_PACKET = 0x80;

                static int s_SerialId;/* = 0x40000000*/;
                static JdwpPacket NullPacket;

                JdwpPacket(ByteBuffer& rawBuffer)
                    : Buffer(rawBuffer) {}

                ~JdwpPacket()
                {
                }

                ByteBuffer& GetPayload()
                {
                    Buffer.Position(HEADER_LEN);
                    return Buffer;
                }

                void    Write(SocketChannel* socket);

                void    FinishPacket(int cmdSet, int cmd, int payloadLength)
                {
                    Length = HEADER_LEN + payloadLength;
                    Id = GetNextSerial();
                    Flags = 0;
                    CmdSet = cmdSet;
                    Cmd = cmd;

                    Buffer.Put(0, Length);
                    Buffer.Put(4, Id);
                    Buffer.Put(8, Flags);
                    Buffer.Put(9,(U8)CmdSet);
                    Buffer.Put(10, (U8)Cmd);
                }

                static int  GetNextSerial();

                static JdwpPacket FindPacket(ByteBuffer& buf)
                {/*
                    int count = buf.position();
                    int length, id, flags, cmdSet, cmd;

                    if (count < JDWP_HEADER_LEN)
                        return null;

                    ByteOrder oldOrder = buf.order();
                    buf.order(ChunkHandler.CHUNK_ORDER);

                    length = buf.getInt(0x00);
                    id = buf.getInt(0x04);
                    flags = buf.get(0x08) & 0xff;
                    cmdSet = buf.get(0x09) & 0xff;
                    cmd = buf.get(0x0a) & 0xff;

                    buf.order(oldOrder);

                    if (length < JDWP_HEADER_LEN)
                        throw new BadPacketException();
                    if (count < length)
                        return null;

                    JdwpPacket pkt = new JdwpPacket(buf);
                    //pkt.mBuffer = buf;
                    pkt.mLength = length;
                    pkt.mId = id;
                    pkt.mFlags = flags;

                    if ((flags & REPLY_PACKET) == 0) {
                        pkt.mCmdSet = cmdSet;
                        pkt.mCmd = cmd;
                        pkt.mErrCode = -1;
                    }
                    else {
                        pkt.mCmdSet = -1;
                        pkt.mCmd = -1;
                        pkt.mErrCode = cmdSet | (cmd << 8);
                    }

                    return pkt;*/
                    return NullPacket;
                }

                ByteBuffer& Buffer;
                I32     Length;
                I32     Id;
                I32     Flags;
                int     CmdSet;
                int     Cmd;
                int     ErrCode;
            };

            struct JdwpHandShake
            {
                static const int HANDSHAKE_GOOD = 1;
                static const int HANDSHAKE_NOTYET = 2;
                static const int HANDSHAKE_BAD = 3;
                static const int HANDSHAKE_LEN = 14;
                static char HANDSHAKEBUFFER[14];
                static int FindHandShake(ByteBuffer& buf);
            };

            struct ChunkHandler
            {
                static const int CHUNK_HEADER_LEN = 8; // 4-byte type, 4-byte len
                static const int DDMS_CMD_SET = 0xc7;
                static const int DDMS_CMD = 0x01;

                static ByteBuffer AllocateBuffer(int maxChunkLen)
                {
                    return ByteBuffer(JdwpPacket::HEADER_LEN + CHUNK_HEADER_LEN + maxChunkLen);
                }

                static ByteBuffer& GetChunkDataBuffer(ByteBuffer& jdwpBuf)
                {
                    jdwpBuf.Position(JdwpPacket::HEADER_LEN + CHUNK_HEADER_LEN);
                    return jdwpBuf;
                }

                static void FinishChunkPacket(JdwpPacket& packet, I32 type, I32 chunkLen)
                {
                    auto& buf = packet.GetPayload();
                    buf.Put(0, type);
                    buf.Put(4, chunkLen);
                    packet.FinishPacket(DDMS_CMD_SET, DDMS_CMD, CHUNK_HEADER_LEN + chunkLen);
                }
            };

            class ShellProtocol
            {
            public:
                enum Id : uint8_t
                {
                    kIdStdin = 0,
                    kIdStdout = 1,
                    kIdStderr = 2,
                    kIdExit = 3,
                    // Close subprocess stdin if possible.
                    kIdCloseStdin = 4,
                    // Window size change (an ASCII version of struct winsize).
                    kIdWindowSizeChange = 5,
                    // Indicates an invalid or unknown packet.
                    kIdInvalid = 255,
                };

                explicit ShellProtocol(SocketChannel* s);
                virtual ~ShellProtocol();

                const char* data() const { return buffer_ + kHeaderSize; }
                char* data() { return buffer_ + kHeaderSize; }

                size_t data_capacity() const { return buffer_end_ - data(); }

                bool Read();

                int id() const { return buffer_[0]; }

                size_t data_length() const { return data_length_; }

                bool Write(Id id, size_t length);
            private:
                // Packets support 4-byte lengths.
                typedef uint32_t length_t;
                enum {
                    // It's OK if MAX_PAYLOAD doesn't match on the sending and receiving
                    // end, reading will split larger packets into multiple smaller ones.
                    kBufferSize = MAX_PAYLOAD,
                    // Header is 1 byte ID + 4 bytes length.
                    kHeaderSize = sizeof(Id) + sizeof(length_t)
                };

                SocketChannel* m_Sock;
                char buffer_[kBufferSize];
                size_t data_length_ = 0, bytes_left_ = 0;
                // We need to be able to modify this value for testing purposes, but it
                // will stay constant during actual program use.
                char* buffer_end_ = buffer_ + sizeof(buffer_);
            };

            union SyncMsg {
                KPACK(struct {
                    uint32_t id;
                    uint32_t mode;
                    uint32_t size;
                    uint32_t time;
                } stat_v1;)
                    KPACK(struct {
                    uint32_t id;
                    uint32_t error;
                    uint64_t dev;
                    uint64_t ino;
                    uint32_t mode;
                    uint32_t nlink;
                    uint32_t uid;
                    uint32_t gid;
                    uint64_t size;
                    int64_t atime;
                    int64_t mtime;
                    int64_t ctime;
                } stat_v2;)
                    KPACK(struct {
                    uint32_t id;
                    uint32_t mode;
                    uint32_t size;
                    uint32_t time;
                    uint32_t namelen;
                } dent;)
                    KPACK(struct {
                    uint32_t id;
                    uint32_t size;
                } data;)
                    KPACK(struct {
                    uint32_t id;
                    uint32_t msglen;
                } status;)
            };

            KPACK(struct SyncRequest {
                uint32_t id;  // ID_STAT, et cetera.
                uint32_t path_length;  // <= 1024
                                       // Followed by 'path_length' bytes of path (not NUL-terminated).
            };)

            class SocketChannel : public os::Socket
            {
            public:
                explicit SocketChannel(const char* ipAddr = nullptr);
                
                virtual ~SocketChannel() override;
                bool                    Connected() const { return m_Connected; }
                I32                     Write(String const& Data);
                I32                     Write(ByteBuffer const& buffer, int length);
                I32                     Read(void*Data, size_t Len);
                I32                     Read(ByteBuffer& buffer);

                void                    SendJdwpPacket(JdwpPacket const& packet);

                I32                     ReadInt();
                String                  ReadString(int strLen);

                friend class            AndroidBridge;
                friend struct           JdwpPacket;
                friend class            Client;

            private:
                bool                    m_Connected;
                os::IpAddress           m_SockAddr;
            };

            using SocketChannelPtr = SharedPtr<SocketChannel>;

            class AdbHelper
            {
            public:
                typedef void(*LogCharReceive)(char c, void* UserData);

                static SocketChannelPtr Open(String const& DeviceSerialId, int DevicePort);
                static void             RunLogService(String const& DeviceSerialId, String const& LogName, LogCharReceive Callback, void* UserData);

                /**
                * @param pid the process pid to connect to.
                */
                static SocketChannelPtr CreatePassThroughConnection(String const& DeviceSerialId, int PID);

                static void             ExecuteRemoteCommand(String const& DeviceSerialId, String const& Cmd, String& Output, int TimeOutInMS = 1000, int RecvRetryLimit = 2);
                /*
                * Send a SPSS (Sampling Profiling Streaming Start) request to the client.
                * @param bufferSize
                * @param samplingInterval
                * @param samplingIntervalTimeUnits
                */
                static void             SendSPSS(int bufferSize, int samplingInterval, int samplingIntervalTimeUnit);

                /*
                * Send a MPRQ (Method PRofiling Query) request to the client.
                */
                static void             SendMPRQ();

                static void             SendHello(SocketChannel* sock);

                static void             ListJDWPProcesses(IDevice* InDevice, std::function<void(DynArray<int>& pids)> callback);

                static void             SetDevice(SocketChannel& Socket, String const& SerialId);
                
                static String           FormAdbRequest(String const& Request);
                
                static AdbResponse      ReadResponse(SocketChannel& Socket, bool ReadDiagString);
                
                static String           CreateForwardRequest(String const&Addr, int Port);

                static String           CreateJdwpForwardRequest(int PID);

            private:
                static void             ProcessingIncomingJDWPData(SocketChannel& sock, DynArray<int>& pids);
                static void             SendDeviceMonitoringRequest(SocketChannel& sock, String const& serial);
            };

        }
    }
}