#include "Network.h"
#include "Core/Logger.h"
#include <cstring>

namespace Gini {

// Packet implementation
void Packet::Write(const void* src, size_t size) {
    size_t oldSize = data.size();
    data.resize(oldSize + size);
    std::memcpy(data.data() + oldSize, src, size);
}

void Packet::Read(void* dst, size_t size) {
    if (m_ReadPos + size > data.size()) {
        GINI_ERROR("Packet read overflow");
        return;
    }
    std::memcpy(dst, data.data() + m_ReadPos, size);
    m_ReadPos += size;
}

void Packet::WriteString(const std::string& str) {
    u32 length = static_cast<u32>(str.size());
    Write(length);
    if (length > 0) {
        Write(str.data(), length);
    }
}

std::string Packet::ReadString() {
    u32 length = Read<u32>();
    if (length == 0) return "";
    
    std::string str(length, '\0');
    Read(str.data(), length);
    return str;
}

// NetworkManager implementation
bool NetworkManager::InitServer(u16 port, u32 maxClients) {
    if (m_Mode != NetworkMode::None) {
        GINI_ERROR("Network already initialized");
        return false;
    }
    
    m_Mode = NetworkMode::Server;
    m_MaxClients = maxClients;
    m_State = ConnectionState::Connected;
    m_ClientId = 0; // Server is always client 0
    
    // TODO: Initialize platform-specific socket
    // For now, this is a stub implementation
    
    GINI_INFO("Network server started on port ", port);
    return true;
}

bool NetworkManager::InitClient() {
    if (m_Mode != NetworkMode::None) {
        GINI_ERROR("Network already initialized");
        return false;
    }
    
    m_Mode = NetworkMode::Client;
    m_State = ConnectionState::Disconnected;
    
    GINI_INFO("Network client initialized");
    return true;
}

void NetworkManager::Shutdown() {
    if (m_Mode == NetworkMode::None) return;
    
    Disconnect();
    
    m_Mode = NetworkMode::None;
    m_State = ConnectionState::Disconnected;
    m_ConnectedClients.clear();
    
    // Clear packet queues
    std::queue<Packet> empty1, empty2;
    std::swap(m_IncomingPackets, empty1);
    std::swap(m_OutgoingPackets, empty2);
    
    GINI_INFO("Network shutdown");
}

bool NetworkManager::Connect(const std::string& ip, u16 port) {
    if (m_Mode != NetworkMode::Client) {
        GINI_ERROR("Cannot connect: not in client mode");
        return false;
    }
    
    if (m_State != ConnectionState::Disconnected) {
        GINI_ERROR("Cannot connect: already connected or connecting");
        return false;
    }
    
    m_State = ConnectionState::Connecting;
    
    // TODO: Implement actual socket connection
    // For now, simulate successful connection
    m_State = ConnectionState::Connected;
    m_ClientId = 1; // Assigned by server in real implementation
    
    if (m_OnConnected) {
        m_OnConnected();
    }
    
    GINI_INFO("Connected to ", ip, ":", port);
    return true;
}

void NetworkManager::Disconnect() {
    if (m_State == ConnectionState::Disconnected) return;
    
    m_State = ConnectionState::Disconnecting;
    
    // Send disconnect packet
    Packet disconnectPacket;
    disconnectPacket.type = PacketType::Disconnect;
    disconnectPacket.senderId = m_ClientId;
    Send(disconnectPacket);
    
    m_State = ConnectionState::Disconnected;
    
    if (m_OnDisconnected) {
        m_OnDisconnected();
    }
    
    GINI_INFO("Disconnected from network");
}

void NetworkManager::Update() {
    if (m_Mode == NetworkMode::None) return;
    
    PollEvents();
    ProcessPackets();
}

void NetworkManager::ProcessPackets() {
    std::lock_guard<std::mutex> lock(m_PacketMutex);
    
    while (!m_IncomingPackets.empty()) {
        Packet packet = m_IncomingPackets.front();
        m_IncomingPackets.pop();
        
        HandlePacket(packet, packet.senderId);
    }
}

void NetworkManager::Send(const Packet& packet, u32 clientId) {
    if (m_State != ConnectionState::Connected) return;
    
    Packet p = packet;
    p.senderId = m_ClientId;
    
    std::lock_guard<std::mutex> lock(m_PacketMutex);
    m_OutgoingPackets.push(p);
    
    m_Stats.packetsSent++;
    m_Stats.bytesSent += packet.data.size();
}

void NetworkManager::SendToAll(const Packet& packet, u32 excludeClient) {
    if (!IsServer()) return;
    
    for (u32 clientId : m_ConnectedClients) {
        if (clientId != excludeClient) {
            Send(packet, clientId);
        }
    }
}

void NetworkManager::SendReliable(const Packet& packet, u32 clientId) {
    Packet reliablePacket = packet;
    reliablePacket.type = PacketType::Reliable;
    Send(reliablePacket, clientId);
}

void NetworkManager::Broadcast(const Packet& packet) {
    SendToAll(packet, 0);
}

void NetworkManager::RegisterPacketHandler(PacketType type, PacketHandler handler) {
    m_PacketHandlers[type] = handler;
}

void NetworkManager::RegisterCustomHandler(u8 customType, PacketHandler handler) {
    m_CustomHandlers[customType] = handler;
}

void NetworkManager::PollEvents() {
    // TODO: Poll socket for incoming data
    // This would be platform-specific (select/poll/epoll/IOCP/kqueue)
}

void NetworkManager::HandlePacket(const Packet& packet, u32 senderId) {
    m_Stats.packetsReceived++;
    m_Stats.bytesReceived += packet.data.size();
    
    // Handle built-in packet types
    switch (packet.type) {
        case PacketType::Ping: {
            Packet pong;
            pong.type = PacketType::Pong;
            pong.timestamp = packet.timestamp;
            Send(pong, senderId);
            break;
        }
        case PacketType::Pong: {
            // Calculate ping
            // m_Stats.ping = currentTime - packet.timestamp;
            break;
        }
        case PacketType::Connect: {
            if (IsServer()) {
                m_ConnectedClients.push_back(senderId);
                if (m_OnClientConnected) {
                    m_OnClientConnected(senderId);
                }
            }
            break;
        }
        case PacketType::Disconnect: {
            if (IsServer()) {
                auto it = std::find(m_ConnectedClients.begin(), m_ConnectedClients.end(), senderId);
                if (it != m_ConnectedClients.end()) {
                    m_ConnectedClients.erase(it);
                }
                if (m_OnClientDisconnected) {
                    m_OnClientDisconnected(senderId);
                }
            }
            break;
        }
        default:
            break;
    }
    
    // Call registered handler
    auto it = m_PacketHandlers.find(packet.type);
    if (it != m_PacketHandlers.end() && it->second) {
        it->second(packet, senderId);
    }
}

// LockstepManager implementation
void LockstepManager::Init(u32 tickRate) {
    m_TickRate = tickRate;
    m_CurrentTurn = 0;
    m_SimulationTick = 0;
    m_Accumulator = 0.0f;
    m_Ready = true;
    
    GINI_INFO("LockstepManager initialized at ", tickRate, " ticks/sec");
}

void LockstepManager::Shutdown() {
    m_Ready = false;
    m_TurnBuffer.clear();
    
    std::queue<Packet> empty;
    std::swap(m_PendingCommands, empty);
    
    GINI_INFO("LockstepManager shutdown");
}

void LockstepManager::Update(f32 deltaTime) {
    if (!m_Ready) return;
    
    f32 tickDuration = 1.0f / static_cast<f32>(m_TickRate);
    m_Accumulator += deltaTime;
    
    while (m_Accumulator >= tickDuration) {
        ProcessTurn();
        m_Accumulator -= tickDuration;
        m_SimulationTick++;
    }
}

void LockstepManager::QueueCommand(const Packet& command) {
    m_PendingCommands.push(command);
}

void LockstepManager::ProcessTurn() {
    // Check if we have commands for the current turn
    auto it = m_TurnBuffer.find(m_CurrentTurn);
    if (it != m_TurnBuffer.end() && it->second.confirmed) {
        // Execute all commands for this turn
        for (const auto& command : it->second.commands) {
            if (m_CommandHandler) {
                m_CommandHandler(command);
            }
        }
        
        // Clean up old turn data
        m_TurnBuffer.erase(it);
    }
    
    m_CurrentTurn++;
}

} // namespace Gini
