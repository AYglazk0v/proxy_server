#ifndef PROXY_SERVER_HPP
# define PROXY_SERVER_HPP

# include <vector>
# include <arpa/inet.h>
# include <sys/epoll.h>
# include <unordered_map>

# include "logger.hpp"
# include "tcp_proxy_socket.hpp"
# include "connections.hpp"

class TcpProxySocket;
class ProxyServer {
	private:
		static constexpr size_t MAX_COUNT_EVENTS = 1024;
		friend class ManagerServer;

		pthread_t							thread_;
		int									master_socket_;
		int                                 epoll_fd_;
		std::unordered_map<int, info_ptr>   connections_;
		Logger								logger_;
		TcpProxySocket						socket_;
		
	public:
		~ProxyServer() {
			close(master_socket_);
			close(epoll_fd_);
			for (auto it = connections_.begin(), ite = connections_.end(); it != ite; ++it) {
				info_ptr curr = (*it).second;
				close(curr->src_fd);
				close(curr->targer_fd);
			}
		}

		void Setup(const char* src_host, int src_port, const char* remote_host, int remote_port) {			
			socket_.src_addr_.sin_family = AF_INET;
			socket_.src_addr_.sin_port = htons(src_port);
			if (src_host == nullptr) {
				socket_.src_addr_.sin_addr.s_addr = htonl(INADDR_ANY);
			} else if (inet_pton(AF_INET, src_host, &socket_.src_addr_.sin_addr) <= 0) {
				throw std::runtime_error("Error: Setup(): inet_pton(SRC_HOST)");
			}

			socket_.target_addr_.sin_family = AF_INET;
			socket_.target_addr_.sin_port = htons(remote_port);
			if (inet_pton(AF_INET, remote_host, &socket_.target_addr_.sin_addr) <= 0) {
				throw std::runtime_error("Error: Setup(): inet_pton(REMOTE_HOST)");
			}
			try {
				master_socket_ = socket_.Listen(&socket_.src_addr_);
				epoll_fd_ = epoll_create1(0);
				struct epoll_event ev;
				ev.events = EPOLLIN;
				ev.data.fd = master_socket_;
				if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, master_socket_, &ev) < 0) {
					throw std::runtime_error("Error: Setup(): epoll_ctl");
				}
			} catch (std::exception& e) {
				throw std::runtime_error(e.what());
			}
		}

		void CloseConnect(int fd) {
			if (connections_.count(fd) == 1) {
				int fd_pair = GetSecondDescriptor(fd);
				close(fd);
				close(fd_pair);
				connections_.erase(fd);
				connections_.erase(fd_pair);
				logger_.AddToLog("Close sockets : " + std::to_string(fd) + " and " + std::to_string(fd_pair));
			}
		} 

		void EpollOut(int send_fd) {
			logger_.AddToLog("EpollOut: fd = " + std::to_string(send_fd));
			if (connections_.count(send_fd) == 1) {
				info_ptr curr_info = connections_.find(send_fd)->second;
				try { //send
					socket_.Send(curr_info, send_fd);
					logger_.AddToLog("The Send() function was executed successfully. Socket fd = " + std::to_string(send_fd) + " buffer: " + curr_info->buffer);
					struct epoll_event ev;
					ev.data.fd = send_fd;
					ev.events = EPOLLIN | EPOLLOUT;
					epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, send_fd, &ev);
				} catch (std::exception& e) {
					logger_.AddToLog(e.what());
					if (static_cast<std::string>(e.what()) == "Send(): ERNO == EAGAIN") {
						return;
					}
					CloseConnect(send_fd);
				}
			}
		}

		void EpollIn(int recv_fd) {
			logger_.AddToLog("EpollIN: fd = " + std::to_string(recv_fd));
			if (connections_.count(recv_fd) == 1) {
				info_ptr curr_info = connections_.find(recv_fd)->second;
				int send_fd = GetSecondDescriptor(recv_fd);
				if (curr_info->length != 0) { 
					//wait buffer to empty
					return;
				}
				try { //recv
					socket_.Recv(curr_info, recv_fd);
					logger_.AddToLog("The Recv function was executed successfully. Socket fd = " + std::to_string(recv_fd) + " buffer: " + curr_info->buffer);
				} catch (std::exception& e) {
					logger_.AddToLog(e.what());
					if (static_cast<std::string>(e.what()) == "Recv(): ERNO == EAGAIN") {
						return;
					}
					CloseConnect(recv_fd);
				}
				try { //send
					socket_.Send(curr_info, send_fd);
					logger_.AddToLog("The Send() function was executed successfully. Socket fd = " + std::to_string(send_fd) + " buffer: " + curr_info->buffer);
				} catch (std::exception& e) {
					logger_.AddToLog(e.what());
					if (static_cast<std::string>(e.what()) == "Send(): ERNO == EAGAIN") {
						struct epoll_event ev;
						ev.data.fd = send_fd;
						ev.events = EPOLLIN | EPOLLOUT;
						epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, send_fd, &ev);
						return ;
					}
					CloseConnect(send_fd);
				}
			}
		}

		void SetInfoConnection(int fd) {
			int src_fd = socket_.Accept(fd);
			logger_.AddToLog("The Accept function was executed successfully. Socket fd = " + std::to_string(src_fd));
			int targer_fd = socket_.Connect(&socket_.target_addr_);
			logger_.AddToLog("The Connect function was executed successfully. Socket fd = " + std::to_string(targer_fd));
			info_ptr tmp_connection = std::make_shared<info_connection>(info_connection{src_fd, targer_fd});
			connections_.insert(std::pair<int, info_ptr>(src_fd, tmp_connection));
			connections_.insert(std::pair<int, info_ptr>(targer_fd, tmp_connection));

			struct epoll_event ev;
			ev.data.fd = tmp_connection->src_fd;
			ev.events = EPOLLIN;
			if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, tmp_connection->src_fd, &ev) < 0) {
				throw std::runtime_error("Error on SetInfoConnection(): epoll_ctl(), fd=" + std::to_string(src_fd));
			}
			logger_.AddToLog("The epoll_ctl function was executed successfully. Socket fd = " + std::to_string(src_fd));
			ev.data.fd = tmp_connection->targer_fd;
			ev.events = EPOLLIN;
			if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, tmp_connection->targer_fd, &ev) < 0) {
				throw std::runtime_error("Error on SetInfoConnection(): epoll_ctl(), fd=" + std::to_string(targer_fd));
			}
			logger_.AddToLog("The epoll_ctl function was executed successfully. Socket fd = " + std::to_string(targer_fd));
		} 

		int GetSecondDescriptor(int fd){
			info_ptr curr_info = connections_.find(fd)->second;
			if (curr_info->src_fd == fd) {
				return curr_info->targer_fd;
			} else {
				return curr_info->src_fd;
			}
		}

		void Loop() {
			struct epoll_event events[MAX_COUNT_EVENTS];
			int count;
			int i;

			while (true) {
				count = epoll_wait(epoll_fd_, events, MAX_COUNT_EVENTS, -1);
				if (count < 0) {
					if (errno == EINTR) {
						logger_.AddToLog("Loop(): erno == EINTR");
						continue;
					} else {
						logger_.AddToLog("Error Loop(): epoll_wait()");
						continue;
					}
				}
				for (i = 0; i < count; ++i) {
					try{
						if (events[i].data.fd == master_socket_) {
							SetInfoConnection(master_socket_);
						} else {
							if (events[i].events & EPOLLOUT) {
								EpollOut(events[i].data.fd);
							}
							if (events[i].events & EPOLLIN) {
								EpollIn(events[i].data.fd);
							}
						}
					} catch (std::exception& e) {
						logger_.AddToLog(e.what());
					}
				}
			}
			logger_.AddToLog("Server stop");
		}

}; //ProxyServer

#endif