#include "../includes/proxy_server.hpp"

ProxyServer::ProxyServer(int argc, char** argv) {
	try {
		InputValidation(argc, argv);
		SettingСontextAddr();
	} catch (std::exception& e) {
		loger_.AddToLog("ERROR START SERVER : " + static_cast<std::string>(e.what()));
		exit(EXIT_FAILURE);
	}
}

void ProxyServer::SettingСontextAddr() {
	local_addr_.sin_family = AF_INET;
	local_addr_.sin_addr.s_addr = inet_addr(local_ip_.c_str());
	local_addr_.sin_port = htons(local_port_);

	remote_addr_.sin_family = AF_INET;
	remote_addr_.sin_addr.s_addr = inet_addr(local_ip_.c_str());
	remote_addr_.sin_port = htons(local_port_);
}

void ProxyServer::InputValidation(int argc, char** argv) {
	if (argc != 3) {
		throw std::invalid_argument("Error input argumets. It is necessary to submit:\n proxy_port remote_host remote_port");
	}

	std::string tmp_port = argv[1];
	if (!std::all_of(tmp_port.begin(), tmp_port.end(), [](const char& c){return std::isdigit(c);})) {
		throw std::invalid_argument("Error input argumets. Local port is't digital");
	}
	local_port_ = std::atoi(tmp_port.c_str());

	remote_ip_ = argv[2];
	if (remote_ip_ == "localhost") {
		remote_ip_ = "127.0.0.1";
	} else if (inet_addr(remote_ip_.c_str()) == INADDR_NONE) {
		throw std::invalid_argument("Error input argumets. Wrong remote ip");
	}

	tmp_port = argv[3];
	if (!std::all_of(tmp_port.begin(), tmp_port.end(), [](const char& c){return std::isdigit(c);})) {
		throw std::invalid_argument("Error input argumets. Remote port is't digital");
	}
	remote_port_ = std::atoi(tmp_port.c_str());
}