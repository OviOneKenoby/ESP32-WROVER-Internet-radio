#include "ccm_wire.h"
#include <cstring>

namespace ccm_wire {
namespace {
uint32_t get32(const uint8_t* p) { return uint32_t(p[0])<<24|uint32_t(p[1])<<16|uint32_t(p[2])<<8|p[3]; }
uint64_t get64(const uint8_t* p) { uint64_t v=0; for(unsigned i=0;i<8;++i)v=(v<<8)|p[i]; return v; }
void put32(uint8_t* p,uint32_t v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
void put64(uint8_t* p,uint64_t v){for(int i=7;i>=0;--i){p[i]=uint8_t(v);v>>=8;}}
}
bool parseVolumeSet(const uint8_t* p,size_t n,VolumeSet& v){
    if(!p||n!=17||p[0]!=uint8_t(Kind::volume_set))return false;
    v.request=get64(p+1);v.numerator=get32(p+9);v.denominator=get32(p+13);return v.request!=0;
}
bool volumePercent(const VolumeSet& v,uint8_t& out){
    if(!v.denominator||v.numerator>v.denominator)return false;
    const uint64_t scaled=uint64_t(v.numerator)*100;
    if(scaled%v.denominator)return false;
    const uint64_t value=scaled/v.denominator;if(value>100)return false;out=uint8_t(value);return true;
}
size_t identity(const char* firmware,uint8_t* o,size_t cap){
    if(!firmware||!o||cap<5)return 0;
    const size_t n=strnlen(firmware,31);
    if(cap<5+n)return 0;
    o[0]=uint8_t(Kind::identity_response);o[1]=1;o[2]=1;o[3]=0;o[4]=uint8_t(n);memcpy(o+5,firmware,n);return 5+n;
}
size_t capabilities(uint8_t* o,size_t cap){
    if(!o||cap<16)return 0;
    o[0]=uint8_t(Kind::capability_response);o[1]=0x07;
    put64(o+2,1);put32(o+10,2000);o[14]=1;o[15]=100;return 16;
}
size_t snapshot(uint8_t volume,uint8_t playback,uint64_t sequence,uint64_t media,uint8_t* o,size_t cap){
    if(!o||cap<19||volume>100||playback>3||!sequence||!media)return 0;
    o[0]=uint8_t(Kind::snapshot_response);o[1]=volume;o[2]=playback;put64(o+3,sequence);put64(o+11,media);return 19;
}
size_t result(uint64_t request,bool accepted,uint8_t error,uint8_t* o,size_t cap){
    if(!o||cap<11||!request)return 0;
    o[0]=uint8_t(Kind::command_result);put64(o+1,request);
    o[9]=accepted?1:0;o[10]=error;return 11;
}
}
