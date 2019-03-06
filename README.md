# Cappuccino
### ~ご注文はCappuccinoですか？~

[![Build Status](https://travis-ci.org/MizukiSonoko/Cappuccino.svg?branch=master)](https://travis-ci.org/MizukiSonoko/Cappuccino)
[![Build Status](https://travis-ci.org/MizukiSonoko/Cappuccino.svg?branch=develop)](https://travis-ci.org/MizukiSonoko/Cappuccino)

Tiny HTTP server library.

# version
```
0.5.1
```

# Micro sample
```shell
$ git clone https://github.com/MizukiSonoko/Cappuccino.git
$ cd Cappuccino
$ cd samples; make
$ ./chino
```

# Spec

```shell
$ ab -k -c 10 -n 10000 http://127.0.0.1:1204/
This is ApacheBench, Version 2.3 <$Revision: 1843412 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 127.0.0.1 (be patient)
Completed 1000 requests
Completed 2000 requests
Completed 3000 requests
Completed 4000 requests
Completed 5000 requests
Completed 6000 requests
Completed 7000 requests
Completed 8000 requests
Completed 9000 requests
Completed 10000 requests
Finished 10000 requests


Server Software:        Cappuccino
Server Hostname:        127.0.0.1
Server Port:            1204

Document Path:          /
Document Length:        1 bytes

Concurrency Level:      10
Time taken for tests:   1.621 seconds
Complete requests:      10000
Failed requests:        0
Keep-Alive requests:    0
Total transferred:      910000 bytes
HTML transferred:       10000 bytes
Requests per second:    6168.26 [#/sec] (mean)
Time per request:       1.621 [ms] (mean)
Time per request:       0.162 [ms] (mean, across all concurrent requests)
Transfer rate:          548.16 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.2      0       3
Processing:     0    1  12.9      1     411
Waiting:        0    0   8.2      0     410
Total:          0    2  13.0      1     411

Percentage of the requests served within a certain time (ms)
  50%      1
  66%      1
  75%      1
  80%      1
  90%      2
  95%      2
  98%      2
  99%      3
 100%    411 (longest request)
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
#include "../cappuccino.hpp"
#include <json.hpp>

using json = nlohmann::json;
using Response = Cappuccino::Response;
using Request = Cappuccino::Request;

int main(int argc, char *argv[]){
	Cappuccino::Cappuccino(argc, argv);

	Cappuccino::templates("html");
	Cappuccino::publics("public");

	Cappuccino::route<Method::GET>("/",[](std::shared_ptr<Request> request) -> Response{
		auto res =  Response(request);
		res.file("index.html");
		return res;
	});

	Cappuccino::run();

	return 0;
}
```

### 3. compile
```shell
clang++ -std=c++14 -Wall app.cpp -o app
```
### 4. run it
```shell
$ ./chino
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
