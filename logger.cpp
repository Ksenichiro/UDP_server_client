#include "Logger.h"
#include <vector>
#include <string>

void saveVectorToCSV(const std::vector<double>& data, const std::string& filename) {
    Logger& logger = Logger::getInstance();

    logger.log("Attempting to save vector to CSV file: " + filename, Logger::INFO);

    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        logger.log("Error: Could not open file " + filename + " for writing.", Logger::ERROR);
        return;
    }

    for (size_t i = 0; i < data.size(); ++i) {
        outFile << data[i];
        if (i != data.size() - 1) {
            outFile << ",";
        }
    }

    outFile.close();
    if (!outFile.good()) {
        logger.log("Error: Writing to file " + filename + " failed.", Logger::ERROR);
    } else {
        logger.log("Vector successfully saved to " + filename, Logger::INFO);
    }
}


