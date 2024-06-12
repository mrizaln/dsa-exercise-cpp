# dsa-exercise-cpp

My attempt at implementing various Data Structure and Algorithms as an exercise. The reference I used for this exercise is [Data Structures and Algorithm Analysis in C++ 3rd Edition by Clifford A Shaffer](https://people.cs.vt.edu/shaffer/Book/). The code written here is not an exact copy-and-paste from the book, but adapted to current best practices in C++ and uses modern C++ features including but not limited to STL `iterator` support (also `range` support if possible).

```cpp
namespace rv = std::views;

dsa::ArrayList<int> list{ 40 };

// populate list ...

// ArrayList has a random access iterator, thus it's possible to do this
for (auto& value : list | rv::reverse | rv::take(10)) {
    // process the value ...
}
```

Thus, this exercise also aims to hone my ability in creating an STL-compliant containers in case I needed to create one in the future.

## TODO

- [ ] Make the DSA implementations free of UB even if it sacrifices performance
