#include "NNTP/NNTPClient.h"
#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <json/json.h>
#include "pugixml.hpp"

namespace asio = boost::asio;
using asio::ip::tcp;

class ThreadSafeQueue {
public:
    void push(const std::string& articleId) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(articleId);
        cond_var_.notify_one();
    }

    bool try_pop(std::string& articleId) {
        std::unique_lock<std::mutex> lock(mutex_);
        while (queue_.empty() && !finished_) {
            cond_var_.wait(lock);
        }
        if (queue_.empty())
            return false;
        articleId = queue_.front();
        queue_.pop();
        return true;
    }

    void set_finished() {
        std::lock_guard<std::mutex> lock(mutex_);
        finished_ = true;
        cond_var_.notify_all();
    }

private:
    std::queue<std::string> queue_;
    std::mutex mutex_;
    std::condition_variable cond_var_;
    bool finished_ = false;
};


void writeReport(std::ofstream& reportFile, const std::string& server, int total, int available) {
    double availability = (total > 0) ? (static_cast<double>(available) / total) * 100.0 : 0.0;

    reportFile << "———————————————————————\n"
               << "Hostname: " << server << "\n"
               << "Total Articles: " << total << "\n"
               << "Available Articles: " << available << "\n"
               << "Availability: " << availability << "%\n"
               << "_____________________________________________\n\n";
}

// Function to process a single server configuration
void processServer(const Json::Value& serverConfig, const pugi::xml_node& rootNode,
                  std::ofstream& reportFile, asio::io_service& io_service,
                  asio::ssl::context& ssl_context, std::mutex& reportMutex) {
    // Extract server configuration
    std::string server = serverConfig["server"].asString();
    int port = serverConfig["port"].asInt();
    bool useSSL = serverConfig["ssl"].asBool();
    std::string username = serverConfig.get("username", "").asString();
    std::string password = serverConfig.get("password", "").asString();
    int maxConnections = serverConfig.get("connections", 4).asInt(); // Default to 4

    // Initialize the thread-safe queue
    ThreadSafeQueue articleQueue;

    // Extract all article IDs from XML and populate the queue
    for (pugi::xml_node fileNode = rootNode.child("file"); fileNode; fileNode = fileNode.next_sibling("file")) {
        for (pugi::xml_node segmentNode = fileNode.child("segments").child("segment"); segmentNode;
             segmentNode = segmentNode.next_sibling("segment")) {
            std::string articleId = segmentNode.child_value();
            if (!articleId.empty()) {
                articleQueue.push(articleId);
            }
        }
    }

    // Signal that all articles have been enqueued
    articleQueue.set_finished();

    // Atomic counters for tracking progress
    std::atomic<int> totalArticles(0);
    std::atomic<int> availableArticles(0);

    // Mutex for synchronized error logging and report writing
    std::mutex errorMutex;

    // Worker lambda function
    auto worker = [&](int threadId) {
        try {
            // Instantiate a separate NNTPClient for each thread
            NNTPClient client(io_service, ssl_context, useSSL);
            client.connect(server, port);

            // Authenticate if credentials are provided
            if (!username.empty()) {
                client.login(username, password);
            }

            std::string articleId;
            while (articleQueue.try_pop(articleId)) {
                totalArticles++;
                if (client.checkNZBExists(articleId)) {
                    availableArticles++;
                }
            }
        }
        catch (const std::exception& e) {
            std::lock_guard<std::mutex> lock(errorMutex);
            std::cerr << "[Error] Exception in thread " << threadId
                      << " for server " << server << ": " << e.what() << std::endl;
        }
    };

    // Launch worker threads
    std::vector<std::thread> threads;
    for (int i = 0; i < maxConnections; ++i) {
        threads.emplace_back(worker, i + 1);
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
        if (thread.joinable())
            thread.join();
    }

    // Write the final report with thread synchronization
    {
        std::lock_guard<std::mutex> lock(reportMutex);
        writeReport(reportFile, server, totalArticles.load(), availableArticles.load());
    }

    std::cout << "[Info] Completed processing server: " << server
              << " - Total Articles: " << totalArticles
              << ", Available: " << availableArticles << std::endl;
}

int main(int argc, char* argv[]) {
    // Check if NZB file path is provided
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_nzb_file>" << std::endl;
        return 1;
    }

    // Retrieve NZB file path from the first command-line argument
    std::string nzbPath = argv[1];

    // Optional: You can add more command-line arguments for config path or report file path
    // For simplicity, we'll assume config.json is in the current directory

    // Initialize Boost.Asio and SSL context
    asio::io_service io_service;
    asio::ssl::context ssl_context(asio::ssl::context::sslv23);
    ssl_context.set_default_verify_paths();

    // Open the report file
    std::ofstream reportFile("nzb_report.txt");
    if (!reportFile.is_open()) {
        std::cerr << "[Error] Failed to open report file." << std::endl;
        return 1;
    }

    // Load and parse your JSON configuration
    // Example using JsonCpp
    std::ifstream configFile("config.json");
    if (!configFile.is_open()) {
        std::cerr << "[Error] Failed to open config.json." << std::endl;
        return 1;
    }

    Json::Value config;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    if (!Json::parseFromStream(readerBuilder, configFile, &config, &errs)) {
        std::cerr << "[Error] Failed to parse config.json: " << errs << std::endl;
        return 1;
    }

    // Load and parse the NZB XML
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(nzbPath.c_str());
    if (!result) {
        std::cerr << "[Error] Failed to load NZB file '" << nzbPath << "': " << result.description() << std::endl;
        return 1;
    }

    pugi::xml_node rootNode = doc.child("nzb");
    if (!rootNode) {
        std::cerr << "[Error] Invalid NZB file format." << std::endl;
        return 1;
    }

    // Mutex for synchronized report writing
    std::mutex reportMutex;

    // Iterate through each server configuration and process
    std::vector<std::thread> serverThreads;
    for (const auto& serverConfig : config["servers"]) {
        serverThreads.emplace_back(processServer, serverConfig, rootNode, std::ref(reportFile),
                                   std::ref(io_service), std::ref(ssl_context), std::ref(reportMutex));
    }

    // Wait for all server processing threads to complete
    for (auto& thread : serverThreads) {
        if (thread.joinable())
            thread.join();
    }

    // Close the report file
    reportFile.close();

    std::cout << "[Info] Processing complete. Report saved to nzb_report.txt" << std::endl;
    return 0;
}