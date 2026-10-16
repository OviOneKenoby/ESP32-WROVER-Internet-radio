#include "../src/ccm_wire.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

int main(){
    uint8_t b[64]{}; unsigned checks=0;
    assert(ccm_wire::identity("1.1.11",b,sizeof b)==11);++checks;
    assert(b[0]==2&&b[1]==1&&b[2]==1&&b[3]==0&&b[4]==6);++checks;
    assert(ccm_wire::capabilities(b,sizeof b)==16&&b[1]==0x07);++checks;
    assert(ccm_wire::snapshot(68,1,9,3,b,sizeof b)==19&&b[0]==6&&b[1]==68&&b[2]==1);++checks;
    uint8_t request[17]={7,0,0,0,0,0,0,0,42,0,0,0,17,0,0,0,25};
    ccm_wire::VolumeSet v;assert(ccm_wire::parseVolumeSet(request,sizeof request,v));++checks;
    uint8_t percent=0;assert(ccm_wire::volumePercent(v,percent)&&percent==68);++checks;
    const struct { uint32_t n,d; uint8_t expected; } targets[] = {
        {0,1,0},{1,100,1},{1,2,50},{19,20,95},{1,1,100}
    };
    for (const auto& target : targets) {
        v.numerator=target.n;v.denominator=target.d;
        assert(ccm_wire::volumePercent(v,percent)&&percent==target.expected);++checks;
    }
    v.numerator=1;v.denominator=3;
    assert(!ccm_wire::volumePercent(v,percent));++checks;
    request[16]=24;assert(!ccm_wire::parseVolumeSet(request,16,v));++checks;
    request[16]=0;assert(ccm_wire::parseVolumeSet(request,17,v)&&!ccm_wire::volumePercent(v,percent));++checks;
    assert(ccm_wire::result(42,true,0,b,sizeof b)==11&&b[0]==8&&b[9]==1&&b[10]==0);++checks;
    assert(ccm_wire::identity("x",b,5)==0);++checks;
    std::cout<<"TOTAL "<<checks<<"/"<<checks<<" PASS\n";
}
