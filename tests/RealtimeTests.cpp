#include "EpicDSP.h"
#include <cstdlib>
#include <new>
#include <iostream>
#include <chrono>
namespace { bool audioThread=false;size_t allocations=0; }
void* operator new(std::size_t bytes){if(audioThread)++allocations;if(auto* p=std::malloc(bytes?bytes:1))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept{std::free(p);}
void* operator new[](std::size_t bytes){return ::operator new(bytes);}
void operator delete[](void* p) noexcept{::operator delete(p);}
int main(){
    epinizer::Engine engine;engine.prepare(48000);epinizer::Settings s;engine.snap(s);
    std::array<float,512> l{},r{};double seconds=0;volatile float checksum=0;
    for(int block=0;block<2000;++block){for(size_t i=0;i<l.size();++i)l[i]=r[i]=.2f*std::sin(float(block*512+int(i))*.061f);
        s.character=(block/200)%4;s.hold=block%400>=200;s.echo=block%300<150;
        const auto start=std::chrono::steady_clock::now();audioThread=true;engine.process(l.data(),r.data(),512,s);audioThread=false;
        seconds+=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        for(size_t i=0;i<l.size();i+=16)checksum+=l[i]+r[i];
    }
    if(!std::isfinite(checksum))return 2;
    if(allocations){std::cerr<<"FAIL: "<<allocations<<" C++ allocations on the audio thread\n";return 1;}
    std::cout<<"PASS: no C++ allocations during rendering/character/Echo/Hold changes. Rendered 21.33s stereo in "<<seconds<<"s\n";
}
