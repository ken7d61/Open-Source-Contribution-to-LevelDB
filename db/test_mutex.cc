#include <iostream>
#include <mutex>
#include <thread>
int main() {
    std::mutex m;
    std::lock_guard<std::mutex> lock(m);
    std::cout << "Success" << std::endl;
    return 0;
}
