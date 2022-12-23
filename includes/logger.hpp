#ifndef LOGGER_HPP
# define LOGGER_HPP

# include <fstream>
# include <iostream>
# include <ctime>

class Logger {
	private:
		std::string		file_name_;

	private:
		const std::string CurrentDate() const {
			std::time_t now = std::time(nullptr);
			std::string datetime(20,0);
			datetime.resize(std::strftime(&datetime[0], datetime.size(), "%Y:%m:%d_%X", std::localtime(&now)));
			return datetime;
		}

	public:
		Logger() {
			file_name_ = "proxy_server[" + CurrentDate() + "].log";
		}

		~Logger(){}

		void AddToLog(const std::string& log) const {
			std::string tmp = "[" + CurrentDate() + "] : " + log;
			std::cout << tmp << std::endl;
			std::ofstream out(file_name_.c_str(), std::ios::app);
			if (!out) {
				std::cerr << "Error open file : " << file_name_ << std::endl;
				return;
			}
			out << tmp << std::endl;
			out.close();
		}

}; //Logger

#endif