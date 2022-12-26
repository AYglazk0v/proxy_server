#include "../includes/proxy_server.hpp"
#include <asm-generic/errno-base.h>
#include <cerrno>
#include <cstddef>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <sys/socket.h>

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

ProxyServer::~ProxyServer() {
	for (auto it = users_.begin(), ite = users_.end(); it != ite; ++it) {
		close((*it).first);
	}
	logger_.AddToLog("Server stop");
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
	fd_epoll_ = epoll_create1(0);
	if (fd_epoll_ < 0 ) {
		throw std::runtime_error("Error: erpoll_create1");
	}
	Event.data.fd = local_sd_;
	Event.events = POLLIN | POLLOUT | POLLERR | POLLHUP;
	// Event.events = EPOLLIN;
	if (epoll_ctl(fd_epoll_, EPOLL_CTL_ADD, local_sd_, &Event) < 0) {
		throw std::runtime_error("Error: erpoll_ctl");
	}
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
	if (argc != 4) {
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
	Event.events = POLLIN;
	Event.data.fd = fd;
	if (epoll_ctl(fd_epoll_, EPOLL_CTL_ADD, fd, &Event) < 0) {
		throw std::runtime_error("Error epoll_ctl ");
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
		logger_.AddToLog("NEW USER CONNECT: " + std::to_string(user_fd) + " from : " + buff);
	
		try {
			ptr_User user = std::make_shared<User>(User(user_fd, addr, remote_addr_));	
			createEpollFd(user->GetUserFd());
			createEpollFd(user->GetRemoteFd());
			users_.insert(std::pair<int, ptr_User>(user_fd, user));
			users_.insert(std::pair<int, ptr_User>(user->GetRemoteFd(), user));
			logger_.AddToLog("NEW USER fd: " + std::to_string(user_fd) + " NEW remote fd: " + std::to_string(user->GetRemoteFd()));
		} catch (std::exception& e) {
			close(user_fd);
			logger_.AddToLog("Error: " + static_cast<std::string>(e.what()));
			return;
		}
	}
}

void ProxyServer::EpollInUser(epoll_event& curr_evnt) {
	logger_.AddToLog("Epoll in User: " + std::to_string(curr_evnt.data.fd));
	
	ptr_User us = users_.find(curr_evnt.data.fd)->second;

	char buffer[MAX_BUFFER_RECV];
	int cnt_bytes = recv(curr_evnt.data.fd, buffer, MAX_BUFFER_RECV - 1, MSG_NOSIGNAL);

	//recv
	logger_.AddToLog("RECV: " + std::to_string(cnt_bytes));
	if (cnt_bytes < 0) {
		logger_.AddToLog("Error read from : " + std::to_string(curr_evnt.data.fd));
		users_close_.push_back(us->GetUserFd());
		users_close_.push_back((us->GetRemoteFd()));
	} else if (cnt_bytes == 0) {
		logger_.AddToLog("Client ended session from : " + std::to_string(curr_evnt.data.fd));	
		users_close_.push_back(us->GetUserFd());
		users_close_.push_back((us->GetRemoteFd()));
	} else {
		if (curr_evnt.data.fd == us->GetUserFd()) {
			us->RecvRequestUser(buffer, cnt_bytes);
			logger_.AddToLog("RecvRequestUser :" + us->GetRequestUser());
		} else {
			us->RecvRequestServer(buffer, cnt_bytes);
			logger_.AddToLog("RecvRequestServer : " + us->GetRequestServer());
		}
	}

	//send
	int send_cnt_bytes;
	int send_fd;
	if (curr_evnt.data.fd == us->GetUserFd()) {
		send_fd = us->GetUserFd();
		send_cnt_bytes = send(us->GetRemoteFd(), us->GetRequestUser().c_str(), us->GetRequestUser().length(), MSG_NOSIGNAL);
	} else {
		send_fd = us->GetRemoteFd();
		send_cnt_bytes = send(us->GetUserFd(), us->GetRequestServer().c_str(), us->GetRequestServer().length(), MSG_NOSIGNAL);
	}
	if (send_cnt_bytes == 0) {
		Event.data.fd = send_fd;
		Event.events = EPOLLIN | EPOLLOUT;
		if (epoll_ctl(fd_epoll_, EPOLL_CTL_MOD, send_fd, &Event) < 0) {
			logger_.AddToLog("Error POLLOUT: epoll_ctl MOD");
			return;
		}
	}
	if (curr_evnt.data.fd == us->GetUserFd()) {
		us->UpdateRequestUser(cnt_bytes);
	} else {
		us->UpdateRequestServer(cnt_bytes);
	}
	

	Event.events = POLLIN | POLLOUT;
	epoll_ctl(fd_epoll_, EPOLL_CTL_MOD, curr_evnt.data.fd, &Event);
}

void ProxyServer::EpollOut(epoll_event& curr_evnt) {
	logger_.AddToLog("Epoll Out : " + std::to_string(curr_evnt.data.fd));
	
	ptr_User us = users_.find(curr_evnt.data.fd)->second;
	
	int cnt_bytes;
	if (curr_evnt.data.fd == us->GetUserFd()) {
		cnt_bytes = send(us->GetRemoteFd(), us->GetRequestUser().c_str(), us->GetRequestUser().length(), MSG_NOSIGNAL);
	} else {
		cnt_bytes = send(us->GetUserFd(), us->GetRequestServer().c_str(), us->GetRequestServer().length(), MSG_NOSIGNAL);
	}
	if (cnt_bytes == 0) {
		//keep watch EPOLLOUT
	}else if (cnt_bytes < 0){
		logger_.AddToLog("Error send from : " + std::to_string(curr_evnt.data.fd));
		users_close_.push_back(us->GetUserFd());
		users_close_.push_back((us->GetRemoteFd()));
		return;
	}
	if (curr_evnt.data.fd == us->GetUserFd()) {
		us->UpdateRequestUser(cnt_bytes);
	} else {
		us->UpdateRequestServer(cnt_bytes);
	}
	Event.data.fd = curr_evnt.data.fd;
	Event.events = EPOLLIN;
	if (epoll_ctl(fd_epoll_, EPOLL_CTL_MOD, curr_evnt.data.fd, &Event) < 0) {
		logger_.AddToLog("Error POLLOUT: epoll_ctl MOD");
		return;
	}
}


void ProxyServer::EpollElse(epoll_event& curr_evnt) {
	if (curr_evnt.events & POLLERR) {
		logger_.AddToLog("EPoll POLLER : " + std::to_string(curr_evnt.data.fd));
	} else {
		logger_.AddToLog("EPoll POLLHUP : " + std::to_string(curr_evnt.data.fd));
	}
	ptr_User us = users_.find(curr_evnt.data.fd)->second;
	users_close_.push_back(us->GetUserFd());
	users_close_.push_back((us->GetRemoteFd()));
}

void ProxyServer::CloseConnection() {
	for (auto it = users_close_.begin(), ite = users_close_.end(); it != ite; ++it) {
		users_.erase(*it);
		close(*it);
	}
	users_close_.clear();
}

void ProxyServer::Loop() {
	while (true) {
		int count_epoll = epoll_wait(fd_epoll_, events_, MAX_EVENTS, -1);
		if (count_epoll < 0) {
			if (errno == EINTR) {
				continue;
			} else {
				logger_.AddToLog("Error: epoll wait");
			}
		}
		std::cout << count_epoll << std::endl;
		for (unsigned int i = 0; i < count_epoll; ++i) {
			if (events_[i].events & POLLIN && events_[i].data.fd == local_sd_) {
				EpollInServer();
			} else {
				if (events_[i].events & POLLOUT) {
					EpollOut(events_[i]);
				}
				if (events_[i].events & POLLIN) {
					EpollInUser(events_[i]);
				}
				if (events_[i].events & POLLERR || events_[i].events & POLLHUP) {
					EpollElse(events_[i]);
				}
			}
		}
		CloseConnection();
	}
}