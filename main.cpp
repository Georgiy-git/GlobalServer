#include "StartSession.hpp"
#include "loger.hpp"

using namespace boost::asio;

const int port = 53888;

//ЛОГ --------------------------------------------------------------------|
int count_make_user = 0;
int count_del_user = 0;
int count_message = 0;
int count_send_line = 0;
//ЛОГ --------------------------------------------------------------------|

class Connector {
public:
    Connector(io_context& context, ip::tcp::acceptor& acceptor)
        : context{ context }, acceptor{ acceptor }
    {
        std::cout << "Сервер запущен с портом " << port << std::endl;
    }

    ~Connector() {
        make_log("Принято запросов: " + std::to_string(count_message));
        make_log("Отправлено сообщений: " + std::to_string(count_send_line));
        make_log("Создано объектов пользователей: " + std::to_string(count_make_user));
        make_log("Удалено объектов пользователей: " + std::to_string(count_del_user) + "\n");
    }

    void _async_accept() {
        socket = std::make_shared<ip::tcp::socket>(context);
        acceptor.async_accept(*socket, [&](boost::system::error_code ec) {
            if (ec) {
                std::cerr << "Ошибка подключения пользователя: " << ec.message() << std::endl;
                if (socket->is_open()) {
                    socket->close();
                }
            }
            else {
                if (socket->remote_endpoint().address().to_string().empty()) {
                    std::cerr << "Ошибка определения ip подключения\n";
                    if (socket->is_open()) {
                        socket->close();
                    }
                }
                else {
                    //Польза --------------------------------------------------------------------|
                    std::cout << "Новое подключение: " <<
                        socket->remote_endpoint().address().to_string() << std::endl;
                    try {
                        StartSession* session = new StartSession(context, std::move(socket));
                        count_make_user++; //log
                    }
                    //Польза --------------------------------------------------------------------|
                    catch (...) {
                        std::cerr << socket->remote_endpoint().address().to_string() << 
                            ": создать класс пользователя не удалось\n";
                        if (socket->is_open()) {
                            socket->close();
                        }
                    }
                }
            }
            _async_accept(); //Гарант
            });
    }

private:
    io_context& context;
    ip::tcp::acceptor& acceptor;
    std::shared_ptr<ip::tcp::socket> socket;
};

int main() {
    setlocale(LC_ALL, "RU");
    io_context context;
    ip::tcp::acceptor acceptor(context, ip::tcp::endpoint(ip::tcp::v4(), port));
    Connector connector(context, acceptor);
    connector._async_accept();
    context.run();
}