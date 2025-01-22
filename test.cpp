#include <iostream>
#include <vector>

std::vector<int*> createPointerVector() {
    std::vector<int*> vec; // 创建一个 vector 存储指针
    for (int i = 1; i <= 5; ++i) {
        int* ptr = new int(i); // 动态分配整数
        vec.push_back(ptr); // 将指针添加到 vector 中
    }
    return vec; // 返回 vector
}

int main() {
    std::vector<int*> result = createPointerVector(); // 调用函数

    // 输出 vector 中的指针所指向的值
    for (int* ptr : result) {
        std::cout << *ptr << " "; // 解引用指针以获取值
    }
    std::cout << std::endl;

    // 释放动态分配的内存
    for (int* ptr : result) {
        delete ptr; // 释放每个指针指向的内存
    }

    return 0;
}
