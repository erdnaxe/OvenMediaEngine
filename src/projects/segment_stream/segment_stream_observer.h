//==============================================================================
//
//  OvenMediaEngine
//
//  Created by Jaejong Bong
//  Copyright (c) 2018 AirenSoft. All rights reserved.
//
//==============================================================================

#pragma once
#include <memory>
#include <base/ovlibrary/ovlibrary.h>
#include "stream_packetyzer.h"

//====================================================================================================
// SegmentStreamObserver
//====================================================================================================
class SegmentStreamObserver : public ov::EnableSharedFromThis<SegmentStreamObserver>
{
public:
    // PlayList 요청
    virtual bool OnPlayListRequest(const ov::String &app_name, const ov::String &stream_name, PlayListType play_list_type, ov::String &play_list) = 0;

    // Segment 요청
    virtual bool OnSegmentRequest(const ov::String &app_name, const ov::String &stream_name, SegmentType segment_type, const ov::String &file_name, std::shared_ptr<ov::Data> &segment_data) = 0;

    // Crossdomain 요청
    virtual bool OnCrossdomainRequest(ov::String &cross_domain) = 0;

    // Cors 확인
    virtual bool OnCorsCheck(const ov::String &app_name, const ov::String &stream_name, ov::String &origin_url) = 0;
};
