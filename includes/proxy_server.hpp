#ifndef PROXY_SERVER_HPP
# define PROXY_SERVER_HPP

# include <vector>
# include <memory>
# include <cstdlib>
# include <sys/poll.h>
# include <sys/epoll.h>
# include <fcntl.h>
# include <unordered_map>
# include <algorithm>
# include <sys/socket.h>

# include "logger.hpp"
# include "user.hpp"

# define MAX_LISTEN 1024

using ptr_User = std::shared_ptr<User>;

class ProxyServer {
	private:
		static constexpr size_t MAX_EVENTS = 1024;
		
		int									fd_epoll_;
		struct epoll_event	 				events_[MAX_EVENTS];
		
		Logger								logger_;

		std::unordered_map<int, ptr_User>	users_;
		std::vector<int>					users_close_;

		int									local_sd_;
		
		std::string							local_ip_ = "127.0.0.1";
		std::string							remote_ip_;
		int									local_port_;
		int									remote_port_;
		sockaddr_in							local_addr_{};
		sockaddr_in							remote_addr_{};
	
	private:
		void createEpollFd(int fd);
		void EpollInServer();
		void InputValidation(int argc, char** argv);
		void SettingContextAddr();
		void CreateSocketAndStart();
		void initEpoll();

	public:
		ProxyServer() = delete;
		ProxyServer(int argc, char** argv);
		~ProxyServer();
		void Loop();
}; //ProxyServer

#endif