
// MP4Encoder.cpp
#include "Encoder.h"



MP4Encoder::MP4Encoder(const std::string& filename,
                       int width, int height, int fps,
                       int sample_rate, int channels)
    : _filename(filename)
    , _width(width)
    , _height(height)
    , _fps(fps)
    , _sampleRate(sample_rate)
    , _channels(channels)
{
    avformat_network_init();
    initOutput();
    initVideoStream();
    initAudioStream();
    writeHeader();

    // Allocate packet
    _pkt = av_packet_alloc();

    // Prepare RGB buffer for glReadPixels
    _rgbBuffer.resize(_width * _height * 4);
}

MP4Encoder::~MP4Encoder() {
    // ensure finalize was called
    if (_fmtCtx) {
        finalize();
    }
}

void MP4Encoder::initOutput() {
    int ret = avformat_alloc_output_context2(&_fmtCtx, NULL, NULL, _filename.c_str());
    CHECK_ERR(ret);
    if (!(_fmtCtx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&_fmtCtx->pb, _filename.c_str(), AVIO_FLAG_WRITE);
        CHECK_ERR(ret);
    }
}

void MP4Encoder::initVideoStream() {
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) throw std::runtime_error("H264 encoder not found");
    _videoStream = avformat_new_stream(_fmtCtx, codec);
    _videoCtx = avcodec_alloc_context3(codec);
    _videoCtx->codec_id = AV_CODEC_ID_H264;
    _videoCtx->width = _width;
    _videoCtx->height = _height;
    _videoCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    _videoCtx->time_base = AVRational{1, _fps};
    _videoCtx->bit_rate = 800000;
    _videoCtx->gop_size = 12;
    if (_fmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
        _videoCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    int ret = avcodec_open2(_videoCtx, codec, NULL);
    CHECK_ERR(ret);
    avcodec_parameters_from_context(_videoStream->codecpar, _videoCtx);
    _videoStream->time_base = _videoCtx->time_base;

    // Allocate video frame
    _videoFrame = av_frame_alloc();
    _videoFrame->format = _videoCtx->pix_fmt;
    _videoFrame->width = _width;
    _videoFrame->height = _height;
    ret = av_frame_get_buffer(_videoFrame, 0);
    CHECK_ERR(ret);

    // Initialize sws for RGBA->YUV420P
    _swsCtx = sws_getContext(
        _width, _height, AV_PIX_FMT_RGBA,
        _width, _height, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, NULL, NULL, NULL);
    if (!_swsCtx) throw std::runtime_error("Failed to init sws context");
}

void MP4Encoder::initAudioStream() {
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!codec) throw std::runtime_error("AAC encoder not found");
    _audioStream = avformat_new_stream(_fmtCtx, codec);
    _audioCtx = avcodec_alloc_context3(codec);
    _audioCtx->codec_id = AV_CODEC_ID_AAC;
    _audioCtx->sample_rate = _sampleRate;
    _audioCtx->ch_layout = {AVChannelOrder::AV_CHANNEL_ORDER_UNSPEC, _channels};
    _audioCtx->sample_fmt = codec->sample_fmts[0]; // e.g. AV_SAMPLE_FMT_FLTP
    _audioCtx->bit_rate = 128000;
    _audioCtx->time_base = AVRational{1, _sampleRate};
    if (_fmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
        _audioCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    int ret = avcodec_open2(_audioCtx, codec, NULL);
    CHECK_ERR(ret);
    avcodec_parameters_from_context(_audioStream->codecpar, _audioCtx);
    _audioStream->time_base = _audioCtx->time_base;

    // Allocate audio frame
    _audioFrame = av_frame_alloc();
    _audioFrame->format = _audioCtx->sample_fmt;
    _audioFrame->ch_layout = _audioCtx->ch_layout;
    _audioFrame->sample_rate = _audioCtx->sample_rate;
    _audioFrame->nb_samples = _audioCtx->frame_size;
    ret = av_frame_get_buffer(_audioFrame, 0);
    CHECK_ERR(ret);

    // Initialize swr if needed (input float to output planar)
    if (_audioCtx->sample_fmt == AV_SAMPLE_FMT_FLTP) {
        ret = swr_alloc_set_opts2(
            &_swrCtx,
            &_audioCtx->ch_layout, _audioCtx->sample_fmt, _audioCtx->sample_rate,
            &_audioCtx->ch_layout, AV_SAMPLE_FMT_FLT, _audioCtx->sample_rate,
            0, nullptr);
        CHECK_ERR(ret);

        ret = swr_init(_swrCtx);
        CHECK_ERR(ret);
    }
}

