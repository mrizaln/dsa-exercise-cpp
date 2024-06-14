# dsa-exercise-cpp

My attempt at implementing various Data Structure and Algorithms as an exercise. The code written here is adapted to current best practices in C++ and uses modern C++ features including but not limited to STL `iterator` support (also `range` support if possible).

> see [reference](./reference) for list of references I used for this exercise

```cpp
namespace rv = std::views;

dsa::ArrayList<int> list{ 40 };

// populate list ...

// ArrayList has a random access iterator, thus it's possible to do this
for (auto& value : list | rv::reverse | rv::take(10)) {
    // process the value ...
}
```

## TODO

- [ ] Make the DSA implementations free of UB even if it sacrifices performance
