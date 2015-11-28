#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <string>
#include <functional>
#include <map>
#include <list>
#include <regex>

#define BUF_SIZE 1024

namespace Logger{

	using string = std::string;

	static bool debug_ = false;

   	static void d(string msg){
		if(debug_)
	   		printf("%s\n",msg.c_str());
   	}

   	static void i(string msg){
   		printf("%s\n",msg.c_str());
   	}

   	static void e(string msg){
   		printf("\033[1;31m%s\033[0m\n",msg.c_str());
   	}
};

namespace Cappuccino{
	
	using string = std::string;

	static string document_root_ = "";

	std::list<string> split(const string& str, const string& delim){

		std::list<string> result;

	    string::size_type pos = 0;

	    while(pos != string::npos ){
	        string::size_type p = str.find(delim, pos);
	        if(p == string::npos){
	            result.push_back(str.substr(pos));
	            break;
	        }else{
	            result.push_back(str.substr(pos, p - pos));
	        }
	        pos = p + delim.size();
	    }
	    return result;
	}

	int hex2int(char hex){
		if(hex >= 'A' && hex <= 'F'){
	        return hex - 'A' + 10;
	    }else if(hex >= 'a' && hex <= 'f'){
	        return hex - 'a' + 10;
	    }else if(hex >= '0' && hex <= '9'){
	        return hex - '0';
	    }
	    return -1;
	}

	char hex2char(char high,char low){
	    char res = hex2int(high);
	    res = res << 4;
	    return res + hex2int(low);
	}

	string url_decode(string str){
	    string res = "";
	    char tmp;
	    auto len = str.size();
	    for(auto i = 0; i < len; i++){
	        if(str[i] == '+'){
	            res += ' ';
	        }else if(str[i] == '%' && (i+2) < len){
	            if((tmp = hex2char((char)str[i+1],(char)str[i+2])) != -1){
	                i += 2;
	                res += tmp;
	            }else{
	                res += '%';
	            }
	        }else{
	            res += str[i];
	        }
	    }
	    return res;
	}

	class Request{
  	  public:
		enum Method{
			GET,
			POST,
			PUT,
			DELETE,
			HEAD,
			OPTION
		};

	  private:
		std::map<string,string> headers_;
		std::map<string,string> params_;
		std::map<string,string> url_params_;

		string url_;
		string protocol_;
		Method method_;		

		void set_method(string m){
			if(m == "GET"){
				method_ = GET;
			}else if(m == "POST"){
				method_ = POST;
			}else if(m == "PUT"){
				method_ = PUT;
			}else if(m == "DELETE"){
				method_ = DELETE;
			}else if(m == "HEAD"){
				method_ = HEAD;
			}else if(m == "OPTION"){
				method_ = OPTION;
			}
		}

	  public:

	  	string protocol() const{
	  		return protocol_;
	  	}
		string url() const{
			return url_;
		}
		Method method() const{
			return method_;
		}
		std::map<string,string> headers() const{
			return headers_;
		}
		std::map<string,string> params() const{
			return params_;
		}
		std::map<string,string> url_params() const{
			return url_params_;
		}

		void add_url_param(string key, string val){			
			url_params_.insert( std::map< string, string>::value_type( key, val));
		}

		Request(string method, string url, string protocol, string request){
			set_method(method);
			url_ = url;
			protocol_ = protocol;

			auto lines = split(request,"\n");

			string name;

			for(auto p : lines){
				auto header_name_val = split(p,": ");
				if(header_name_val.size() == 2){
					name = header_name_val.front();
					header_name_val.pop_front();
					headers_.insert( std::map< string, string>::value_type( name, header_name_val.front()));
				}else{
					auto param_name_val = split(p,"=");
					if(param_name_val.size() == 2){
						name = param_name_val.front();
						param_name_val.pop_front();
						params_.insert( std::map< string, string>::value_type( name, url_decode(param_name_val.front())));
					}
				}
			}
		}
	};

