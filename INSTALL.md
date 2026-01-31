# Installation

## Build from source:

Install required packages:

### For Debian/Ubuntu:

`sudo apt install gcc libreadline-dev git make cmake`

### For Fedora:

`sudo dnf install gcc readline-devel git make cmake`

Clone the project repository, generate the Makefile and install using make:

```
git clone --depth 1 --branch v3.1.1 https://github.com/a-h-ismail/libtmsolve.git
cd libtmsolve

# Generate the makefile and use it to build the library
cmake -S . -B build; cmake --build build
sudo cmake --install build
```
