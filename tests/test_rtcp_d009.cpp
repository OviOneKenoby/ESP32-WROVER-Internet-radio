#include "../src/rtcp_d009.h"
#include <cassert>
#include <cstring>
#include <iostream>

namespace {
unsigned checks=0;
void expect(bool value){assert(value);++checks;}

rtcp_d009::Input fixed(const char* station,const char* title,uint8_t state=0,
                       uint8_t selected=0,uint8_t count=3){
    rtcp_d009::Input input;input.context=rtcp_d009::Context::fixed_radio;
    input.playback_state=state;input.station=station;input.combined=title;
    input.selected_station=selected;input.station_count=count;input.source_identity=1;return input;
}

ccm_wire::PlaybackCommand command(uint8_t operation,uint8_t state,uint64_t generation){
    ccm_wire::PlaybackCommand value;value.request=9;value.operation=operation;
    value.expected_state=state;value.media_generation=generation;return value;
}
}

int main(){
    expect(sizeof(rtcp_d009::Projection)+sizeof(rtcp_d009::Mailbox)<=512);
    rtcp_d009::Projection projection;
    projection.publish(fixed("Station A","Artist - Title"));
    expect(projection.mediaGeneration()==1&&projection.metadataRevision()==1);
    expect((projection.flags()&(rtcp_d009::station_valid|rtcp_d009::metadata_valid|
           rtcp_d009::pause_available|rtcp_d009::previous_available|rtcp_d009::next_available))!=0);
    auto wire=projection.wire(12);
    expect(wire.sequence==12&&wire.station_size==9&&wire.combined_size==14&&
           !memcmp(wire.combined,"Artist - Title",14));

    const uint64_t revision=projection.metadataRevision();
    projection.publish(fixed("Station A","Next title"));
    expect(projection.mediaGeneration()==1&&projection.metadataRevision()==revision+1);
    projection.invalidateMediaContext();
    expect(projection.mediaGeneration()==2&&!(projection.flags()&rtcp_d009::metadata_valid));
    projection.publish(fixed("Station B","Fresh title",0,1,3));
    expect(projection.mediaGeneration()==2&&projection.wire(1).station_size==9&&
           !memcmp(projection.wire(1).station,"Station B",9));

    auto decision=projection.admit(command(uint8_t(ccm_wire::PlaybackOperation::next),0,2));
    expect(decision.error==rtcp_d009::success&&decision.target_station==2);
    projection.publish(fixed("Station B","Fresh title",0,2,3));
    decision=projection.admit(command(uint8_t(ccm_wire::PlaybackOperation::next),0,2));
    expect(decision.error==rtcp_d009::success&&decision.target_station==0);
    decision=projection.admit(command(uint8_t(ccm_wire::PlaybackOperation::previous),0,2));
    expect(decision.error==rtcp_d009::success&&decision.target_station==1);
    decision=projection.admit(command(uint8_t(ccm_wire::PlaybackOperation::pause),1,2));
    expect(decision.error==rtcp_d009::stale);
    decision=projection.admit(command(99,0,2));expect(decision.error==rtcp_d009::invalid_argument);
    decision=projection.admit(command(uint8_t(ccm_wire::PlaybackOperation::pause),0,1));
    expect(decision.error==rtcp_d009::stale);

    auto discovered=fixed("One off","Live stream");
    discovered.context=rtcp_d009::Context::discovered_radio;
    projection.publish(discovered);
    expect(!(projection.flags()&rtcp_d009::metadata_valid));
    decision=projection.admit(command(uint8_t(ccm_wire::PlaybackOperation::next),0,
                                      projection.mediaGeneration()));
    expect(decision.error==rtcp_d009::unavailable);

    projection.publish(fixed("Empty","Track",0,0,0));
    decision=projection.admit(command(uint8_t(ccm_wire::PlaybackOperation::previous),0,
                                      projection.mediaGeneration()));
    expect(decision.error==rtcp_d009::unavailable);

    const char malformed[]={'B','a','d',char(0xc0),char(0xaf),0};
    projection.publish(fixed(malformed,malformed));
    wire=projection.wire(3);
    expect(!(wire.flags&rtcp_d009::station_valid)&&!(wire.flags&rtcp_d009::metadata_valid));
    char oversized[300];memset(oversized,'x',sizeof oversized);oversized[sizeof oversized-1]=0;
    projection.publish(fixed(oversized,oversized));wire=projection.wire(4);
    expect(wire.station_size==63&&wire.combined_size==255&&(wire.flags&rtcp_d009::truncated));
    expect(rtcp_d009::validUtf8Text("\xe2\x82\xac",3));
    expect(!rtcp_d009::validUtf8Text("\x01",1));

    rtcp_d009::Mailbox mailbox;auto queued=command(1,1,projection.mediaGeneration());
    expect(mailbox.put(queued,2)&&!mailbox.put(queued,3));
    ccm_wire::PlaybackCommand taken;uint8_t target=0;
    expect(mailbox.take(taken,target)&&taken.request==queued.request&&target==2);
    mailbox.put(queued,1);mailbox.clear();expect(!mailbox.take(taken,target));

    std::cout<<"D009 FOCUSED "<<checks<<"/"<<checks<<" PASS\n";
}
