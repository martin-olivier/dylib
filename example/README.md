# dylib example

Here is an example about the usage of the `dylib` library in a project.

The functions and variables of our future dynamic library are located inside [lib.cpp](lib.cpp)

The code that will load the functions and global variables of our dynamic library at runtime is located inside [main.cpp](main.cpp)

Then, our build system, located inside [CMakeLists.txt](CMakeLists.txt), is performing the following tasks:

- Fetch `dylib` into the project
- Build [lib.cpp](lib.cpp) into a dynamic library
- Build [main.cpp](main.cpp) into an executable and link it with `dylib`

Let's build our code:
> Make sure to type the following commands inside the `example` folder

```sh
cmake . -B build
cmake --build build
```

Let's run our code:

```sh
# on unix, run the following command inside "build" folder
./dylib_example

# on windows, run the following command inside "build/Debug" folder
./dylib_example.exe
```

You will have the following result:

```sh
Hello World!
dylib - v3.0.0
pi value: 3.14159
magic value: cafebabe
10 + 10 = 20
string = abcdef
vector:
- abc
- def
- ghi
- jkl
```
