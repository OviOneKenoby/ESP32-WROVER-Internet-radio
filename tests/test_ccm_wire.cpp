#include "../src/ccm_wire.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

int main(){
    uint8_t b[64]{}; unsigned checks=0;
    assert(ccm_wire::identity("1.1.11",b,sizeof b)==11);++checks;
    assert(b[0]==2&&b[1]==1&&b[2]==1&&b[3]==0&&b[4]==6);++checks;
    assert(ccm_wire::capabilities(b,sizeof b)==16&&b[1]==0xff);++checks;
    assert(b[2]==0&&b[3]==0&&b[4]==0&&b[5]==0&&b[6]==0&&b[7]==0&&b[8]==0&&b[9]==2);++checks;
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
    uint8_t playback[19]={9,0,0,0,0,0,0,0,42,4,0,0,0,0,0,0,0,0,3};
    ccm_wire::PlaybackCommand pc;
    assert(ccm_wire::parsePlaybackCommand(playback,sizeof playback,pc)&&pc.request==42&&
           pc.operation==4&&pc.expected_state==0&&pc.media_generation==3);++checks;
    assert(!ccm_wire::parsePlaybackCommand(playback,18,pc));++checks;
    playback[18]=0;assert(!ccm_wire::parsePlaybackCommand(playback,19,pc));++checks;
    playback[18]=3;playback[9]=5;assert(!ccm_wire::parsePlaybackCommand(playback,19,pc));++checks;
    playback[9]=4;playback[10]=4;assert(!ccm_wire::parsePlaybackCommand(playback,19,pc));++checks;
    playback[10]=0;
    ccm_wire::NowPlaying np;
    np.flags=0x93;np.context=1;np.station="Radio";np.station_size=5;
    np.combined="Artist - Title";np.combined_size=14;
    np.sequence=9;np.media_generation=3;np.metadata_revision=7;
    assert(ccm_wire::nowPlaying(np,b,sizeof b)==51&&b[0]==11&&b[1]==0x93&&b[2]==1&&
           b[3]==5&&b[4]==0&&b[5]==0&&b[6]==0&&b[7]==14&&
           b[15]==9&&b[23]==3&&b[31]==7&&!memcmp(b+32,"RadioArtist - Title",19));++checks;
    np.flags=0x02;np.combined_size=0;assert(ccm_wire::nowPlaying(np,b,sizeof b)==0);++checks;
    assert(ccm_wire::identity("x",b,5)==0);++checks;
    std::cout<<"TOTAL "<<checks<<"/"<<checks<<" PASS\n";
}