	class Response{
	  public:
		enum Response_Type{
			TEXT,
			FILE
		};

	  protected:
		string response_;
		string filename_;
		bool is_ready_;

		string content_type_;
		string protocol_;
		Response_Type response_type_;
		int status_;

		std::map<string,string> replace_values_;

	  public:

	  	string replace_all(string response) const{
			for(auto value = replace_values_.begin(); value != replace_values_.end(); value++){
			    std::string::size_type pos(response.find(value->first));
			    while( pos != std::string::npos ){
			        response.replace( pos, value->first.length(), value->second );
			        pos = response.find( value->first, pos + value->second.length() );
			    }
			}
	  		return response;
	  	}

		explicit Response(string protocol, Response_Type response_type):
			content_type_("text/html"),
			protocol_(protocol),
			response_type_(response_type),
			status_(200)
			{}

		void add_replace_value(string key, string val){
			replace_values_.insert( std::map<string, string>::value_type( key, val));
		}

		void set_status(int status){
			status_ = status;
		}		

		int status() const{
			return status_;
		}

		string status_str() const{
			switch(status_){
				case 200: return "OK";
				case 404: return "NotFound";

				/* ToDo: Add more status*/
				default: return "OK";
			}
		}
		void set_content_type(string content_type){
			content_type_ = content_type;
		}

		string content_type() const{
			return content_type_;
		}

		void set_filename(std::string filename){
			filename_ = filename;
		}

		void set_text(std::string text){
			response_ = text;
		}
		string header() const{
			return protocol_ + " " + std::to_string(status_) + " "+ status_str() +"\nContent-type: "+content_type_+"\n\n";
		}
		operator string() const{
			int file;
			if(response_type_ == FILE){
				string response = header(); 

				string file_path = document_root_;
				file_path += "/"+ filename_;

				if ((file = open(file_path.c_str(), O_RDONLY)) < 0) {
					Logger::e("No such file or directory \""+ file_path +"\"");
				}else {
					char buf[BUF_SIZE] = "";
					while (read(file, buf, sizeof(buf)) > 0) {
						response += buf;
					}
				}

				return replace_all(response);
			}else{
				return header() + response_;
			}
		}
	};

	class NotFound : public Response{
	  public:
	  	NotFound(string protocol):
	  		Response(protocol, Response_Type::TEXT)
  		{
  			status_ = 404;
  			response_ = "<html>\n<head>\n<title>Not found</title>\n</head>\n<body>\n<h1>Page not found! </h1>\n</body>\n</html>";
  		}
	  	string content_type(){
	  		return "html/text";
	  	}	
	};


	std::map<string, std::function<Response(Request*)>> routes_;

	static int port_ = 1204;
	static bool debug_ = false;
	static int sockfd_ = 0;
	static int sessionfd_ = 0;

	static void document_root(string path){
		document_root_ = path;
	}

	static void load_argument_value(int argc, char *argv[]){
		char result;
		while((result = getopt(argc,argv,"dp:")) != -1){
			switch(result){
			case 'd':
				debug_ = true;
				break;
			case 'p':
				port_ = atoi(optarg);
				break;
			}
		}
		Logger::debug_ = debug_;
		if(debug_){
			Logger::d("mode: debug");
		}else{			
			Logger::d("mode: product");
		}
		Logger::d("port:" +  std::to_string(port_));
	}

