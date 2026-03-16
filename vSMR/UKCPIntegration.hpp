#pragma once
#include <string>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "Logger.h"

#pragma comment(lib, "Ws2_32.lib")

// Connects to the UK Controller Plugin integration socket (localhost:52814)
// and listens for departure_frequency_updated messages.
class UKCPIntegration {
public:
    UKCPIntegration();
    ~UKCPIntegration();

    // Start the background connection thread
    void Start();

    // Stop the background connection thread and clean up
    void Stop();

    // Get the departure frequency for a callsign (thread-safe)
    // Returns empty string if no frequency is known
    std::string GetDepartureFrequency(const std::string& callsign);

    // Check if connected to UKCP
    bool IsConnected() const { return m_connected.load(); }

private:
    // Background thread entry point
    void ConnectionThread();

    // Attempt to connect to UKCP socket
    SOCKET TryConnect();

    // Send the initialisation handshake
    bool SendHandshake(SOCKET sock);

    // Read and process messages from the socket
    // Returns false if the connection was lost
    bool ReadAndProcess(SOCKET sock);

    // Parse a single JSON message
    void ProcessMessage(const std::string& json);

    // Parse the departure_frequency_updated message
    void HandleDepartureFrequencyUpdated(const std::string& json);

    // Thread-safe frequency map
    std::mutex m_mutex;
    std::map<std::string, std::string> m_frequencies;

    // Receive buffer for accumulating partial TCP data
    std::string m_recvBuffer;

    // Connection state
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_connected{false};
    std::thread m_thread;

    static const int UKCP_PORT = 52814;
    static const char MESSAGE_DELIMITER = '\x1F';
    static const int RECONNECT_DELAY_MS = 5000;
    static const int RECV_BUFFER_SIZE = 4096;
};
