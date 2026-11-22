#pragma once
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include <fcntl.h>
#include <unistd.h>

namespace kill_cosmetics { namespace memory {
inline constexpr std::size_t maximumRead=256;
class Reader {
public:
    Reader()=default;
    ~Reader() {Close();}
    Reader(const Reader&)=delete;
    Reader& operator=(const Reader&)=delete;
    bool Copy(const void* source,void* output,std::size_t size) {
        const auto address=reinterpret_cast<std::uintptr_t>(source);
        if(!address || !output || !size || size>maximumRead || size>std::numeric_limits<std::uintptr_t>::max()-address) return false;
        std::lock_guard<std::mutex> lock(mutex_);
        if(descriptors_[0]<0 && pipe2(descriptors_,O_CLOEXEC|O_NONBLOCK)!=0) return false;
        ssize_t written=-1,read=-1;
        for(unsigned i=0;i<4;++i) {
            written=write(descriptors_[1],source,size);
            if(written>=0 || errno!=EINTR)break;
        }
        if(written!=static_cast<ssize_t>(size)) {Close();return false;}
        for(unsigned i=0;i<4;++i) {
            read=::read(descriptors_[0],output,size);
            if(read>=0 || errno!=EINTR)break;
        }
        if(read!=static_cast<ssize_t>(size)) {Close();return false;}
        return true;
    }
private:
    void Close() {
        for(auto& descriptor:descriptors_) {
            if(descriptor>=0)close(descriptor);
            descriptor=-1;
        }
    }
    int descriptors_[2]{-1,-1};
    std::mutex mutex_;
};
inline bool Copy(const void* source,void* output,std::size_t size) {
    static Reader reader;
    return reader.Copy(source,output,size);
}
}}
