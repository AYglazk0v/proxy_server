#ifndef CONNECTIONS_HPP
# define CONNECTIONS_HPP

# include <cstddef>
# include <memory>

static constexpr size_t MAX_MSG_PACK = 1024;

typedef struct {
	int		src_fd;
	int		targer_fd;
	size_t	length;
	size_t	offset;
	char	buffer[MAX_MSG_PACK];
} info_connection;

using info_ptr = std::shared_ptr<info_connection>;

#endif