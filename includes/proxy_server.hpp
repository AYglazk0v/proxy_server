#ifndef PROXY_SERVER_HPP
# define PROXY_SERVER_HPP

# include <vector>
# include <cstdlib>
# include <poll.h>
# include <fcntl.h>
# include <algorithm>
# include <sys/socket.h>

# include "logger.hpp"
# include "user.hpp"

# define MAX_LISTEN 1024

class ProxyServer {
	private:
		std::vector<struct pollfd>	fds_;
		Logger						logger_;

		int							local_sd_;

		std::string					local_ip_ = "127.0.0.1";
		std::string					remote_ip_;
		int							local_port_;
		int							remote_port_;
		sockaddr_in					local_addr_{};
		sockaddr_in					remote_addr_{};
	
	private:
		void InputValidation(int argc, char** argv);
		void SettingContextAddr();
		void CreateSocketAndStart();

	public:
		ProxyServer() = delete;
		ProxyServer(int argc, char** argv);
		~ProxyServer();
		void Loop();
}; //ProxyServer

#endif