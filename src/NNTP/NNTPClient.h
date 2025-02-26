#ifndef NNTPCLIENT_H
#define NNTPCLIENT_H

#include <string>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

class NNTPClient {
public:
    /**
     * @brief Constructs an NNTPClient instance.
     *
     * @param io_service Reference to Boost.Asio io_service.
     * @param ssl_context Reference to Boost.Asio SSL context.
     * @param useSSL Boolean indicating whether to use SSL.
     */
    NNTPClient(boost::asio::io_service& io_service, boost::asio::ssl::context& ssl_context, bool useSSL);

    /**
     * @brief Destructor. Sends QUIT command and closes the connection.
     */
    ~NNTPClient();

    /**
     * @brief Connects to the NNTP server.
     *
     * @param server The server address.
     * @param port The server port.
     * @throws std::runtime_error if connection fails.
     */
    void connect(const std::string& server, int port);

    /**
     * @brief Authenticates with the NNTP server using provided credentials.
     *
     * @param username The username.
     * @param password The password.
     * @throws std::runtime_error if authentication fails.
     */
    void login(const std::string& username, const std::string& password);

    /**
     * @brief Checks if an NZB article exists on the NNTP server.
     *
     * @param messageID The Message-ID of the article.
     * @return true if the article exists.
     * @return false otherwise.
     * @throws std::runtime_error if the command fails.
     */
    bool checkNZBExists(const std::string& messageID);

private:
    /**
     * @brief Sends a command to the NNTP server and returns the response.
     *
     * @param command The command string.
     * @return std::string The server's response.
     * @throws std::runtime_error if sending or reading fails.
     */
    std::string sendCommand(const std::string& command);

    /**
     * @brief Reads a single line response from the server.
     *
     * @return std::string The response line.
     * @throws std::runtime_error if reading fails.
     */
    std::string readResponse();

    boost::asio::io_service& io_service_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket_;
    bool useSSL_;
};

#endif // NNTPCLIENT_H