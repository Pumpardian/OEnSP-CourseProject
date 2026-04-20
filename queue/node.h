#pragma once
#include <atomic>

template <typename T>
struct Node {
    T data;
    std::atomic<Node*> next;

    Node() : next(nullptr) {}
    Node(const T& val) : data(val), next(nullptr) {}
};
