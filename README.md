# VirtualDisplayUtil
The uitl class to add a virtual display without needing a display dummy plug.

## Preliminary
Install pre-requisite tools and libs before compilation.
```
$ sudo apt install g++ libx11-dev libxrandr-dev
```

## Build
Open a terminal and execute `make` under this project src.
```
$ make
g++ -c virtual_display_util.cpp -o virtual_display_util.o
g++ -g virtual_display_util.o virtual_display_util_test.cpp -I. -lX11 -lXrandr -o virtual_display_util_test
rm -f virtual_display_util.o
<<<<<< virtual_display_util_test is created successfully! >>>>>>
```
Congratulation if you see these outputs!

## Quick test
This project has also provide a script for quick test named `run_test`. Here are the steps about how to use it:

1. Given execution permission to the script.
```
$ chmod +x run_test
```

2. Execute the script for a quick test.
```
$ ./run_test
```

It will try to add a virtual display, wait for your click, then log some useful information about your system environment.

*Hope you enjoy it! Any feedbacks are welcome. :)*
