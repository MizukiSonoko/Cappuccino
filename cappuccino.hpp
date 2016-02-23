#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <string>
#include <functional>
#include <unordered_map>
#include <list>
#include <algorithm>
#include <iostream>
#include <sstream>

#if defined(__APPLE__) || defined(__GNUC__) && __GNUC__ * 10  + __GNUC_MINOR__ >= 49
#include <regex>
#endif

#include <vector>
#include <fstream>
#include <future>

#define BUF_SIZE 4096
#define MAX_LISTEN 128



namespace Cappuccino{
	
	using string = std::string;

	struct Context
	{
		int port_{ 1204 };
		int sockfd_{ 0 };
		int sessionfd_{ 0 };
	    fd_set mask1fds, mask2fds;

		string view_root_{ "" };
		string static_root_{ "public" };

	};

	namespace signal_utils{
		static void signal_handler(int SignalName){
			Logger::i("\nserver terminated!\n");		
			close(sessionfd_);
			close(sockfd_);
			exit(0);
			return;
		}

		static void signal_handler_child(int SignalName){
			while(waitpid(-1,NULL,WNOHANG)>0){}
	        signal(SIGCHLD, Cappuccino::signal_utils::signal_handler_child);
		}

		static void init_signal(){
			if (signal(SIGINT, Cappuccino::signal_utils::signal_handler) == SIG_ERR) {
				Logger::e("signal setting error");
				exit(1);
			}
			if (signal(SIGCHLD, Cappuccino::signal_utils::signal_handler_child) == SIG_ERR) {
				Logger::e("signal setting error");
				exit(1);
			}
		}
	}

	static void set_argument_value(int argc, char *argv[]) noexcept{
		char result;
		while((result = getopt(argc,argv,"dp:")) != -1){
			switch(result){
			case 'd':
				Logger::debug_ = true;
				break;
			case 'p':
				port_ = atoi(optarg);
				break;
			}
		}
		if(Logger::debug_){
			Logger::i("mode: debug");
		}else{			
			Logger::i("mode: product");
		}
		Logger::d("port:" + std::to_string(port_));
	}

	static void init_socket(){
	    struct sockaddr_in server;
		if ((sockfd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			Logger::e("socket create error");
			exit(EXIT_FAILURE);
		}
		memset( &server, 0, sizeof(server));
		server.sin_family = AF_INET;	
		server.sin_addr.s_addr = INADDR_ANY;
		server.sin_port = htons(port_);

		char opt = 1;
		setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

		int temp = 1;
  		if(setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
	            &temp, sizeof(temp))){
		    Logger::e("setsockopt() failed");
		}
		
		if (bind(sockfd_, (struct sockaddr *) &server, sizeof(server)) < 0) {
			Logger::e("socket bind error");
			exit(EXIT_FAILURE);
		}

		if(listen(sockfd_,  MAX_LISTEN) < 0) {
			Logger::e("listen error");
			exit(EXIT_FAILURE);
		}

	    FD_ZERO(&context->mask1fds);
	    FD_SET(sockfd_, &context->mask1fds);
	}

	namespace Regex{
		std::vector<string> findParent(string text) noexcept{
			std::vector<string> res;
			string tmp = "";
			bool isIn = false;
			for(int i = 0; i< text.size(); ++i){
				if(text[i] == '<'){
					isIn = true;
					continue;
				}
				if(text[i] == '>'){
					isIn = false;
					res.push_back("<"+tmp+">");
					tmp = "";
				}

				if(isIn){
					tmp += text[i];
				}
			}
			return res;
		}
	}

#if defined(__APPLE__) || defined(__GNUC__) && __GNUC__ * 10  + __GNUC_MINOR__ >= 49	
	std::regex re( R"(<\w+>)");
    std::smatch m;
#endif    

