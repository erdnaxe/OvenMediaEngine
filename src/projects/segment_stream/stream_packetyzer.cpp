﻿//==============================================================================
//
//  OvenMediaEngine
//
//  Created by Jaejong Bong
//  Copyright (c) 2018 AirenSoft. All rights reserved.
//
//==============================================================================

#include "stream_packetyzer.h"

#define MAX_VIDEO_DATA_QUEUE_COUNT 		(500)
#define MAX_INPUT_DATA_SIZE				(10485760*2) // 20mb

//====================================================================================================
// Constructor
//====================================================================================================
StreamPacketyzer::StreamPacketyzer(std::string &segment_prefix, PacketyzerStreamType stream_type, int segment_count, int segment_duration, PacketyzerMediaInfo &media_info)
{
	_video_timescale    = PACKTYZER_DEFAULT_TIMESCALE;
	_audio_timescale    = media_info.audio_samplerate;
    _stream_type        = stream_type;

	// timescale 변경
	media_info.video_timescale = _video_timescale;
	media_info.audio_timescale = _audio_timescale;

	_hls_packetyzer = std::make_shared<HlsPacketyzer>(segment_prefix, stream_type, segment_count, segment_duration, media_info);
	_dash_packetyzer = std::make_shared<DashPacketyzer>(segment_prefix, stream_type, segment_count, segment_duration, media_info);
}

//====================================================================================================
// Destructor
//====================================================================================================
StreamPacketyzer::~StreamPacketyzer( )
{
	// Video 데이터 정리
	_video_data_queue.clear();
}

//====================================================================================================
// Append Video Data
//====================================================================================================
bool StreamPacketyzer::AppendVideoData(uint64_t timestamp, uint32_t timescale, bool is_keyframe, uint64_t time_offset, uint32_t data_size, uint8_t *data)
{
	// 임시
	timescale = 90000;

	// data vaild check
	if(data_size <= 0 || data_size > MAX_INPUT_DATA_SIZE)
	{
		printf("ERROR : [StreamHlsMaker] AppendVideoData - Data Size Error(%d:%d)", data_size, MAX_INPUT_DATA_SIZE);
		return false; 
	}
	
	// Queue size check
	if(_video_data_queue.size() > MAX_VIDEO_DATA_QUEUE_COUNT)
	{
		printf("ERROR : [StreamHlsMaker] AppendVideoData - VideoDataQueue Count Over(%d:%d)", (int)_video_data_queue.size(), MAX_VIDEO_DATA_QUEUE_COUNT);
		return false;
	}

	// timestamp change
	if (timescale != _video_timescale)
	{
		timestamp = Packetyzer::ConvertTimeScale(timestamp, timescale, _video_timescale);
		if(time_offset != 0)time_offset = Packetyzer::ConvertTimeScale(time_offset, timescale, _video_timescale);

	}

	auto frame = std::make_shared<std::vector<uint8_t>>(data, data + data_size);
	auto video_data = std::make_shared<PacketyzerFrameData>(is_keyframe ? PacketyzerFrameType::VideoIFrame : PacketyzerFrameType::VideoPFrame,  timestamp, time_offset, _video_timescale, frame);

	// Video data save
	if(_stream_type == PacketyzerStreamType::VideoOnly)
    {
        _dash_packetyzer->AppendVideoFrame(video_data);
        _hls_packetyzer->AppendVideoFrame(video_data);
    }
	else
    {
	    _video_data_queue.push_back(video_data);
    }

	return true; 
}

//====================================================================================================
// Video Data SampleWrite
//====================================================================================================
bool StreamPacketyzer::VideoDataSampleWrite(uint64_t timestamp)
{
	//Video data append
	while(!_video_data_queue.empty())
	{
		auto video_data = _video_data_queue.front();

		if(video_data != nullptr)
		{
			if(video_data->timestamp > timestamp)
			{
				break;
			}

			_dash_packetyzer->AppendVideoFrame(video_data);
			_hls_packetyzer->AppendVideoFrame(video_data);
 		}

		_video_data_queue.pop_front();
	}

	return true;
}

//====================================================================================================
// Append Audio Data
//====================================================================================================
bool StreamPacketyzer::AppendAudioData(uint64_t timestamp, uint32_t timescale, uint32_t data_size, uint8_t *data)
{
	// data vaild check
	if(data_size <= 0 || data_size > MAX_INPUT_DATA_SIZE)
	{
		printf("ERROR : [StreamHlsMaker] AppendAudioData - Data Size Error(%d:%d)", data_size, MAX_INPUT_DATA_SIZE);
		return false; 
	}

	// Video 데이터 삽입(오디오 타임스탬프 이전 데이터)
	VideoDataSampleWrite(Packetyzer::ConvertTimeScale(timestamp, timescale, _video_timescale));

	// timestamp change
	if (timescale != _audio_timescale)
	{
		timestamp = Packetyzer::ConvertTimeScale(timestamp, timescale, _audio_timescale);
	}

	auto frame = std::make_shared<std::vector<uint8_t>>(data, data + data_size);
	auto audio_data = std::make_shared<PacketyzerFrameData>(PacketyzerFrameType::AudioFrame,  timestamp, 0, _audio_timescale, frame);

	// hls/dash Audio 데이터 삽입
	_dash_packetyzer->AppendAudioFrame(audio_data);
	_hls_packetyzer->AppendAudioFrame(audio_data);

	return true;
}

//====================================================================================================
// Get PlayList
// - M3U8/MPD
//====================================================================================================
bool StreamPacketyzer::GetPlayList(PlayListType play_list_type, ov::String &segment_play_list)
{
	bool result = false;
	std::string play_list;

	if		(play_list_type == PlayListType::M3u8)  result = _hls_packetyzer->GetPlayList(play_list);
	else if(play_list_type == PlayListType::Mpd)   result = _dash_packetyzer->GetPlayList(play_list);

	if(result)segment_play_list = play_list.c_str();

	return result;
}

//====================================================================================================
// GetSegment
// - TS/MP4
//====================================================================================================
bool StreamPacketyzer::GetSegment(SegmentType type, const ov::String &segment_file_name, std::shared_ptr<ov::Data> &segment_data)
{
	bool result = false;
	std::string file_name = segment_file_name.CStr();
    std::shared_ptr<std::vector<uint8_t>> data;

	if      (type == SegmentType::MpegTs)   result = _hls_packetyzer->GetSegmentData(file_name, data);
	else if(type == SegmentType::M4S)      result = _dash_packetyzer->GetSegmentData(file_name, data);

    if(result)segment_data = std::make_shared<ov::Data>(data->data(), data->size());

	return result;
}
