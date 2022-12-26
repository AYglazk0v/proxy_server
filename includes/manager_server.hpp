#ifndef MANAGER_SERVER_HPP
# define MANAGER_SERVER_HPP

#include <iostream>
# include <pthread.h>
# include <signal.h>
#include <unistd.h>

# include "proxy_server.hpp"

class ProxyServer;

class ManagerServer {
	private:
		std::vector<ProxyServer*>	servers_;
	
	public:
		~ManagerServer() {
			Clear(true);
		}

		void Handle() {
			while (true) {
				Clear(false);
				usleep(250);
			}
		}

		static void* Routine(void * arg) {
			ProxyServer* curr = reinterpret_cast<ProxyServer*>(arg);
			curr->Loop();
			return nullptr;
		}

		void	Add(const char* src_host, int src_port, const char* remote_host, int remote_port) {
			ProxyServer* curr_serv = new ProxyServer();
			try {
				curr_serv->Setup(src_host, src_port, remote_host, remote_port);
				pthread_create(&(curr_serv->thread_), nullptr, &Routine, curr_serv);
				servers_.push_back(curr_serv);
			} catch (std::exception& e) {
				std::cerr << e.what();
			}
		}

		void Clear(bool force) {
			auto it = servers_.begin();
			while(it != servers_.end()) {
				ProxyServer* curr = *it;
				if (force) {
					pthread_cancel(curr->thread_);
					pthread_join(curr->thread_, nullptr);
					delete curr;
					it = servers_.erase(it);
				} else {
					int ret = pthread_kill(curr->thread_, 0);
					if (ret != 0) {
						pthread_join(curr->thread_, nullptr);
						delete curr;
						it = servers_.erase(it);
					} else {
						it++;
					}
				}
			}
		}
		
};
#endif