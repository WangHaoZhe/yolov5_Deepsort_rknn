#ifndef WEB_IO_H
#define WEB_IO_H

#include <opencv2/opencv.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>

#include "control.h"

typedef websocketpp::server<websocketpp::config::asio> server;

class StreamServer {
public:
    StreamServer(boost::asio::io_context &io_context, uint16_t http_port, uint16_t ws_port)
        : http_acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), http_port)) {
        // Initialize WebSocket server
        ws_server.init_asio(&io_context);
        ws_server.set_open_handler([this](websocketpp::connection_hdl hdl) {
            connections.insert(hdl);
            std::cout << "Client connected via WebSocket!" << std::endl;
        });
        ws_server.set_close_handler([this](websocketpp::connection_hdl hdl) {
            connections.erase(hdl);
            std::cout << "Client disconnected from WebSocket!" << std::endl;
        });
        ws_server.set_message_handler([this](websocketpp::connection_hdl hdl, server::message_ptr msg) {
            try {
                std::string input = msg->get_payload();
                std::cout << "Received input from client: " << input << std::endl;

                // Convert input to integer
                int value = std::stoi(input);
                id = value;

                // Send response back to client
                std::string response = "Present ID: " + std::to_string(value);
                ws_server.send(hdl, response, websocketpp::frame::opcode::text);
            } catch (std::exception &e) {
                std::cerr << "Error processing input: " << e.what() << std::endl;
                ws_server.send(hdl, "Invalid input. Please enter a valid integer.", websocketpp::frame::opcode::text);
            }
        });

        ws_server.clear_access_channels(websocketpp::log::alevel::frame_header);
        ws_server.clear_access_channels(websocketpp::log::alevel::frame_payload);
    }

    void start() {
        // Start WebSocket server
        std::thread ws_thread([this]() {
            ws_server.listen(9002); // WebSocket port
            ws_server.start_accept();
            ws_server.run();
        });
        ws_thread.detach();

        // Start HTTP server
        std::thread http_thread([this]() {
            accept_http_connections();
        });
        http_thread.detach();
    }

    void send_frame(const std::vector<uchar> &frame) {
        for (const auto &hdl: connections) {
            try {
                ws_server.send(hdl, frame.data(), frame.size(), websocketpp::frame::opcode::binary);
            } catch (websocketpp::exception const &e) {
                std::cerr << "Error sending frame: " << e.what() << std::endl;
            }
        }
    }

private:
    void accept_http_connections() {
        while (true) {
            boost::asio::ip::tcp::socket socket(http_acceptor.get_executor());
            http_acceptor.accept(socket);

            std::thread([this, socket = std::move(socket)]() mutable {
                handle_http_request(std::move(socket));
            }).detach();
        }
    }

    void handle_http_request(boost::asio::ip::tcp::socket socket) {
        try {
            // Read HTTP request
            boost::asio::streambuf request;
            boost::asio::read_until(socket, request, "\r\n\r\n");

            // Serve the HTML page
            std::ifstream html_file("../robot/src/index.html");
            if (!html_file) {
                std::cerr << "Error: Cannot find index.html!" << std::endl;
                return;
            }

            std::string html_content((std::istreambuf_iterator<char>(html_file)), std::istreambuf_iterator<char>());
            std::string response =
                    "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(
                        html_content.size()) +
                    "\r\n\r\n" + html_content;

            boost::asio::write(socket, boost::asio::buffer(response));
        } catch (std::exception &e) {
            std::cerr << "HTTP error: " << e.what() << std::endl;
        }
    }

    server ws_server;
    boost::asio::ip::tcp::acceptor http_acceptor;
    std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl> > connections;
};

void webStreamer(int cpuid);

#endif //WEB_IO_H
