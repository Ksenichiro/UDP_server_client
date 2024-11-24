#include <iostream>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <mutex>
#include <cstdlib>  // For rand() and RAND_MAX
#include <ctime>    // For seeding srand()

#include <fstream>
#include <sstream>
#include <map>

#include <limits>
#include <iomanip> //for double precision output

#include "logger.h"

std::map<std::string, std::string> parseConfigFile(const std::string& filename);
void saveVectorToCSV(const std::vector<double>& data, const std::string& filename); // TODO binary output

class Server {
public:
    Server(int port) : port(port) {}

    void start(); // main loop

private:
    int port;
    std::mutex dataMutex; // Mutex for thread-safe access to clientData
    std::unordered_map<std::string, std::vector<double>> clientData;

    std::string getClientKey(const sockaddr_in& addr) {
        return std::to_string(addr.sin_addr.s_addr) + ":" + std::to_string(addr.sin_port);
    }

    // Improvment TODO: BITWISE NOT SPACED UNIQUE GENERATION

    double generateRandomDouble(double max);
    // Cheap uniqness || alternative O(n) space complexity unordered_map generation
    std::vector<double> generateLargeArray(double base);

    void sendArrayMetadata(int sockfd, sockaddr_in& clientAddr, socklen_t len, const std::vector<double>& array);

    void sendChunk(int sockfd, sockaddr_in& clientAddr, socklen_t len, const std::vector<double>& array, size_t chunkIndex);

    double calculateChecksum(const std::vector<double>& part);

    void handleClient(int sockfd, sockaddr_in clientAddr, socklen_t len, const std::string& command); 
};

//--------------------------------MAIN-----------------------------------------//




int main() {

    Logger& logger = Logger::getInstance();
    logger.log("Starting server logger...", Logger::INFO);
    logger.setLogFile("server.log");

    std::string configFileName = "config.txt";

    try {
        // Parse the configuration file
        std::map<std::string, std::string> config = parseConfigFile(configFileName);

        // Extract parameters
        int port = std::stoi(config["port"]);
        //int initValue = std::stoi(config["init_value"]);

        Server server(port); 
        server.start();
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }


    
}



//-------------------------------------------------------------------------//

