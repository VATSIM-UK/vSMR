#include "stdafx.h"
#include "UKCPIntegration.hpp"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

// Single definition of the shared pointer (declared extern in SMRRadar.hpp)
namespace SMRPluginSharedData {
    UKCPIntegration* ukcpIntegration = nullptr;
}

UKCPIntegration::UKCPIntegration() {}

UKCPIntegration::~UKCPIntegration() {
    Stop();
}

void UKCPIntegration::Start() {
    if (m_running.load())
        return;

    m_running.store(true);
    m_thread = std::thread(&UKCPIntegration::ConnectionThread, this);
    Logger::info("UKCPIntegration: Started");
}

void UKCPIntegration::Stop() {
    m_running.store(false);
    if (m_thread.joinable()) {
        m_thread.join();
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_frequencies.clear();
    m_connected.store(false);
    Logger::info("UKCPIntegration: Stopped");
}

std::string UKCPIntegration::GetDepartureFrequency(const std::string& callsign) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_frequencies.find(callsign);
    if (it != m_frequencies.end()) {
        return it->second;
    }
    return "";
}

void UKCPIntegration::ConnectionThread() {
    while (m_running.load()) {
        Logger::info("UKCPIntegration: Attempting connection to localhost:" + std::to_string(UKCP_PORT));

        SOCKET sock = TryConnect();
        if (sock == INVALID_SOCKET) {
            Logger::info("UKCPIntegration: Connection failed, retrying in " + std::to_string(RECONNECT_DELAY_MS / 1000) + "s");
            // Wait with periodic checks so we can stop quickly
            for (int i = 0; i < RECONNECT_DELAY_MS / 100 && m_running.load(); i++) {
                Sleep(100);
            }
            continue;
        }

        Logger::info("UKCPIntegration: Connected to UKCP");

        if (!SendHandshake(sock)) {
            Logger::info("UKCPIntegration: Handshake failed");
            closesocket(sock);
            m_connected.store(false);
            continue;
        }

        m_connected.store(true);
        Logger::info("UKCPIntegration: Handshake sent, listening for messages");

        // Read loop
        while (m_running.load() && ReadAndProcess(sock)) {
            // Keep reading
        }

        Logger::info("UKCPIntegration: Connection lost");
        closesocket(sock);
        m_connected.store(false);
        m_recvBuffer.clear();

        // Clear stale frequency data on disconnect
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_frequencies.clear();
        }

        // Wait before reconnecting
        for (int i = 0; i < RECONNECT_DELAY_MS / 100 && m_running.load(); i++) {
            Sleep(100);
        }
    }
}

SOCKET UKCPIntegration::TryConnect() {
    struct addrinfo hints = {};
    struct addrinfo* result = nullptr;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    std::string port = std::to_string(UKCP_PORT);
    int rc = getaddrinfo("127.0.0.1", port.c_str(), &hints, &result);
    if (rc != 0) {
        return INVALID_SOCKET;
    }

    SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == INVALID_SOCKET) {
        freeaddrinfo(result);
        return INVALID_SOCKET;
    }

    rc = connect(sock, result->ai_addr, (int)result->ai_addrlen);
    freeaddrinfo(result);

    if (rc == SOCKET_ERROR) {
        closesocket(sock);
        return INVALID_SOCKET;
    }

    // Set a receive timeout so we can periodically check m_running
    int timeout = 1000; // 1 second
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    return sock;
}

bool UKCPIntegration::SendHandshake(SOCKET sock) {
    // Build the init message using rapidjson
    rapidjson::Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    doc.AddMember("type", "initialise", alloc);
    doc.AddMember("version", 1, alloc);
    doc.AddMember("id", "vsmr-init-1", alloc);

    rapidjson::Value data(rapidjson::kObjectType);
    data.AddMember("integration_name", "vSMR", alloc);
    data.AddMember("integration_version", "1.0.0", alloc);

    rapidjson::Value subscriptions(rapidjson::kArrayType);
    rapidjson::Value sub(rapidjson::kObjectType);
    sub.AddMember("type", "departure_frequency_updated", alloc);
    sub.AddMember("version", 1, alloc);
    subscriptions.PushBack(sub, alloc);

    data.AddMember("event_subscriptions", subscriptions, alloc);
    doc.AddMember("data", data, alloc);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::string msg = buffer.GetString();
    msg += MESSAGE_DELIMITER;

    int sent = send(sock, msg.c_str(), (int)msg.length(), 0);
    if (sent == SOCKET_ERROR) {
        return false;
    }

    Logger::info("UKCPIntegration: Sent handshake: " + std::string(buffer.GetString()));
    return true;
}

bool UKCPIntegration::ReadAndProcess(SOCKET sock) {
    char buf[RECV_BUFFER_SIZE];

    int bytesRead = recv(sock, buf, RECV_BUFFER_SIZE - 1, 0);

    if (bytesRead == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAETIMEDOUT) {
            // should keep running?
            return true;
        }
        Logger::info("UKCPIntegration: recv error: " + std::to_string(err));
        return false;
    }

    if (bytesRead == 0) {
        // closed by server
        return false;
    }

    m_recvBuffer.append(buf, bytesRead);

    size_t pos;
    while ((pos = m_recvBuffer.find(MESSAGE_DELIMITER)) != std::string::npos) {
        std::string message = m_recvBuffer.substr(0, pos);
        m_recvBuffer.erase(0, pos + 1);

        if (!message.empty()) {
            ProcessMessage(message);
        }
    }

    return true;
}

void UKCPIntegration::ProcessMessage(const std::string& json) {
    rapidjson::Document doc;
    doc.Parse<0>(json.c_str());

    if (doc.HasParseError() || !doc.IsObject()) {
        Logger::info("UKCPIntegration: Failed to parse message: " + json);
        return;
    }

    if (!doc.HasMember("type") || !doc["type"].IsString()) {
        return;
    }

    std::string type = doc["type"].GetString();

    if (type == "initialisation_success") {
        Logger::info("UKCPIntegration: Initialisation successful");
    }
    else if (type == "initialisation_failure") {
        std::string errors = "";
        if (doc.HasMember("errors") && doc["errors"].IsArray()) {
            for (rapidjson::SizeType i = 0; i < doc["errors"].Size(); i++) {
                if (i > 0) errors += ", ";
                errors += doc["errors"][i].GetString();
            }
        }
        Logger::info("UKCPIntegration: Initialisation FAILED: " + errors);
    }
    else if (type == "departure_frequency_updated") {
        HandleDepartureFrequencyUpdated(json);
    }
}

void UKCPIntegration::HandleDepartureFrequencyUpdated(const std::string& json) {
    rapidjson::Document doc;
    doc.Parse<0>(json.c_str());

    if (doc.HasParseError() || !doc.IsObject()) return;
    if (!doc.HasMember("data") || !doc["data"].IsObject()) return;

    const rapidjson::Value& data = doc["data"];

    if (!data.HasMember("callsign") || !data["callsign"].IsString()) return;
    if (!data.HasMember("frequency") || !data["frequency"].IsString()) return;

    std::string callsign = data["callsign"].GetString();
    std::string frequency = data["frequency"].GetString();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_frequencies[callsign] = frequency;
    }

    Logger::info("UKCPIntegration: Frequency update - " + callsign + " -> " + frequency);
}
