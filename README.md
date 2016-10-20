# Cappuccino
### ~ご注文はCappuccinoですか？~

[![Build Status](https://travis-ci.org/MizukiSonoko/Cappuccino.svg?branch=master)](https://travis-ci.org/MizukiSonoko/Cappuccino)
[![Build Status](https://travis-ci.org/MizukiSonoko/Cappuccino.svg?branch=develop)](https://travis-ci.org/MizukiSonoko/Cappuccino)

Tiny HTTP server library.

[Demo web page](http://cappuccino.mizuki.io/)

# version
```
0.1.0
```

# Micro sample
```shell
$ git clone https://github.com/MizukiSonoko/Cappuccino.git
$ cd Cappuccino
$ git clone https://github.com/nlohmann/json lib/json
$ cd samples; make
$ ./chino
```

# Environments

### Language
- C++14

### Compiler

- clang version 3.4~
- gcc   version 5.0~


### Dependics 
- [Json](https://github.com/nlohmann/json)

# Usage

#### 1. include "cappuccino.hpp"
#### 2. write code
```cpp
#include "cappuccino.hpp"

int main(int argc, char *argv[]) {
	// initialize
	Cappuccino::Cappuccino(argc, argv);

	// set document_root
	Cappuccino::templates("html");
	Cappuccino::publics("public");
	
	// add function
	Cappuccino::route("/",[](std::shared_ptr<Request> request) -> Response{
		auto res =  Response(request);
		res.file("index.html");
		return res;
	});
	
	// runnning
	Cappuccino::run();
	
	return 0;
}
```

### 3. compile
```shell
clang++ -std=c++0x -Wall app.cpp -o app
```
### 4. runnig
```shell
$ ./app
```

# Directory
```
├── html
│   └── index.html
├── public
│   └── css
│       └── sample.css
└── chino.cpp
```

# LICENCE
MIT

