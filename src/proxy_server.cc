#include "../includes/proxy_server.hpp"
#include <stdexcept>

ProxyServer::ProxyServer(int argc, char** argv) {
	try {
		InputValidation(argc, argv);
		SettingContextAddr();
		CreateSocketAndStart();
		initEpoll();
		logger_.AddToLog("Server started");
	} catch (std::exception& e) {
		logger_.AddToLog("ERROR START SERVER : " + static_cast<std::string>(e.what()));
		exit(EXIT_FAILURE);
	}
}

void ProxyServer::CreateSocketAndStart() {
	if ((local_sd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		throw std::runtime_error("Error: failed to create socket");
	}
	const int on = 1;
	if (setsockopt(local_sd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
		throw std::runtime_error("Error: failed to setsockopt socket");
	}
	if (fcntl(local_sd_, F_SETFL, O_NONBLOCK) < 0) {
		throw std::runtime_error("Error: failed to fcntl socket");
	}
	if (bind(local_sd_, reinterpret_cast<struct sockaddr*>(&local_addr_), sizeof(local_addr_)) < 0) {
		throw std::runtime_error("Error: failed to bind socket");
	}
	if (listen(local_sd_, MAX_LISTEN) < 0) {
		throw std::runtime_error("Error: failed to listen socket");
	}
}

void ProxyServer::initEpoll() {
	struct epoll_event Event;
	fd_epoll_ = epoll_create1(0);
	Event.data.fd = local_sd_;
	Event.events = EPOLLIN;
	epoll_ctl(fd_epoll_, EPOLL_CTL_ADD, local_sd_, &Event);
}

void ProxyServer::SettingContextAddr() {
	local_addr_.sin_family = AF_INET;
	local_addr_.sin_addr.s_addr = inet_addr(local_ip_.c_str());
	local_addr_.sin_port = htons(local_port_);

	remote_addr_.sin_family = AF_INET;
	remote_addr_.sin_addr.s_addr = inet_addr(remote_ip_.c_str());
	remote_addr_.sin_port = htons(remote_port_);
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

void ProxyServer::createEpollFd(int fd) {
	struct epoll_event Event;
	Event.data.fd = fd;
	Event.events = EPOLLIN;
	if (epoll_ctl(fd_epoll_, EPOLL_CTL_ADD, fd, &Event) < 0) {
		throw std::runtime_error("Error epoll_ctl");
	}
}

void ProxyServer::EpollInServer() {
	logger_.AddToLog("Epoll in serv: " + std::to_string(local_sd_));

	sockaddr_in	addr;
	socklen_t addr_len = sizeof(addr);

	int user_fd = accept(local_sd_, reinterpret_cast<struct sockaddr*>(&addr), &addr_len);
	if (user_fd < 0) {
		logger_.AddToLog("ERROR ADD NEW USER: failed to execute accept");
	} else {
		
		if (fcntl(user_fd, F_SETFL, O_NONBLOCK) < 0) {
			logger_.AddToLog("ERROR ADD NEW USER: failed to execute fcntl");
			close(user_fd);
			return;
		}
		
		char buff[16];
		inet_ntop(AF_INET, &addr.sin_addr, buff, sizeof(buff));
		logger_.AddToLog("NEW USER CONNECT: " + std::to_string(user_fd) + "from : " + buff);
		
		try {
			ptr_User user = std::make_shared<User>(User(user_fd, addr, remote_addr_));	
			createEpollFd(user_fd);
			createEpollFd(user->GetRemoteFd());
			users_.insert(std::pair<int, ptr_User>(user_fd, user));
			users_.insert(std::pair<int, ptr_User>(user->GetRemoteFd(), user));
			logger_.AddToLog("NEW USER fd: " + std::to_string(user_fd) + "NEW remote fd: " + std::to_string(user->GetRemoteFd()));
		} catch (std::exception& e) {
			close(user_fd);
			logger_.AddToLog("Error: " + static_cast<std::string>(e.what()));
			return;
		}
	}
}



void ProxyServer::Loop() {
	while (true) {
		int count_epoll = epoll_wait(fd_epoll_, events_, MAX_EVENTS, 1000);
		if (count_epoll < 0) {
			logger_.AddToLog("count_epoll < 0");
			continue;
		}
		if (count_epoll == 0) {
			continue;
		}
		for (unsigned int i = 0; i < count_epoll; ++i) {
			if (events_[i].data.fd == local_sd_) {
				EpollInServer();
			}
		}
	}
}