//==============================================================================
//
//  OvenMediaEngine
//
//  Created by Jaejong Bong
//  Copyright (c) 2018 AirenSoft. All rights reserved.
//
//==============================================================================

#include "segment_stream.h"
#include "segment_stream_application.h"

#define OV_LOG_TAG "SegmentStream"

using namespace common;

//====================================================================================================
// Create
//====================================================================================================
std::shared_ptr<SegmentStream> SegmentStream::Create(const std::shared_ptr<Application> application, const StreamInfo &info)
{
	auto stream = std::make_shared<SegmentStream>(application, info);
	if(!stream->Start())
	{
		return nullptr;
	}
	return stream;
}

//====================================================================================================
// SegmentStream
//====================================================================================================
SegmentStream::SegmentStream(const std::shared_ptr<Application> application, const StreamInfo &info) : Stream(application, info)
{
	std::string prefix = this->GetName().CStr();
	std::shared_ptr<MediaTrack> video_track = nullptr;
	std::shared_ptr<MediaTrack> audio_track = nullptr;

	for(auto &track_item : _tracks)
	{
		auto &track = track_item.second;

		switch (track->GetMediaType())
		{
			case MediaType::Video:
			{
				video_track = track;
			}
			case MediaType::Audio:
			{
				audio_track = track;
			}
		}
	}

	PacketyzerMediaInfo media_info;

	if(video_track != nullptr)
	{
		if(video_track->GetCodecId() == MediaCodecId::H264) media_info.video_codec_type = SegmentCodecType::H264Codec;
		else if(video_track->GetCodecId() == MediaCodecId::Vp8) media_info.video_codec_type = SegmentCodecType::Vp8Codec;
		else media_info.video_codec_type = SegmentCodecType::UnknownCodec;

		media_info.video_framerate = video_track->GetFrameRate();
		media_info.video_width 	= video_track->GetWidth();
		media_info.video_height = video_track->GetHeight();
		media_info.video_timescale = video_track->GetTimeBase().GetDen();
		media_info.video_bitrate   = video_track->GetBitrate();
	}

	if(audio_track != nullptr)
	{
		if(audio_track->GetCodecId() == MediaCodecId::Aac) media_info.audio_codec_type = SegmentCodecType::AacCodec;
		else if(audio_track->GetCodecId() == MediaCodecId::Opus) media_info.audio_codec_type = SegmentCodecType::OpusCodec;
		else media_info.audio_codec_type = SegmentCodecType::UnknownCodec;

		media_info.audio_samplerate = audio_track->GetSampleRate();
		media_info.audio_channels 	= audio_track->GetChannel().GetCounts();
		media_info.audio_timescale = audio_track->GetTimeBase().GetDen();
		media_info.audio_bitrate   = audio_track->GetBitrate();
	}

	_stream_packetyzer = std::make_unique<StreamPacketyzer>(prefix, PacketyzerStreamType::Common, 5, 5, media_info);
}

//====================================================================================================
// ~SegmentStream
//====================================================================================================
SegmentStream::~SegmentStream()
{
	Stop();
}

//====================================================================================================
// Start
//====================================================================================================
bool SegmentStream::Start()
{
	return Stream::Start();
}

//====================================================================================================
// Stop
//====================================================================================================
bool SegmentStream::Stop()
{
	return Stream::Stop();
}

//====================================================================================================
// SendVideoFrame
// - Packetyzer에 Video데이터 추가
// - 첫 key 프레임 에서 SPS/PPS 추출  이후 생성
//
//====================================================================================================
void SegmentStream::SendVideoFrame(std::shared_ptr<MediaTrack> 			track,
								   std::unique_ptr<EncodedFrame> 		encoded_frame,
								   std::unique_ptr<CodecSpecificInfo> 	codec_info,
								   std::unique_ptr<FragmentationHeader> fragmentation)
{
	//logtd("Video Timestamp : %d" , encoded_frame->time_stamp);

    if(_stream_packetyzer != nullptr)
    {
        _stream_packetyzer->AppendVideoData(encoded_frame->time_stamp,
											track->GetTimeBase().GetDen(),
											encoded_frame->frame_type == FrameType::VideoFrameKey,
											0,
											encoded_frame->length,
											encoded_frame->buffer);
    }
}

//====================================================================================================
// SendAudioFrame
// - Packetyzer에 Audio데이터 추가
//====================================================================================================
void SegmentStream::SendAudioFrame(std::shared_ptr<MediaTrack> 			track,
								   std::unique_ptr<EncodedFrame> 		encoded_frame,
								   std::unique_ptr<CodecSpecificInfo> 	codec_info,
								   std::unique_ptr<FragmentationHeader> fragmentation)
{
    //logtd("Audio Timestamp : %d", encoded_frame->time_stamp);

    if(_stream_packetyzer != nullptr)
    {
        _stream_packetyzer->AppendAudioData(encoded_frame->time_stamp,
											track->GetTimeBase().GetDen(),
											encoded_frame->length,
											encoded_frame->buffer);
    }

}

//====================================================================================================
// GetPlayList
// - M3U8/MPD
//====================================================================================================
bool SegmentStream::GetPlayList(PlayListType play_list_type, ov::String &play_list)
{
    if (_stream_packetyzer == nullptr)
    {
        return false;
    }

    return  _stream_packetyzer->GetPlayList(play_list_type, play_list);
}

//====================================================================================================
// GetSegment
// - TS/MP4
//====================================================================================================
bool SegmentStream::GetSegment(SegmentType type, const ov::String &file_name, std::shared_ptr<ov::Data> &data)
{
    if (_stream_packetyzer == nullptr)
    {
        return false;
    }

    return  _stream_packetyzer->GetSegment(type, file_name, data);
}
