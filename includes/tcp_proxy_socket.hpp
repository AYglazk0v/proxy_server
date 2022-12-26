#ifndef TCP_SOCKET_HPP
#define  TCP_SOCKET_HPP

#include <cstddef>
#include <fcntl.h>
#include <memory>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>
#include <unordered_map>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <utility>

#include "connections.hpp"

class ProxyServer;

class TcpProxySocket {
	private:
		friend class ProxyServer;

		int                                 fd_;
		struct sockaddr_in                  src_addr_;
		struct sockaddr_in                  target_addr_;

	public:
		TcpProxySocket() {}
		~TcpProxySocket() {}


		static int Listen(struct sockaddr_in* addr) {
			const int optval = 1;
			int fd = socket(AF_INET, SOCK_STREAM, 0);
			if (fd < 0) {
				throw std::runtime_error("Error in Listen(): socket()");
			}
			if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
				throw std::runtime_error("Error in Listen(): setsockopt()");
			}
			if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
				throw std::runtime_error("Error in Listen(): fcntl()");
			}
			if (bind(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(struct sockaddr_in)) < 0) {
				throw std::runtime_error("Error in Listen(): bind()");
			}
			if (listen(fd, SOMAXCONN) < 0) {
				throw std::runtime_error("Error in Listen(): listen()");
			}
			return fd;
		}

		static int Connect(struct sockaddr_in* addr) {
			const int optval = 1;
			int fd = socket(AF_INET, SOCK_STREAM, 0);
			if (fd < 0) {
				throw std::runtime_error("Error in Connect(): socket()");
			}
			if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
				throw std::runtime_error("Error in Connect(): setsockopt()");
			}
			if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
				throw std::runtime_error("Error in Connect(): fcntl()");
			}
			if (connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(sockaddr_in)) < 0) {
				throw std::runtime_error("Error in Connect(): connect()");
			}
			return fd;
		}

		static int Accept(int fd) {
			struct sockaddr_in  addr;
			socklen_t           socklen = sizeof(addr);

			int ret_fd = accept(fd, reinterpret_cast<struct sockaddr*>(&addr), &socklen);
			if (ret_fd < 0) {
				throw std::runtime_error("Error in Accept(): accept");
			}
			return ret_fd;
		}

		static void Send(info_ptr connection, int fd) {
			while (connection->length > 0) {
				int cnt = send(fd, connection->buffer + connection->offset, connection->length, MSG_NOSIGNAL);
				if (cnt < 0) {
					if (errno == EAGAIN) {
						throw std::runtime_error("Send(): ERNO == EAGAIN");
					} else {
						throw std::runtime_error("Error in Send(): send()");
					}
				} else {
					connection->length -= cnt;
					connection->offset += cnt;
				}
			}
		}

		static void Recv(info_ptr connection, int fd) {
			connection->offset = 0;
			connection->length = 0;
			
			int cnt = recv(fd, connection->buffer, MAX_MSG_PACK, MSG_NOSIGNAL);
			if (cnt < 0) {
				if (errno == EAGAIN) {
					throw std::runtime_error("Recv(): ERNO == EAGAIN");
				} else {
					throw std::runtime_error("Error in Recv(): recv");
				}
			} else if (cnt == 0) {
				throw std::runtime_error("Error in Recv(): recv");
			}
			connection->length = cnt;
		}
};

#endif