#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>

class Logger {
public:
    enum LogLevel { INFO, DEBUG, WARNING, ERROR };

    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void setLogFile(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (logFile_.is_open()) {
            logFile_.close();
        }
        logFile_.open(filename, std::ios::app);
        if (!logFile_.is_open()) {
            std::cerr << "Error: Could not open log file: " << filename << std::endl;
        }
    }

    void log(const std::string& message, LogLevel level = INFO) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string levelStr = logLevelToString(level);
        std::string logMessage = "[" + levelStr + "] " + message;

        // Write to file if enabled
        if (logFile_.is_open()) {
            logFile_ << logMessage << std::endl;
        }

        // Always write to console
        std::cout << logMessage << std::endl;
    }

private:
    Logger() = default;
    ~Logger() {
        if (logFile_.is_open()) {
            logFile_.close();
        }
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string logLevelToString(LogLevel level) {
        switch (level) {
            case INFO: return "INFO";
            case WARNING: return "WARNING";
            case ERROR: return "ERROR";
            case DEBUG: return "DEBUG";
        }
        return "UNKNOWN";
    }

    std::ofstream logFile_;
    std::mutex mutex_;
};

void saveVectorToCSV(const std::vector<double>& data, const std::string& filename);

#endif // LOGGER_H
