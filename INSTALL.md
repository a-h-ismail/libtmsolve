# Installation

## Build from source:

Install required packages:

### For Debian/Ubuntu:

`sudo apt install gcc libreadline-dev git make cmake`

### For Fedora:

`sudo dnf install gcc readline-devel git make cmake`

Clone the project repository, generate the Makefile and install using make:

```
git clone --depth 1 https://gitlab.com/a-h-ismail/libtmsolve
cd libtmsolve
cmake -S. -B./build -G "Unix Makefiles"
cd build
make
sudo make install
```
