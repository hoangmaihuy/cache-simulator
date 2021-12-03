# cache-simulator

1. 解压和编译模拟器，本项目使用 C++17，编译时需要 `g++` `cmake` `make`

   ```bash
   tar -xvf cache-simulator
   cd cache-simulator
   cmake CMakeLists.txt
   make
   ```

2. cache-simulator 使用方法

   ```bash
   Usage: cache-simulator [options] trace-path
   
   Positional arguments:
   trace-path   	Path to trace file
   
   Optional arguments:
   -h --help    	shows help message and exits [default: false]
   -v --version 	prints version information and exits [default: false]
   --verbose    	Verbose mode [default: false]
   --optimized  	Use optimized config [default: false]
   --iter       	Trace iteration count [default: 10]
   
   # Example
   # cache-simulator --iter 20 --optimized test.trace
   ```