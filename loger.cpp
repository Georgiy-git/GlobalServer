#include "loger.hpp"

void make_log(std::string line)
{
	try {
		static std::fstream file("log_info.txt", std::ios::app);
		file << line << std::endl;
	}
	catch (...) {
		std::cerr << "Îרטבךא ג נאבמעו כמדדונא\n";
	}
}
