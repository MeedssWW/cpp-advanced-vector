# C++ Advanced Vector

Advanced Vector is a custom implementation of a dynamic array container similar to `std::vector`. The project focuses on manual memory management, object lifetime control, move semantics, exception safety, and iterator support.

## Features

- Dynamic storage with size and capacity.
- Manual raw memory allocation.
- Copy and move constructors.
- Copy and move assignment.
- `Reserve`, `Resize`, `PushBack`, `PopBack`, `EmplaceBack`, and `Erase`-style operations.
- Iterator support through raw pointers.
- Object lifetime management with placement new and explicit destruction.
- Tests for copy/move behavior and exception safety.

## Tech Stack

- C++17 / C++20
- Templates
- RAII
- Move semantics
- Placement new
- STL memory algorithms
- Assertions-based tests

## Project Structure

```text
.
|-- vector.h     # RawMemory and Vector implementation
`-- main.cpp     # Tests and usage checks
```

## Build

```bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic main.cpp -o advanced_vector_tests
```

## Run

```bash
./advanced_vector_tests
```

The executable runs a set of assertions that validate the container behavior.

## Example

```cpp
Vector<std::string> words;

words.EmplaceBack("modern");
words.EmplaceBack("cpp");

words.Reserve(16);
words.PushBack("container");
```

## What This Project Demonstrates

- Understanding of low-level C++ object lifetime.
- Separating raw storage management from container behavior.
- Applying RAII to memory management.
- Implementing strong exception safety for container operations.
- Using templates to build generic containers.

## Possible Improvements

- Replace assertion tests with Google Test.
- Add GitHub Actions for compiler checks.
- Add sanitizer builds.
- Add benchmarks against `std::vector` for educational comparison.
- Document iterator invalidation rules.
