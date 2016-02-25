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
	
	struct {
		int port_{ 1204 };
		int sockfd_{ 0 };
		int sessionfd_{ 0 };
	    fd_set mask1fds, mask2fds;

		std::shared_ptr<std::string> view_root;
		std::shared_ptr<std::string> static_root;

	} context;


	namespace signal_utils{

		void signal_handler(int signal){
			close(context.sessionfd_);
			close(context.sockfd_);
			exit(0);
		}

		static void signal_handler_child(int SignalName){
			while(waitpid(-1,NULL,WNOHANG)>0){}
	        signal(SIGCHLD, signal_utils::signal_handler_child);
		}

		void init_signal(){
			if (signal(SIGINT, signal_utils::signal_handler) == SIG_ERR) {
				exit(1);
			}
			if (signal(SIGCHLD, signal_utils::signal_handler_child) == SIG_ERR) {
				exit(1);
			}
		}
	}


	void init_socket(){
	    struct sockaddr_in server;
		if ((context.sockfd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			exit(EXIT_FAILURE);
		}
		memset( &server, 0, sizeof(server));
		server.sin_family = AF_INET;	
		server.sin_addr.s_addr = INADDR_ANY;
		server.sin_port = htons(context.port_);

		char opt = 1;
		setsockopt(context.sockfd_, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

		int temp = 1;
  		if(setsockopt(context.sockfd_, SOL_SOCKET, SO_REUSEADDR,
	            &temp, sizeof(temp))){
		}
		
		if (bind(context.sockfd_, (struct sockaddr *) &server, sizeof(server)) < 0) {
			exit(EXIT_FAILURE);
		}

		if(listen(context.sockfd_,  MAX_LISTEN) < 0) {
			exit(EXIT_FAILURE);
		}

	    FD_ZERO(&context.mask1fds);
	    FD_SET(context.sockfd_, &context.mask1fds);
	}


	using namespace std;
	using string = string;

	void option(int argc, char *argv[]) noexcept{
		char result;
		while((result = getopt(argc,argv,"dp:")) != -1){
			switch(result){
			case 'd':
				break;
			case 'p':
				context.port_ = atoi(optarg);
				break;
			}
		}
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
    class Response{
      public:
      	operator string(){
      		return "";
      	}
    };

	Response create_response(char* req) noexcept{
		return Response();
	}

	string receive_process(int sessionfd){
		char buf[BUF_SIZE] = {};
		char method[BUF_SIZE] = {};
		char url[BUF_SIZE] = {};
		char protocol[BUF_SIZE] = {};

		if (recv(sessionfd, buf, sizeof(buf), 0) < 0) {
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

	void load(string directory, string filename) noexcept{
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
			/*
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
			*/
		}
	}

	void load_static_files() noexcept{
		load(*context.static_root,"");
	}

	void run(){

		context.view_root = make_shared<string>("");		
		context.static_root = make_shared<string>("public");

		init_socket();
		signal_utils::init_signal();
		load_static_files();


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

	        memcpy(&context.mask2fds, &context.mask1fds, sizeof(context.mask1fds));

	        int select_result = select(FD_SETSIZE, &context.mask2fds, (fd_set *)0, (fd_set *)0, &tv);
	        if(select_result < 1) {
	            for(fd = 0; fd < FD_SETSIZE; fd++) {
	                if(cd[fd] == 1) {
	                    close(fd);
	                    FD_CLR(fd, &context.mask1fds);
	                    cd[fd] = 0;
	                }
	            }
	            continue;
	        }
	        for(fd = 0; fd < FD_SETSIZE; fd++){
	            if(FD_ISSET(fd,&context.mask2fds)) {
	                if(fd == context.sockfd_) {
	                	memset( &client, 0, sizeof(client));
						int len = sizeof(client);
	                    int clientfd = accept(context.sockfd_, (struct sockaddr *)&client,(socklen_t *) &len);
                        FD_SET(clientfd, &context.mask1fds);
	                }else {
	                    if(cd[fd] == 1) {
	                        close(fd);
	                        FD_CLR(fd, &context.mask1fds);
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
		context.port_ = 1204;
		option(argc, argv);
	}
};

namespace Cocoa{
	class App{};
};

