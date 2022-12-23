#ifndef USER_HPP
# define USER_HPP

# include <iostream>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <sys/stat.h>

class User {
	private:
		int				user_fd_;
		int				retmote_fd_;
		sockaddr_in		user_addr_;
		std::string		request_user_;
		std::string		request_server_;

	public:
		User() = delete;
		~User(){}
		User(const int user_fd, const sockaddr_in& addr, sockaddr_in& remote_addr);

	public:
		const int GetUserFd() const					{ return user_fd_; }
		const int GetRemoteFd() const				{ return retmote_fd_; }
		const std::string &GetRequestUser() const	{ return request_user_; }
		const std::string &GetRequestServer() const	{ return request_server_; }
		
		void RecvRequestUser(const char* buffer, const size_t nbytes)	{ request_user_.append(buffer, nbytes); }
		void RecvRequestServer(const char* buffer, const size_t nbytes)	{ request_server_.append(buffer, nbytes); }
		void UpdateRequestUser(const size_t result)		{ request_user_ = request_user_.substr(result, request_user_.size()); }
		void UpdateRequestServer(const size_t result)	{ request_user_ = request_user_.substr(result, request_server_.size()); }

}; //User

#endif