void saveVectorToCSV(const std::vector<double>& data, const std::string& filename) {
    Logger& logger = Logger::getInstance();

    logger.log("Attempting to save vector to CSV file: " + filename, Logger::INFO);

    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        logger.log("Error: Could not open file " + filename + " for writing.", Logger::ERROR);
        return;
    }

    for (size_t i = 0; i < data.size(); ++i) {
        outFile<< std::setprecision(15) << data[i];
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

//-------------------------------------------------------------------------//
void Server::start() {
    
    Logger& logger = Logger::getInstance();
    

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        //perror("Socket creation failed");
        logger.log("Socket creation failed ", Logger::ERROR);
        exit(EXIT_FAILURE);
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(sockfd, (const sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        //perror("Bind failed");
        logger.log("Bind failed ", Logger::ERROR);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    logger.log("Server is running on port "+std::to_string(port), Logger::INFO);

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t len = sizeof(clientAddr);
        char buffer[256];
        ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (sockaddr*)&clientAddr, &len); 

        if (n > 0) {
            buffer[n] = '\0'; // Null-terminate the received string
            std::string command(buffer);
            logger.log("Received command: " + command, Logger::INFO);
            //std::cout << "Received command" << command << std::endl;
            std::thread(&Server::handleClient, this, sockfd, clientAddr, len, command).detach();
        }
    }

    close(sockfd);
}

//-------------------------------------------------------------------------//

double Server::generateRandomDouble(double max) {
    // Generate a random double in the range [0, x]
    double dMax = static_cast<double>(RAND_MAX);
    double dRand = static_cast<double>(rand());
    double test = dRand / dMax * max;
    //std::cout<<" generated value: "<<test<<std::endl;
    return test;
}

//-------------------------------------------------------------------------//

void Server::sendArrayMetadata(int sockfd, sockaddr_in& clientAddr, socklen_t len, const std::vector<double>& array) {
    const size_t MAX_PART_SIZE = 1000;
    size_t totalParts = (array.size() + MAX_PART_SIZE - 1) / MAX_PART_SIZE;

    sendto(sockfd, &totalParts, sizeof(totalParts), 0, (sockaddr*)&clientAddr, len);
    sendto(sockfd, &MAX_PART_SIZE, sizeof(MAX_PART_SIZE), 0, (sockaddr*)&clientAddr, len);
    std::cout << "Array metadata sent.\n";
}

//-------------------------------------------------------------------------//

std::vector<double> Server::generateLargeArray(double base) {

    std::vector<double> array(1'000'000);
    
    double stepMargin = base * 2.0 / 1'000'000.0;
    //std::cout<<" stepMargin value: "<<stepMargin<<std::endl;

    array[0] = -base + generateRandomDouble(stepMargin);
    
    for (size_t i = 1; i < array.size(); ++i) {
        array[i] = array[i-1] + generateRandomDouble(stepMargin); // 
    }
    return array;
}

//-------------------------------------------------------------------------//

void Server::sendChunk(int sockfd, sockaddr_in& clientAddr, socklen_t len, const std::vector<double>& array, size_t chunkIndex) {
    const size_t MAX_PART_SIZE = 1000;
    size_t start = chunkIndex * MAX_PART_SIZE;
    size_t end = std::min(start + MAX_PART_SIZE, array.size());

    std::vector<double> chunk(array.begin() + start, array.begin() + end);
    double checksum = calculateChecksum(chunk);

    sendto(sockfd, chunk.data(), chunk.size() * sizeof(double), 0, (sockaddr*)&clientAddr, len);
    sendto(sockfd, &checksum, sizeof(checksum), 0, (sockaddr*)&clientAddr, len);
    //std::cout << "Chunk " << chunkIndex << " sent.\n";
    Logger& logger = Logger::getInstance();
    logger.log("Chunk " + std::to_string(chunkIndex)+" sent.", Logger::INFO);
}

//-------------------------------------------------------------------------//

double Server::calculateChecksum(const std::vector<double>& part) {
    double checksum = 0;
    for (const auto& val : part) {
        checksum += val;
    }
    return checksum;
}

//-------------------------------------------------------------------------//

void Server::handleClient(int sockfd, sockaddr_in clientAddr, socklen_t len, const std::string& command) {
    std::string clientKey = getClientKey(clientAddr);
    Logger& logger = Logger::getInstance();

    //std::cout << "Client: " << clientKey << std::endl;
    logger.log("Client " + clientKey + " thread detached COMMAND: " + command, Logger::INFO);

    if(command.find("INIT") == 0 ){
        //std::cout << "Client: " << clientKey<<" established connection.\n";
        // TODO: Version Responce Sim.
        // TODO: pass value to array generation 
        // ? send value with  REQUEST_ARRAY
        size_t value = std::stoul(command.substr(5, 7));
        std::string version = command.substr(13);
        logger.log("Client " + clientKey + " established connection. Version: "+version, Logger::INFO);
        logger.log("Client " + clientKey + " established connection. Value of data: "+command.substr(5, 7), Logger::INFO); //TODO switch magic number 1`000`000 to value param

    } else if (command.find("REQUEST_ARRAY") == 0) {

        size_t baseNumber = std::stoul(command.substr(13));
        double bageGen = static_cast<double>(baseNumber);
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            clientData[clientKey] = generateLargeArray(baseNumber);
            //std::cout << "Created array for: " << clientKey << std::endl;
            logger.log("Client " + clientKey + " requested an array with base: " + std::to_string(baseNumber), Logger::INFO);
        }

        sendArrayMetadata(sockfd, clientAddr, len, clientData[clientKey]);
    } else if (command.find("REQUEST_CHUNK") == 0) {
        size_t chunkIndex = std::stoul(command.substr(13));
        
        //std::cout<<"Requsted chunk number: "<<chunkIndex;
        logger.log("Client " + clientKey + " requested chunk " + std::to_string(chunkIndex), Logger::DEBUG);
        std::vector<double> dataCopy;
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            if (clientData.find(clientKey) != clientData.end()) {
                dataCopy = clientData[clientKey];
            }
        }

        if (!dataCopy.empty()) {
            sendChunk(sockfd, clientAddr, len, dataCopy, chunkIndex);
        }
    } else if (command == "CONFIRM_RECEIPT") {
        std::lock_guard<std::mutex> lock(dataMutex);

        // TODO 

        std::string filename ="server_data_for_" + clientKey + ".csv";
        saveVectorToCSV(clientData[clientKey], filename);
        clientData.erase(clientKey);
        
        //std::cout << "Client confirmed receipt. Array removed.\n";
        logger.log("Client " + clientKey + " confirmed receipt of all chunks. Array cleared.", Logger::INFO);
    }
}

//-------------------------------------------------------------------------//

std::map<std::string, std::string> parseConfigFile(const std::string& filename) {
    std::map<std::string, std::string> config;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream lineStream(line);
        std::string key, value;

        if (std::getline(lineStream, key, '=') && std::getline(lineStream, value)) {
            config[key] = value;
        }
    }

    return config;
}