#include "StartSession.hpp"

StartSession::StartSession(io_context& context, std::shared_ptr<ip::tcp::socket> socket)
    : context{ context }, socket(socket), input(&buf), output(&buf)
{
    if (socket == nullptr) {
        std::cerr << "Ошибка: сокет является nullptr\n";
    }
}

StartSession::~StartSession()
{
    std::cout << "Программа завершена.";

}

void StartSession::_process_network()
{
    async_read_until(*socket, buf, '\f', [this](error_code ec, size_t bytes) {
        if (ec) {
            if (name.empty()) {
                std::cout << socket->remote_endpoint().address().to_string() << ": ";
            }
            else {
                std::cout << name << ":  ";
            }

            if (ec == error::connection_aborted) {
                std::cout << "Соединение прервано.\n";
            }
            else if (ec == error::eof) {
                std::cout << "Разорвал соединение.\n";
            }
            else {
                std::cout << "Ошибка обработки сети: " << ec.message() << std::endl;
            }
            delete this;
        }
        else {
            //Польза --------------------------------------------------------------------|
            _process_buffer(bytes);
            //Польза --------------------------------------------------------------------|
        }
        });
}

void StartSession::_process_buffer(size_t bytes)
{
    std::string line;
    std::getline(input, line, '\f');
    int command;

    //Получение номера команды
    try {
        command = std::stoi(line.substr(0, line.find('|')));
    }
    catch (...) {
        std::cerr << "Ошибка распознавания команды\n";
        //_close();
        return;
    } 
    //end

    std::cout << "От " << name << " команда " << command << std::endl;

    //Функционал --------------------------------------------------------------------|
    if      (command == 1) { _hello(); }
    else if (command == 2) { _help(); }
    else if (command == 3) { }
    else if (command == 4) { }
    else if (command == 5) { }
    //Функционал --------------------------------------------------------------------|

    //Предохр
    else { _send_line("Сервер: команда не распознана."); }

    _process_network();
}

void StartSession::_send_line(std::string line)
{
    line = "mes|" + line + '\f';
    async_write(*socket, buffer(line.data(), line.size()), [this](error_code ec, size_t bytes) {
        if (ec) {
            std::cerr << "Ошибка отправки данных для " << name << std::endl;
            _close();
            }
        });
}

void StartSession::_close()
{
    delete this;
}

void StartSession::_hello()
{
    _send_line("Привет, " + socket->remote_endpoint().address().to_string() + "\n");
}

void StartSession::_help()
{
    std::string line = "Доступные команды:\n";
    line += "/hello\n";
    line += "/help";
    _send_line(line);
}