	static void init_socket(){
		struct sockaddr_in server;

		if ((sockfd_ = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
			Logger::e("socket create error");
			exit(EXIT_FAILURE);
		}
		
		memset( &server, 0, sizeof(server));
		server.sin_family = AF_INET;	
		server.sin_addr.s_addr = INADDR_ANY;
		server.sin_port = htons(port_);
		
		char opt = 1;
		setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

		if (bind(sockfd_, (struct sockaddr *) &server, sizeof(server)) < 0) {
			Logger::e("socket bind error");
			exit(EXIT_FAILURE);
		}
		
		if (listen(sockfd_, SOMAXCONN) < 0) {
			Logger::e("listen error");
			exit(EXIT_FAILURE);
		}
	}

	static void signal_handler(int SignalName){
		Logger::i("server terminated");		
		close(sessionfd_);
		close(sockfd_);
		exit(0);
		return;
	}

	static void init_signal(int signame){
		if (signal(signame, Cappuccino::signal_handler) == SIG_ERR) {
			Logger::e("signal setting error");
			exit(1);
		}
	}


	static Response create_response(char* method, char* url, char* protocol,char* req){
		Request* request = new Request( string(method), string(url), string(protocol), string(req));
		Logger::d("method:"+string(method)+" url:"+string(url));

		std::regex re( R"(<\w+>)");
	    std::smatch m;    
	    std::list<string> reg;

	    bool correct = true;
		for(auto url = routes_.begin(); url != routes_.end(); url++){
			correct = true;
		    if(url->first.find("<", 0) != string::npos){

			    auto iter = url->first.cbegin();
			    while ( std::regex_search( iter, url->first.cend(), m, re )){
			        reg.push_back(m.str());
			        iter = m[0].second;
			    }

			    if(reg.size() != 0){
			    	auto val = split(url->first, "/");
			    	auto inp = split(request->url(), "/");

			    	if(val.size() != inp.size()) break;

			    	for(auto v : val){
			    		if(find(reg.begin(), reg.end(), v) != reg.end()){
			    			request->add_url_param( v.substr(1, v.size() - 2), url_decode(inp.front()));   
			    		}else if(v != inp.front()){  			
	    					correct = false;
	    				}
			    		inp.pop_front();
			    	}
			    }
			    if(correct){
					Response result = url->second(request);
					delete request;
					return result;			    	
			    }
			}else{
				if(url->first == request->url()){
					Response result = routes_[request->url()](request);
					delete request;
					return result;
				}
			}
		}

		Logger::d("not found!" + string(url));
		Response response = NotFound(request->protocol());
		Logger::d(response);
		return response;
	}

	static string process(){
		char buf[BUF_SIZE] = {};
		char method[BUF_SIZE] = {};
		char url[BUF_SIZE] = {};
		char protocol[BUF_SIZE] = {};

		if (recv(sessionfd_, buf, sizeof(buf), 0) < 0) {
			Logger::e("receive error!");
			exit(EXIT_FAILURE);
		}
		sscanf(buf, "%s %s %s", method, url, protocol);

		bool isbody = false;
		do {
			if (!isbody && strstr(buf, "\r\n")) {
				isbody = true;
			}
			if(strstr(buf, "\r\n")){
				break;
			}
			if (strlen(buf) >= sizeof(buf)) {
				memset(&buf, 0, sizeof(buf));
			}
		} while (recv(sessionfd_, buf+strlen(buf), sizeof(buf) - strlen(buf), 0) > 0);

		Logger::i(url);
		return create_response( method, url, protocol, buf);
	}
	
	static void run(){
		Logger::i(" * Running on http://localhost:" + std::to_string(port_) + "/");
		while (1) {
			struct sockaddr_in client;

			memset( &client, 0, sizeof(client));
			int len = sizeof(client);
			if ((sessionfd_ = accept(sockfd_, (struct sockaddr *) &client,(socklen_t *) &len)) < 0) {
				Logger::e("accept error");
				exit(EXIT_FAILURE);
			}

			string response = process();
			send(sessionfd_, response.c_str(), response.size(), 0);

			close(sessionfd_);
		}
	}

	static void Cappuccino(int argc, char *argv[]) {

		port_ = 1204;
		debug_ = false;

		load_argument_value(argc, argv);
		init_socket();
		init_signal(SIGINT);
	}	

	static void add_route(string route, std::function<Response(Request*)> function){
		routes_.insert( std::map<string,std::function<Response(Request*)>>::value_type(route, function));
	}
};
