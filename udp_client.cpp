#include <iostream>
#include <vector>
#include <set>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <chrono>
#include <thread>
#include <algorithm>

#include "logger.h"

void saveVectorToBinary(const std::vector<double>& vec, const std::string& filename);

class Client {
public:
    Client(const std::string& serverIp, int serverPort)
        : serverIp(serverIp), serverPort(serverPort) {}

    void sendAndReceive(int number);

private:
    std::string serverIp;
    int serverPort;

    void requestChunk(int sockfd, sockaddr_in& serverAddr, socklen_t len, size_t chunkIndex);
};


int main(int argc, char* argv[]) {

    Logger& logger = Logger::getInstance();
    logger.log("Starting client logger...", Logger::INFO);
    logger.setLogFile("client.log");

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <number>\n";
        return 1;
    }

    std::string serverIp = argv[1];
    int number = std::atoi(argv[2]);

    Client client(serverIp, 8080); // Use port 8080 for the server
    client.sendAndReceive(number);

    return 0;
}



void saveVectorToBinary(const std::vector<double>& vec, const std::string& filename) {
    // Open the file in binary mode
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile) {
        std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
        return;
    }

    // Save the size of the vector first
    size_t size = vec.size();
    outFile.write(reinterpret_cast<const char*>(&size), sizeof(size));

    // Save the data in the vector
    if (size > 0) {
        outFile.write(reinterpret_cast<const char*>(vec.data()), size * sizeof(double));
    }

    outFile.close();
    std::cout << "Vector saved to binary file: " << filename << std::endl;
}

void Client::sendAndReceive(int number) {

    Logger& logger = Logger::getInstance();
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        //perror("Socket creation failed");
        logger.log("Socket creation failed ", Logger::ERROR);
        exit(EXIT_FAILURE);
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr);

    // Request the array
    //std::string request = "REQUEST_CHUNK " + std::to_string(chunkIndex);
    //sendto(sockfd, request.c_str(), request.size() + 1, 0, (sockaddr*)&serverAddr, len);

    // TODO IMPROV: free format. Current resrictions value: 1`000`000 - 9`999`999. Version - two digits separated by dot.
    std::string version = "1.0";
    std::string value = "1000000 ";        
    std::string initReq = "INIT "+value+version;

    sendto(sockfd, initReq.c_str(), initReq.size() + 1, 0, (const sockaddr*)&serverAddr, sizeof(serverAddr));

    //TO DO TIMEOUT 3 SEC
    std::this_thread::sleep_for(std::chrono::seconds(3));

    //Loop + timeout //

    size_t totalParts = 0;
    size_t partSize = 0;
    
    const std::chrono::seconds timeoutDuration(5);
    auto startTime = std::chrono::steady_clock::now();

    socklen_t len = sizeof(serverAddr);

    while(partSize == 0 || totalParts == 0)
    {
        std::string request = "REQUEST_ARRAY " + std::to_string(number);
        logger.log("Requested aaray with initial value of " + std::to_string(number), Logger::INFO);
        sendto(sockfd, request.c_str(), request.size() + 1, 0, (const sockaddr*)&serverAddr, sizeof(serverAddr));
        //sendto(sockfd, &number, sizeof(number), 0, (const sockaddr*)&serverAddr, sizeof(serverAddr));

        
        
        recvfrom(sockfd, &totalParts, sizeof(totalParts), 0, (sockaddr*)&serverAddr, &len);
        recvfrom(sockfd, &partSize, sizeof(partSize), 0, (sockaddr*)&serverAddr, &len);
        logger.log("Received metadata totalParts: " + std::to_string(totalParts), Logger::INFO);
        logger.log("Received metadata partSize: " + std::to_string(partSize), Logger::INFO);

        auto currentTime = std::chrono::steady_clock::now();
        if (currentTime - startTime >= timeoutDuration) {
            logger.log("REQUEST TIMEOUT: metada not received", Logger::ERROR);
            //throw runtime_error("REQUEST TIMEOUT: metada not received");
            return;
        }

    }

    std::vector<double> fullArray(totalParts * partSize, 0);
    //std::vector<std::vector<double>> fullArray(totalParts, std::vector<double>(partSize, 0.0));

    std::set<size_t> missingChunks;
    for (size_t i = 0; i < totalParts; ++i) {
        missingChunks.insert(i);
    }

    while (!missingChunks.empty()) {
        for (auto it = missingChunks.begin(); it != missingChunks.end();) {
            size_t chunkIndex = *it;
            requestChunk(sockfd, serverAddr, len, chunkIndex);
            logger.log("Requested chunk: " + std::to_string(chunkIndex), Logger::INFO);

            std::vector<double> chunk(partSize);
            double checksum, calculatedChecksum = 0;
            ssize_t chunkSize = recvfrom(sockfd, chunk.data(), partSize * sizeof(double), 0, (sockaddr*)&serverAddr, &len);
            recvfrom(sockfd, &checksum, sizeof(checksum), 0, (sockaddr*)&serverAddr, &len);

            size_t actualSize = chunkSize / sizeof(double);
            for (size_t i = 0; i < actualSize; ++i) {
                fullArray[chunkIndex * partSize + i] = chunk[i];
                //fullArray[chunkIndex][i] = chunk[i];
                calculatedChecksum += chunk[i];
            }

            if (std::abs(calculatedChecksum - checksum) < 1e-6) {
                logger.log("Received chunk: " + std::to_string(chunkIndex), Logger::INFO);
                it = missingChunks.erase(it);
            } else {
                ++it;
            }
        }

        //std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    sendto(sockfd, "CONFIRM_RECEIPT", strlen("CONFIRM_RECEIPT") + 1, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
    logger.log("Reassembled array received successfully ", Logger::INFO);
    //std::cout << "Reassembled array received successfully.\n";


    std::sort(fullArray.begin(), fullArray.end());
    saveVectorToBinary(fullArray, "binaryArray.bin");

    close(sockfd);
}

void Client::requestChunk(int sockfd, sockaddr_in& serverAddr, socklen_t len, size_t chunkIndex) {
    std::string request = "REQUEST_CHUNK " + std::to_string(chunkIndex);
    sendto(sockfd, request.c_str(), request.size() + 1, 0, (sockaddr*)&serverAddr, len);
    std::cout << "Requested chunk " << chunkIndex << ".\n";
}