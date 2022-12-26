

<h1 align="center">
📥 Simple Proxy Server📤
</h1>

## 💡 О проекте:

> _Цель проекта -- написать простой tcp server [TCP_server ->Proxy App->TCP_client] с возможностью логирования  всех пакетов, проходящих через него._

		Сервер реализован на сокетах Беркли в неблокирующем режиме.
		Для мультиплексирования использовался epoll.



## 🛠 Тестирование и использование:

	# Клонируйте проект и получите доступ к папке
	git clone git@github.com:AYglazk0v/proxy_server.git && cd proxy_server/

	# Для сборки proxy_server'a необходимо выполнить команду:
	make
	
	# Для тестирования понадобится три экземпляра терминала:
	1) ./proxy_server
	2) cd ./test_echo_server/ && make && ./echo_server 5555
	3) nc localhost 12345
		**Ожидаемый результат**
		1) В терминале 1 будет наблюдаться логирование всех действий на proxy сервере,
			а также будет создан .log файл с этими же данными.
		2) Терминал 2 будет поддерживать echo_server
		3) Весь введенный текст будет пересылаться обратно.
	
	# Для того, чтобы пересобрать проект:
	make re
	
	# Для того, чтобы удалить объектные файлы:
	make clean
	
	# Для того, чтобы полностью очистить систему после тестирования:
	make fclean
		