	static Response create_response(char* req) noexcept{
		std::unique_ptr<Request> request = Request().factory(string(req));

		auto pos(routes_.find(request->url()));
		if( pos != routes_.end()){
			auto res = pos->second(request.get());
			//Logger::d("-------");
			//Logger::d(res);
			return res;
		}		
		// static
		pos = static_routes_.find(request->url());
		if( pos != static_routes_.end()){
			return pos->second(request.get());
		}		

		auto posir = other_routes_.find(404);
		if( posir != other_routes_.end()){
			return posir->second(request.get());
		}
		return Response();
	}

	static string receive_process(int sessionfd){
		char buf[BUF_SIZE] = {};
		char method[BUF_SIZE] = {};
		char url[BUF_SIZE] = {};
		char protocol[BUF_SIZE] = {};

		if (recv(sessionfd, buf, sizeof(buf), 0) < 0) {
			Logger::e("receive error!");
			exit(EXIT_FAILURE);
		}
		bool isbody = false;
		do{
			if (!isbody && strstr(buf, "\r\n")) {
				isbody = true;
			}
			if(strstr(buf, "\r\n")){
				break;
			}
			if (strlen(buf) >= sizeof(buf)) {
				memset(&buf, 0, sizeof(buf));
			}
		} while (read(sessionfd, buf+strlen(buf), sizeof(buf) - strlen(buf)) > 0);
		return create_response(buf);
	}
	
	time_t client_info[FD_SETSIZE];

	void load(string directory,string filename) noexcept{
		if(filename == "." || filename == "..") return;

		if(filename!="")
			directory += "/" + filename;
		DIR* dir = opendir(directory.c_str());
		if(dir!=NULL){
			struct dirent* dent;
	        dent = readdir(dir);
		    while(dent!=NULL){
		        dent = readdir(dir);
		        if(dent!=NULL)
			        load(directory, string(dent->d_name));
		    }
		    closedir(dir);
		}else{
			Logger::i(directory);
			auto static_file = Cappuccino::FileLoader(directory);
			static_file.preload();
			static_routes_.insert( make_pair(
				"/" + directory, 
					[static_file](Request* request) -> Response{
						return Cappuccino::ResponseBuilder(request)
							.status(200,"OK")
							.file(static_file)
							.build();
					}
				));
		}
	}

	void load_static_files() noexcept{
		load(static_root_,"");
	}

	void run(){
		init_socket();
		signal_utils::init_signal();
		load_static_files();

		Logger::i("Running on http://localhost:" + std::to_string(port_) + "/");

	    int cd[FD_SETSIZE];
		struct sockaddr_in client;
        int    fd;
        struct timeval tv;

	    for(int i = 0;i < FD_SETSIZE; i++){
	        cd[i] = 0;
	    }

	    while(1) {
	        tv.tv_sec = 0;
	        tv.tv_usec = 0;

	        memcpy(&mask2fds, &context->mask1fds, sizeof(context->mask1fds));

	        int select_result = select(FD_SETSIZE, &mask2fds, (fd_set *)0, (fd_set *)0, &tv);
	        if(select_result < 1) {
	            for(fd = 0; fd < FD_SETSIZE; fd++) {
	                if(cd[fd] == 1) {
	                    close(fd);
	                    FD_CLR(fd, &context->mask1fds);
	                    cd[fd] = 0;
	                }
	            }
	            continue;
	        }
	        for(fd = 0; fd < FD_SETSIZE; fd++){
	            if(FD_ISSET(fd,&mask2fds)) {
	                if(fd == sockfd_) {
	                	memset( &client, 0, sizeof(client));
						int len = sizeof(client);
	                    int clientfd = accept(sockfd_, 
	                        (struct sockaddr *)&client,(socklen_t *) &len);
	                        FD_SET(clientfd, &context->mask1fds);
	                }else {
	                    if(cd[fd] == 1) {
	                        close(fd);
	                        FD_CLR(fd, &context->mask1fds);
	                        cd[fd] = 0;
	                    } else {
							string response = receive_process(fd);
							write(fd, response.c_str(), response.size());
	                        cd[fd] = 1;
	                    }
	                }
	            }
	        }
	    }	    
	}

	void Cappuccino(int argc, char *argv[]) {
		port_ = 1204;
		set_argument_value(argc, argv);
	}
};

namespace Cocoa{
	class App{};

};

