#include "NNTPClient.h"
#include <iostream>
#include <stdexcept>
#include <boost/asio/connect.hpp>
#include <boost/asio/streambuf.hpp>

namespace asio = boost::asio;
using asio::ip::tcp;

NNTPClient::NNTPClient(asio::io_service& io_service, asio::ssl::context& ssl_context, bool useSSL)
    : io_service_(io_service),
      socket_(io_service),
      ssl_socket_(io_service, ssl_context),
      useSSL_(useSSL) {}

// Destructor
NNTPClient::~NNTPClient() {
    try {
        sendCommand("QUIT");
    }
    catch (...) {
        // Suppress all exceptions to prevent throwing from destructor
    }

    try {
        if (useSSL_) {
            ssl_socket_.lowest_layer().shutdown(tcp::socket::shutdown_both);
            ssl_socket_.lowest_layer().close();
        }
        else {
            socket_.shutdown(tcp::socket::shutdown_both);
            socket_.close();
        }
    }
    catch (...) {
        // Suppress all exceptions during socket shutdown
    }
}

void NNTPClient::connect(const std::string& server, int port) {
    try {
        tcp::resolver resolver(io_service_);
        tcp::resolver::query query(server, std::to_string(port));
        tcp::resolver::iterator endpoints = resolver.resolve(query);

        if (useSSL_) {
            asio::connect(ssl_socket_.lowest_layer(), endpoints);
            ssl_socket_.handshake(asio::ssl::stream_base::client);
            std::cout << "[NNTPClient] SSL connection established to " << server << ":" << port << std::endl;
        }
        else {
            asio::connect(socket_, endpoints);
            std::cout << "[NNTPClient] Plaintext connection established to " << server << ":" << port << std::endl;
        }

        std::string response = readResponse();
        std::cout << "[NNTPClient] Server Response: " << response << std::endl;
    }
    catch (std::exception& e) {
        throw std::runtime_error("Connection error: " + std::string(e.what()));
    }
}

void NNTPClient::login(const std::string& username, const std::string& password) {
    std::string response = sendCommand("AUTHINFO USER " + username);
    if (response.substr(0, 3) != "381") { // 381 = More authentication information needed
        throw std::runtime_error("AUTHINFO USER failed: " + response);
    }

    response = sendCommand("AUTHINFO PASS " + password);
    if (response.substr(0, 3) != "281") { // 281 = Authentication accepted
        throw std::runtime_error("AUTHINFO PASS failed: " + response);
    }

    std::cout << "[NNTPClient] Authentication successful." << std::endl;
}

bool NNTPClient::checkNZBExists(const std::string& messageID) {
    std::string response = sendCommand("STAT <" + messageID + ">");
    // NNTP "223" status means article exists
    if (response.size() < 3) {
        throw std::runtime_error("Invalid response for STAT command: " + response);
    }
    return response.substr(0, 3) == "223";
}

std::string NNTPClient::sendCommand(const std::string& command) {
    std::string cmd = command + "\r\n";
    try {
        if (useSSL_) {
            asio::write(ssl_socket_, asio::buffer(cmd));
        }
        else {
            asio::write(socket_, asio::buffer(cmd));
        }
    }
    catch (std::exception& e) {
        throw std::runtime_error("Failed to send command '" + command + "': " + e.what());
    }

    return readResponse();
}

std::string NNTPClient::readResponse() {
    asio::streambuf buffer;
    boost::system::error_code error;

    try {
        if (useSSL_) {
            asio::read_until(ssl_socket_, buffer, "\r\n", error);
        }
        else {
            asio::read_until(socket_, buffer, "\r\n", error);
        }

        if (error && error != asio::error::eof) {
            throw boost::system::system_error(error);
        }

        std::istream response_stream(&buffer);
        std::string response_line;
        std::getline(response_stream, response_line);

        if (!response_line.empty() && response_line.back() == '\r') {
            response_line.pop_back();
        }

        return response_line;
    }
    catch (std::exception& e) {
        throw std::runtime_error("Failed to read response: " + std::string(e.what()));
    }
}