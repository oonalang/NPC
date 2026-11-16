#include "../src/main/jni/Security/Lease.h"
#include <fstream>
#include <iostream>
#include <iterator>
int main(int argc,char** argv) {
    if(argc!=7) return 2;
    std::ifstream file(argv[1]);std::string pem{std::istreambuf_iterator<char>(file),{}};
    const license::Request request{argv[3],argv[4],argv[5],std::stoull(argv[6]),100000,1};
    std::string payload,signature;std::getline(std::cin,payload);std::getline(std::cin,signature);
    license::Claims claims;
    return license::Verify(request,payload,signature,request.wall,100001,claims,pem.c_str(),argv[2])?0:1;
}
