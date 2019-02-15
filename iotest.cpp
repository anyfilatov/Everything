#include <iostream>

int main(int argc, char** argv) {
    //try to change dec to hex
    std::cout.setf(std::ios::hex);
    std::cout << 20 << std::endl;
    //expected 14, but got 20
    
    
    //now try this
    std::cout.setf(std::ios::hex,std::ios::basefield);
    std::cout << 20 << std::endl;
    //now we get 14 as expected
    
    
    //then we expect to change basefield back to dec
    std::cout << 20 << std::endl;
    //but we still get 14
    
    //if we want to change basefield, then flag std::ios::basefield is already set and we don't need to set it again
    std::cout.setf(std::ios::dec);
    std::cout << 20 << std::endl;
    // here we have got 20 as expected
    
    
    //now try manip
    std::cout << std::ios::hex << 20 << std::endl;
    //we expected 14, but get 820
    
    
    std::cout << std::ios::hex << std::endl;
    //here we get 8
    
    
    std::cout << std::hex << 20 << std::endl;
    //and this is a way to get 14
    
    
    std::cout << 20 << std::endl;
    //and still get 14
}