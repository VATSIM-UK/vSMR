#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

enum class LogLevel
{
    INFO,
    WARNING,
    ERR,
    DEBUG
};

class Logger
{
    public:
    // Singleton access
    static Logger & getInstance();

    static void initialise(const std::filesystem::path & filepath);

    Logger(const Logger &)             = delete;
    Logger & operator=(const Logger &) = delete;

    ~Logger();

    void log(LogLevel level, const std::string & message);
    void info(const std::string & msg);
    void warning(const std::string & msg);
    void error(const std::string & msg);
    void debug(const std::string & msg);

    bool getStatus();
    void startLogger();
    void StopLogger();

    private:
    Logger() = default;
    Logger(const std::filesystem::path & filepath);

    std::ofstream logFile;
    std::mutex mtx;
    static bool enabled;
    static std::unique_ptr<Logger> instance;
    static std::mutex instanceMutex;

    std::string levelToString(LogLevel level) const;
    std::string timestamp() const;
};