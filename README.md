# Cappuccino
[![Build Status](https://travis-ci.org/MizukiSonoko/Cappuccino.svg?branch=master)](https://travis-ci.org/MizukiSonoko/Cappuccino)

Tiny HTTP server library.

[Demo web page](http://cappuccino.mizuki.io/)

# version
```
0.3
```

# Micro sample
```shell
$ git clone https://github.com/MizukiSonoko/Cappuccino.git
$ cd Cappuccino
$ make
$ ./sample
```

# Environments

- clang version 3.4

# Usage

#### 1. include "cappuccino.h"
#### 2. write code
```cpp
#include "cappuccino.h"

int main(int argc, char *argv[]) {
	// initialize
	Cappuccino::Cappuccino(argc, argv);

	// set static files root
	Cappuccino::add_static_root("public");

	// set document_root
	Cappuccino::add_view_root("html");

	// add function
	Cappuccino::add_route("/", [](Cappuccino::Request* req) -> Cappuccino::Response{
		return Cappuccino::ResponseBuilder(req)
					.status(200,"OK")
					.file("index.html")
					.build();
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
└── sample.cpp
```

# LICENCE
MIT

