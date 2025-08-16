#pragma once

#include <fstream>
#include <mutex>
#include <string>

enum class LogLevel
{
    INFO,
    WARNING,
    ERROR,
    DEBUG
};

class Logger
{
public:
    explicit Logger(const std::string &filename);
    ~Logger();

    void log(LogLevel level, const std::string &message);

    void info(const std::string &msg);
    void warning(const std::string &msg);
    void error(const std::string &msg);
    void debug(const std::string &msg);

private:
    std::ofstream logFile;
    std::mutex mtx;

    std::string levelToString(LogLevel level) const;
    std::string timestamp() const;
};