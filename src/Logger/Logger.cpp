#include "Logger.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>

// Static member definitions
bool Logger::enabled = false;
std::unique_ptr<Logger> Logger::instance = nullptr;
std::mutex Logger::instanceMutex;

Logger& Logger::getInstance()
{
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance)
    {
        throw std::runtime_error("Logger not initialized. Call Logger::initialize() first.");
    }
    return *instance;
}

void Logger::initialise(const std::filesystem::path& filepath)
{
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (!instance)
    {
        instance = std::unique_ptr<Logger>(new Logger(filepath));
        enabled = true;
    }
}

Logger::Logger(const std::filesystem::path &filepath) : logFile(filepath, std::ios::app)
{
    if (!logFile)
    {
        throw std::runtime_error("Failed to open log file: " + filepath.string());
    }
}

Logger::~Logger()
{
    if (logFile.is_open())
    {
        logFile.close();
    }
}

void Logger::log(LogLevel level, const std::string &message)
{
    if (Logger::enabled)
    {
        std::lock_guard<std::mutex> lock(mtx);
        logFile << "[" << timestamp() << "] "
                << "[" << levelToString(level) << "] "
                << message << '\n';
        logFile.flush();
    }
}

void Logger::info(const std::string &msg) { log(LogLevel::INFO, msg); }
void Logger::warning(const std::string &msg) { log(LogLevel::WARNING, msg); }
void Logger::error(const std::string &msg) { log(LogLevel::ERR, msg); }
void Logger::debug(const std::string &msg) { log(LogLevel::DEBUG, msg); }

bool Logger::getStatus() { return enabled; }

void Logger::startLogger()
{
    std::lock_guard<std::mutex> lock(mtx);
    enabled = true;
}

void Logger::StopLogger()
{
    std::lock_guard<std::mutex> lock(mtx);
    enabled = false;
    if (logFile.is_open())
    {
        logFile.flush();
    }
}

std::string Logger::levelToString(LogLevel level) const
{
    switch (level)
    {
    case LogLevel::INFO:
        return "INFO";
    case LogLevel::WARNING:
        return "WARNING";
    case LogLevel::ERR:
        return "ERROR";
    case LogLevel::DEBUG:
        return "DEBUG";
    }
    return "UNKNOWN";
}

std::string Logger::timestamp() const
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::ostringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
       << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}