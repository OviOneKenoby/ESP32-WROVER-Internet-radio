#include "rtcp_d009.h"
#include "correctness_guards.h"
#include <cstring>

namespace rtcp_d009 {
namespace {
void increment(uint64_t& value) { if (value != UINT64_MAX) ++value; }

bool decode(const unsigned char* p,size_t n,size_t& width,uint32_t& cp) {
    if(!n)return false;
    if(p[0]<0x80){width=1;cp=p[0];return cp>=0x20&&cp!=0x7f;}
    if(p[0]>=0xc2&&p[0]<=0xdf){width=2;cp=p[0]&0x1f;}
    else if(p[0]>=0xe0&&p[0]<=0xef){width=3;cp=p[0]&0x0f;}
    else if(p[0]>=0xf0&&p[0]<=0xf4){width=4;cp=p[0]&0x07;}
    else return false;
    if(width>n)return false;
    for(size_t i=1;i<width;++i){if((p[i]&0xc0)!=0x80)return false;cp=(cp<<6)|(p[i]&0x3f);}
    if((width==2&&cp<0x80)||(width==3&&cp<0x800)||(width==4&&cp<0x10000)||
       (cp>=0xd800&&cp<=0xdfff)||cp>0x10ffff)return false;
    return true;
}

bool copyText(const char* source,char* destination,size_t capacity,uint8_t& size,bool& was_truncated) {
    size=0;was_truncated=false;if(!source||!source[0]){destination[0]=0;return true;}
    const size_t source_size=strlen(source);
    if(!validUtf8Text(source,source_size)){destination[0]=0;return false;}
    size_t at=0;
    while(at<source_size){size_t width=0;uint32_t cp=0;
        if(!decode(reinterpret_cast<const unsigned char*>(source)+at,source_size-at,width,cp))return false;
        if(at+width>=capacity){was_truncated=true;break;} at+=width;
    }
    memcpy(destination,source,at);destination[at]=0;size=uint8_t(at);return true;
}

bool same(const char* a,uint8_t an,const char* b,uint8_t bn) {
    return an==bn&&(an==0||memcmp(a,b,an)==0);
}
}

bool validUtf8Text(const char* text,size_t size) {
    if(!text&&size)return false;
    size_t at=0;
    while(at<size){size_t width=0;uint32_t cp=0;
        if(!decode(reinterpret_cast<const unsigned char*>(text)+at,size-at,width,cp))return false;
        at+=width;
    }
    return true;
}

void Projection::invalidateMediaContext() {
    increment(media_generation_);increment(metadata_revision_);
    station_[0]=combined_[0]=0;station_size_=combined_size_=0;flags_=0;
    context_=Context::none;playback_state_=2;station_count_=0;selected_station_=0;
    generation_preincremented_=true;initialized_=true;
}

void Projection::publish(const Input& input) {
    char station[64]{},combined[256]{};uint8_t station_size=0,combined_size=0;
    bool station_truncated=false,metadata_truncated=false;
    const bool radio=input.context==Context::fixed_radio||input.context==Context::discovered_radio;
    const bool station_ok=radio&&copyText(input.station,station,sizeof station,station_size,station_truncated)&&station_size;
    bool metadata_ok=false;
    if(radio&&input.combined&&strcmp(input.combined,"Live stream")!=0) {
        metadata_ok=copyText(input.combined,combined,sizeof combined,combined_size,metadata_truncated)&&combined_size;
    }
    uint8_t flags=0;
    if(station_ok)flags|=station_valid;
    if(metadata_ok)flags|=metadata_valid;
    if(station_truncated||metadata_truncated)flags|=truncated;
    if(radio&&input.playback_state==1)flags|=play_available;
    if(radio&&input.playback_state==0)flags|=pause_available;
    if(input.context==Context::fixed_radio&&station_ok&&input.station_count&&
       input.selected_station<input.station_count)
        flags|=previous_available|next_available;

    const bool identity_changed=initialized_&&
        (context_!=input.context||source_identity_!=input.source_identity||
         !same(station_,station_size_,station,station_size));
    if(identity_changed){
        if(generation_preincremented_)generation_preincremented_=false;
        else increment(media_generation_);
    } else if(generation_preincremented_&&input.context==Context::none) {
        generation_preincremented_=false;
    }
    const bool metadata_changed=initialized_&&
        (flags_!=flags||context_!=input.context||
         !same(station_,station_size_,station,station_size)||
         !same(combined_,combined_size_,combined,combined_size));
    if(metadata_changed)increment(metadata_revision_);
    memcpy(station_,station,sizeof station_);memcpy(combined_,combined,sizeof combined_);
    station_size_=station_size;combined_size_=combined_size;flags_=flags;
    context_=input.context;playback_state_=input.playback_state;
    selected_station_=input.selected_station;station_count_=input.station_count;
    source_identity_=input.source_identity;initialized_=true;
}

Decision Projection::admit(const ccm_wire::PlaybackCommand& command) const {
    Decision result{};
    if(command.operation<uint8_t(ccm_wire::PlaybackOperation::play)||
       command.operation>uint8_t(ccm_wire::PlaybackOperation::next)||command.expected_state>3)
        return result;
    if(command.media_generation!=media_generation_){result.error=stale;return result;}
    const auto operation=ccm_wire::PlaybackOperation(command.operation);
    if(operation==ccm_wire::PlaybackOperation::play||operation==ccm_wire::PlaybackOperation::pause){
        if(context_!=Context::fixed_radio&&context_!=Context::discovered_radio){result.error=unavailable;return result;}
        if(command.expected_state!=playback_state_){result.error=stale;return result;}
        const uint8_t required=operation==ccm_wire::PlaybackOperation::play?play_available:pause_available;
        result.error=(flags_&required)?success:unavailable;return result;
    }
    if(context_!=Context::fixed_radio||!station_count_||selected_station_>=station_count_){result.error=unavailable;return result;}
    const bool traversed=operation==ccm_wire::PlaybackOperation::next
        ?CorrectnessGuards::nextStationIndex(selected_station_,station_count_,result.target_station)
        :CorrectnessGuards::previousStationIndex(selected_station_,station_count_,result.target_station);
    if(!traversed){result.error=unavailable;return result;}
    result.error=success;return result;
}

ccm_wire::NowPlaying Projection::wire(uint64_t sequence) const {
    ccm_wire::NowPlaying value{};value.flags=flags_;value.context=uint8_t(context_);
    value.station=station_;value.station_size=station_size_;
    value.combined=combined_;value.combined_size=combined_size_;
    value.sequence=sequence;value.media_generation=media_generation_;
    value.metadata_revision=metadata_revision_;return value;
}

} // namespace rtcp_d009
