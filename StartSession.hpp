#pragma once

#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <thread>

using namespace boost::asio;
using error_code = boost::system::error_code;

class StartSession
{
public:
    StartSession(io_context& context, std::shared_ptr<ip::tcp::socket> socket);
    ~StartSession();

    void _process_network();
    void _process_buffer(size_t bytes);
    void _send_line(std::string);
    void _close();
    void _get_name();

    //Функционал --------------------------------------------------------------------|
    void _hello();
    void _help();
    //Функционал --------------------------------------------------------------------|

private:
    io_context& context;
    std::shared_ptr<ip::tcp::socket> socket;
    std::string name = "noname";
    std::mutex mutex;
    streambuf buf;
    std::istream input;
    std::ostream output;
    std::string remote_ip;
};