void MP4Encoder::writeHeader() {
    int ret = avformat_write_header(_fmtCtx, nullptr);
    CHECK_ERR(ret);
}

void MP4Encoder::addAudio(const std::vector<float>& samples) {
    size_t cursor = 0;
    size_t frameSize = _audioCtx->frame_size;
    while (cursor + frameSize <= samples.size()) {
        // copy raw data
        if (_swrCtx) {
            // convert interleaved float to planar float
            const uint8_t* inData[1] = {(const uint8_t*)(samples.data() + cursor)};
            int ret = swr_convert(_swrCtx,
                _audioFrame->data, frameSize,
                inData, frameSize);
            CHECK_ERR(ret);
        } else {
            // direct copy
            memcpy(_audioFrame->data[0], samples.data() + cursor, frameSize * sizeof(float) * _channels);
        }
        _audioFrame->pts = _audioPts;
        _audioPts += frameSize;

        // encode
        int ret = avcodec_send_frame(_audioCtx, _audioFrame);
        CHECK_ERR(ret);
        while (avcodec_receive_packet(_audioCtx, _pkt) == 0) {
            _pkt->stream_index = _audioStream->index;
            av_packet_rescale_ts(_pkt, _audioCtx->time_base, _audioStream->time_base);
            ret = av_interleaved_write_frame(_fmtCtx, _pkt);
            CHECK_ERR(ret);
            av_packet_unref(_pkt);
        }
        cursor += frameSize;
    }
}

void MP4Encoder::addOpenGLFrame(const Odin::FrameBuffer& fbo) {
    // bind FBO and read pixels
    fbo.Use();
    glReadPixels(0, 0, _width, _height, GL_RGBA, GL_UNSIGNED_BYTE, _rgbBuffer.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    int rowSize = _width * 4;  // bytes per scanline
    std::vector<uint8_t> tmp(rowSize);
    for (int y = 0; y < _height / 2; ++y) {
        uint8_t* top    = _rgbBuffer.data() + y * rowSize;
        uint8_t* bottom = _rgbBuffer.data() + (_height - 1 - y) * rowSize;
        // swap
        memcpy(tmp.data(),       top,    rowSize);
        memcpy(top,              bottom, rowSize);
        memcpy(bottom,           tmp.data(), rowSize);
    }

    // prepare source data pointers
    uint8_t* srcSlices[1] = {_rgbBuffer.data()};
    int srcStride[1] = {4 * _width};

    // convert to YUV420P
    sws_scale(_swsCtx,
        srcSlices, srcStride, 0, _height,
        _videoFrame->data, _videoFrame->linesize);

    // encode
    _videoFrame->pts = _videoPts++;
    int ret = avcodec_send_frame(_videoCtx, _videoFrame);
    CHECK_ERR(ret);
    while (avcodec_receive_packet(_videoCtx, _pkt) == 0) {
        _pkt->stream_index = _videoStream->index;
        av_packet_rescale_ts(_pkt, _videoCtx->time_base, _videoStream->time_base);
        ret = av_interleaved_write_frame(_fmtCtx, _pkt);
        CHECK_ERR(ret);
        av_packet_unref(_pkt);
    }
}

void MP4Encoder::flushEncoder(AVCodecContext* ctx, AVStream* stream) {
    avcodec_send_frame(ctx, nullptr);
    while (avcodec_receive_packet(ctx, _pkt) == 0) {
        _pkt->stream_index = stream->index;
        av_interleaved_write_frame(_fmtCtx, _pkt);
        av_packet_unref(_pkt);
    }
}

void MP4Encoder::writeTrailer() {
    av_write_trailer(_fmtCtx);
}

void MP4Encoder::finalize() {
    // flush audio & video
    flushEncoder(_audioCtx, _audioStream);
    flushEncoder(_videoCtx, _videoStream);

    writeTrailer();

    // cleanup
    avcodec_free_context(&_videoCtx);
    avcodec_free_context(&_audioCtx);
    sws_freeContext(_swsCtx);
    if (_swrCtx) swr_free(&_swrCtx);
    av_frame_free(&_videoFrame);
    av_frame_free(&_audioFrame);
    av_packet_free(&_pkt);
    if (!(_fmtCtx->oformat->flags & AVFMT_NOFILE))
        avio_close(_fmtCtx->pb);
    avformat_free_context(_fmtCtx);
    _fmtCtx = nullptr;
}
