#include "StartSession.hpp"

extern int count_del_user;
extern int count_message;
extern int count_send_line;

StartSession::StartSession(io_context& context, std::shared_ptr<ip::tcp::socket> socket)
    : context{ context }, socket(socket), input(&buf), output(&buf)
{
    if (socket == nullptr) {
        std::cerr << "Ошибка: сокет является nullptr\n";
    }
    remote_ip = socket->remote_endpoint().address().to_string();
    _get_name();
}

StartSession::~StartSession()
{
    count_del_user++; //log
}

void StartSession::_process_network()
{
    async_read_until(*socket, buf, '\f', [this](error_code ec, size_t bytes) {
        if (ec) {
            if (name.empty()) {
                std::cout << remote_ip << ": ";
            }
            else {
                std::cout << name << ":  ";
            }

            if (ec == error::connection_aborted) {
                std::cout << "Соединение прервано.\n";
            }
            else if (ec == error::eof || ec == error::connection_reset) {
                std::cout << "отключился.\n";
            }
            else {
                std::cout << "Ошибка обработки сети: " << ec.message() << std::endl;
            }
            //Гарант 
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

    auto s = line.find('}');
    if (s == std::string::npos) {
        _send_line("Сервер: команда не распознана.");
        return;
    }

    //Получение номера команды
    try {
        command = std::stoi(line.substr(line.find('{') + 1, s));
    }
    catch (...) {
        std::cerr << "Ошибка распознавания команды от " << name << std::endl;
        _send_line("Ошибка распознавания команды.");
        return;
    } 
    //end

    line = line.substr(s + 1);
    count_message++; //log

    std::cout << "От " << name << " команда " << command << std::endl;

    //Функционал --------------------------------------------------------------------|
    if      (command == 1) { _hello(); }
    else if (command == 2) { _help(); }
    else if (command == 3) { _files(); }
    else if (command == 4) { _load_file(line); }
    else if (command == 5) { }
    //Функционал --------------------------------------------------------------------|
    
    //Остановка сервера
    else if (command == 127001 && 
        (remote_ip == "176.59.52.248" || remote_ip == "127.0.0.1")) { _stop(); }

    //Предохр
    else { _send_line("Сервер: команда не распознана."); }
}

void StartSession::_send_line(std::string line)
{
    line = "{mes}" + line + '\f';
    async_write(*socket, buffer(line.data(), line.size()), [this](error_code ec, size_t bytes) {
        if (ec) {
            std::cerr << "Ошибка отправки данных для " << name << std::endl;
            _close();
            return;
            }
        count_send_line++; //log
        _process_network();
        });
}

void StartSession::_close()
{
    delete this;
}

void StartSession::_get_name()
{
    auto error_name = [this] { std::cerr << remote_ip << ": ошибка при получении имени.\n"; delete this; };
    
    async_write(*socket, buffer("{login}Введите Ваше имя: \f"), [this, error_name](error_code ec, size_t bytes) {
        if (ec) { error_name(); return; }
        async_read_until(*socket, buf, '\f', [this, error_name](error_code ec, size_t bytes) {
            if (ec) { error_name(); return; }
            //Польза --------------------------------------------------------------------|
            std::getline(input, name, '\f');
            if (name.empty()) { name = "noname"; }
            std::cout << remote_ip << " установил имя " << name << std::endl;
            std::string line = "{mes}Сервер: установлено имя " + name + '\f';
            async_write(*socket, buffer(line.data(), line.size()),
                [this, error_name](error_code ec, size_t bytes){ 
                    if (ec) { error_name(); return; }
                    _process_network(); 
                });
            //Польза --------------------------------------------------------------------|
            });
        });
}

void StartSession::_hello()
{
    _send_line("Привет, " + remote_ip);
}

void StartSession::_help()
{
    std::string line = "Доступные команды:\n";
    line += "/hello\n";
    line += "/help";
    _send_line(line);
}

void StartSession::_stop()
{
    count_message--;
    std::cout << "Сервер остановлен.\n";
    count_del_user++;
    context.stop();
}

void StartSession::_files()
{
    std::string line = "Файлы на сервере:";
    std::filesystem::path path;
    try {
        path = std::filesystem::current_path();
    }
    catch (...) {
        _send_line("Ошибка фаловой системы, файлы не могут быть отображены.");
        return;
    }
    for (const auto& i : std::filesystem::directory_iterator(path)) {
        line += '\n' + i.path().filename().string();
    }
    _send_line(line);
}

void StartSession::_load_file(std::string file_name)
{
    std::ifstream file(file_name.c_str(), std::ios::binary);
    if (!file.is_open()) {
        _send_line("Не удалось найти или открыть файл.");
        return;
    }
    error_code _ec;
    size_t file_size;
    try {
        file_size = std::filesystem::file_size(std::filesystem::path(file_name), _ec);
    }
    catch (...) {
        _send_line("Не удалось вычислить размер файла.");
        return;
    }
    if (_ec) {
        _send_line("Не удалось вычислить размер файла.");
        return;
    }

    output << file.rdbuf();
    std::string line = "{loadfile}" + file_name + '|' + std::to_string(file_size) + '\f';

    auto error = [this] { std::cerr << "Ошибка отправки файла для " + name; _close(); };
    
    //1 --------------------------------------------------------------------|
    async_write(*socket, buffer(line.data(), line.size()), [this, error](error_code ec, size_t bytes) {
        if (ec) { error(); return; }

    //3 --------------------------------------------------------------------|
    async_write(*socket, buf, [this, error](error_code ec, size_t bytes) {
        if (ec) { error(); return; }
        _process_network();

    }); //3
    }); //1
}
