#pragma once

#include "Core/Types.h"
#include "Core/Threading.h"
#include <atomic>
#include <string>
#include <vector>
#include <functional>
#include <queue>
#include <mutex>
#include <unordered_map>

namespace Gini {

enum class NetworkMode {
    None,
    Server,
    Client
};

enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Disconnecting
};

enum class PacketType : u8 {
    Ping = 0,
    Pong,
    Connect,
    Disconnect,
    Data,
    Reliable,
    Unreliable,
    Command,
    Sync,
    Custom
};

struct NetworkAddress {
    std::string ip = "127.0.0.1";
    u16 port = 7777;
    
    std::string ToString() const { return ip + ":" + std::to_string(port); }
};

struct Packet {
    PacketType type = PacketType::Data;
    u32 senderId = 0;
    u32 sequenceNumber = 0;
    u64 timestamp = 0;
    std::vector<u8> data;
    
    void Write(const void* src, size_t size);
    void Read(void* dst, size_t size);
    
    template<typename T>
    void Write(const T& value) { Write(&value, sizeof(T)); }
    
    template<typename T>
    T Read() { T value; Read(&value, sizeof(T)); return value; }
    
    void WriteString(const std::string& str);
    std::string ReadString();
    
private:
    size_t m_ReadPos = 0;
};

struct NetworkStats {
    u64 bytesSent = 0;
    u64 bytesReceived = 0;
    u64 packetsSent = 0;
    u64 packetsReceived = 0;
    f32 ping = 0.0f;
    f32 packetLoss = 0.0f;
};

using PacketHandler = std::function<void(const Packet&, u32 clientId)>;
using ConnectionHandler = std::function<void(u32 clientId)>;

class NetworkManager {
public:
    static NetworkManager& Get() {
        static NetworkManager instance;
        return instance;
    }
    
    bool InitServer(u16 port, u32 maxClients = 32);
    bool InitClient();
    void Shutdown();
    
    bool Connect(const std::string& ip, u16 port);
    void Disconnect();
    
    void Update();
    void ProcessPackets();
    
    void Send(const Packet& packet, u32 clientId = 0);
    void SendToAll(const Packet& packet, u32 excludeClient = 0);
    void SendReliable(const Packet& packet, u32 clientId = 0);
    void Broadcast(const Packet& packet);
    
    void RegisterPacketHandler(PacketType type, PacketHandler handler);
    void RegisterCustomHandler(u8 customType, PacketHandler handler);
    
    void OnClientConnected(ConnectionHandler handler) { m_OnClientConnected = handler; }
    void OnClientDisconnected(ConnectionHandler handler) { m_OnClientDisconnected = handler; }
    void OnConnected(std::function<void()> handler) { m_OnConnected = handler; }
    void OnDisconnected(std::function<void()> handler) { m_OnDisconnected = handler; }
    
    NetworkMode GetMode() const { return m_Mode; }
    ConnectionState GetState() const { return m_State; }
    bool IsServer() const { return m_Mode == NetworkMode::Server; }
    bool IsClient() const { return m_Mode == NetworkMode::Client; }
    bool IsConnected() const { return m_State == ConnectionState::Connected; }
    
    u32 GetClientId() const { return m_ClientId; }
    u32 GetClientCount() const { return static_cast<u32>(m_ConnectedClients.size()); }
    const std::vector<u32>& GetConnectedClients() const { return m_ConnectedClients; }
    
    const NetworkStats& GetStats() const { return m_Stats; }
    f32 GetPing() const { return m_Stats.ping; }
    
    void SetTickRate(u32 ticksPerSecond) { m_TickRate = ticksPerSecond; }
    u32 GetTickRate() const { return m_TickRate; }
    
private:
    NetworkManager() = default;
    ~NetworkManager() = default;
    
    void PollEvents();
    void HandlePacket(const Packet& packet, u32 senderId);
    void StartNetworkThread();
    void StopNetworkThread();
    void NetworkThreadMain();
    
    NetworkMode m_Mode = NetworkMode::None;
    ConnectionState m_State = ConnectionState::Disconnected;
    
    u32 m_ClientId = 0;
    u32 m_TickRate = 60;
    u32 m_MaxClients = 32;
    
    std::vector<u32> m_ConnectedClients;
    std::queue<Packet> m_IncomingPackets;
    std::queue<Packet> m_OutgoingPackets;
    std::mutex m_PacketMutex;
    
    std::unordered_map<PacketType, PacketHandler> m_PacketHandlers;
    std::unordered_map<u8, PacketHandler> m_CustomHandlers;
    
    ConnectionHandler m_OnClientConnected;
    ConnectionHandler m_OnClientDisconnected;
    std::function<void()> m_OnConnected;
    std::function<void()> m_OnDisconnected;
    
    NetworkStats m_Stats;
    WorkerThread m_NetworkThread;
    std::atomic<bool> m_ThreadedUpdateEnabled{true};
    
    // Platform-specific socket handle (void* for cross-platform)
    void* m_Socket = nullptr;
};

// Lockstep networking for RTS games
class LockstepManager {
public:
    static LockstepManager& Get() {
        static LockstepManager instance;
        return instance;
    }
    
    void Init(u32 tickRate = 20);
    void Shutdown();
    
    void Update(f32 deltaTime);
    
    void QueueCommand(const Packet& command);
    void ProcessTurn();
    
    bool IsReady() const { return m_Ready; }
    u32 GetCurrentTurn() const { return m_CurrentTurn; }
    u32 GetSimulationTick() const { return m_SimulationTick; }
    
    void SetTurnDelay(u32 turns) { m_TurnDelay = turns; }
    u32 GetTurnDelay() const { return m_TurnDelay; }
    
    using CommandHandler = std::function<void(const Packet&)>;
    void SetCommandHandler(CommandHandler handler) { m_CommandHandler = handler; }
    
private:
    LockstepManager() = default;
    
    struct TurnData {
        u32 turnNumber;
        std::vector<Packet> commands;
        bool confirmed = false;
    };
    
    std::unordered_map<u32, TurnData> m_TurnBuffer;
    std::queue<Packet> m_PendingCommands;
    
    CommandHandler m_CommandHandler;
    
    u32 m_CurrentTurn = 0;
    u32 m_SimulationTick = 0;
    u32 m_TurnDelay = 2;
    u32 m_TickRate = 20;
    f32 m_Accumulator = 0.0f;
    bool m_Ready = false;
};

} // namespace Gini
