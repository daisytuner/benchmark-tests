#include <iostream>

__attribute__((noinline))
bool foo() {
    return false;
}

int main(int argc, char** argv) {
    std::cout << "Simple plugin test" << std::endl;

    if (foo()) {
        std::cout << "foo() was replaced by plugin!" << std::endl;
        return 0;
    } else {
        std::cout << "foo() was NOT replaced by plugin!" << std::endl;
        return 1;
    }
}
