#include "VideoEncoder.h"
#include <iostream>
#include <cstring>

#include "../libs/glad/include/glad/glad.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

VideoEncoder::VideoEncoder()
    : m_formatContext(nullptr)
    , m_codecContext(nullptr)
    , m_videoStream(nullptr)
    , m_frame(nullptr)
    , m_packet(nullptr)
    , m_swsContext(nullptr)
    , m_width(0)
    , m_height(0)
    , m_frameCount(0)
    , m_initialized(false)
{
    // In FFmpeg 4.0+, codecs and formats are automatically registered
    // No manual initialization needed
}

VideoEncoder::~VideoEncoder() {
    cleanup();
}

bool VideoEncoder::initialize(const std::string& filename, int width, int height, int fps, int bitrate) {
    if (m_initialized) {
        std::cerr << "VideoEncoder: Already initialized" << std::endl;
        return false;
    }
    
    m_width = width;
    m_height = height;
    m_frameCount = 0;
    
    // Allocate pixel buffer for vertical flipping
    m_pixelBuffer.resize(width * height * 3);
    
    if (!setupEncoder(filename, fps, bitrate)) {
        cleanup();
        return false;
    }
    
    if (!setupFrame()) {
        cleanup();
        return false;
    }
    
    m_initialized = true;
    return true;
}

bool VideoEncoder::setupEncoder(const std::string& filename, int fps, int bitrate) {
    // Create output format context
    if (avformat_alloc_output_context2(&m_formatContext, nullptr, nullptr, filename.c_str()) < 0) {
        std::cerr << "VideoEncoder: Could not create output context" << std::endl;
        return false;
    }
    
    // Find H.264 encoder
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "VideoEncoder: H.264 codec not found" << std::endl;
        return false;
    }
    
    // Create video stream
    m_videoStream = avformat_new_stream(m_formatContext, nullptr);
    if (!m_videoStream) {
        std::cerr << "VideoEncoder: Could not create video stream" << std::endl;
        return false;
    }
    
    // Create codec context
    m_codecContext = avcodec_alloc_context3(codec);
    if (!m_codecContext) {
        std::cerr << "VideoEncoder: Could not allocate codec context" << std::endl;
        return false;
    }
    
    // Set codec parameters
    m_codecContext->bit_rate = bitrate;
    m_codecContext->width = m_width;
    m_codecContext->height = m_height;
    m_codecContext->time_base = {1, fps};
    m_codecContext->framerate = {fps, 1};
    m_codecContext->gop_size = 10;
    m_codecContext->max_b_frames = 1;
    m_codecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    
    // Some formats want stream headers to be separate
    if (m_formatContext->oformat->flags & AVFMT_GLOBALHEADER) {
        m_codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    
    // Set H.264 specific options for better compression
    av_opt_set(m_codecContext->priv_data, "preset", "medium", 0);
    av_opt_set(m_codecContext->priv_data, "crf", "23", 0);
    
    // Open codec
    if (avcodec_open2(m_codecContext, codec, nullptr) < 0) {
        std::cerr << "VideoEncoder: Could not open codec" << std::endl;
        return false;
    }
    
    // Copy codec parameters to stream
    if (avcodec_parameters_from_context(m_videoStream->codecpar, m_codecContext) < 0) {
        std::cerr << "VideoEncoder: Could not copy codec parameters" << std::endl;
        return false;
    }
    
    // Initialize SwsContext for RGB to YUV conversion
    m_swsContext = sws_getContext(m_width, m_height, AV_PIX_FMT_RGB24,
                                 m_width, m_height, AV_PIX_FMT_YUV420P,
                                 SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!m_swsContext) {
        std::cerr << "VideoEncoder: Could not initialize SwsContext" << std::endl;
        return false;
    }
    
    // Open output file
    if (avio_open(&m_formatContext->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0) {
        std::cerr << "VideoEncoder: Could not open output file: " << filename << std::endl;
        return false;
    }
    
    // Write file header
    if (avformat_write_header(m_formatContext, nullptr) < 0) {
        std::cerr << "VideoEncoder: Error writing header" << std::endl;
        return false;
    }
    
    return true;
}

bool VideoEncoder::setupFrame() {
    // Allocate frame
    m_frame = av_frame_alloc();
    if (!m_frame) {
        std::cerr << "VideoEncoder: Could not allocate frame" << std::endl;
        return false;
    }
    
    m_frame->format = m_codecContext->pix_fmt;
    m_frame->width = m_codecContext->width;
    m_frame->height = m_codecContext->height;
    
    if (av_frame_get_buffer(m_frame, 0) < 0) {
        std::cerr << "VideoEncoder: Could not allocate frame data" << std::endl;
        return false;
    }
    
    // Allocate packet
    m_packet = av_packet_alloc();
    if (!m_packet) {
        std::cerr << "VideoEncoder: Could not allocate packet" << std::endl;
        return false;
    }
    
    return true;
}

bool VideoEncoder::addFrame(const uint8_t* rgbData, int width, int height) {
    if (!m_initialized) {
        std::cerr << "VideoEncoder: Not initialized" << std::endl;
        return false;
    }
    
    if (width != m_width || height != m_height) {
        std::cerr << "VideoEncoder: Frame size mismatch" << std::endl;
        return false;
    }
    
    // Make frame writable
    if (av_frame_make_writable(m_frame) < 0) {
        std::cerr << "VideoEncoder: Could not make frame writable" << std::endl;
        return false;
    }
    
    // Convert RGB to YUV
    const uint8_t* srcData[1] = { rgbData };
    int srcLinesize[1] = { width * 3 };
    
    sws_scale(m_swsContext, srcData, srcLinesize, 0, height,
              m_frame->data, m_frame->linesize);
    
    m_frame->pts = m_frameCount++;
    
    return encodeFrame();
}

bool VideoEncoder::addFrameFromOpenGL(Odin::FrameBuffer &fbo, int width, int height) {
    if (!m_initialized) {
        std::cerr << "VideoEncoder: Not initialized" << std::endl;
        return false;
    }
    
    if (width != m_width || height != m_height) {
        std::cerr << "VideoEncoder: Frame size mismatch" << std::endl;
        return false;
    }
    
    // Read pixels from OpenGL framebuffer
    std::vector<uint8_t> pixels(width * height * 3);
    fbo.Use();
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    // Check for OpenGL errors
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "VideoEncoder: OpenGL error reading pixels: " << error << std::endl;
        return false;
    }
    
    // Flip image vertically (OpenGL is bottom-up, video is top-down)
    flipImageVertically(pixels.data(), m_pixelBuffer.data(), width, height);
    
    return addFrame(m_pixelBuffer.data(), width, height);
}

bool VideoEncoder::encodeFrame() {
    // Send frame to encoder
    int ret = avcodec_send_frame(m_codecContext, m_frame);
    if (ret < 0) {
        std::cerr << "VideoEncoder: Error sending frame to encoder" << std::endl;
        return false;
    }
    
    // Receive encoded packets
    while (ret >= 0) {
        ret = avcodec_receive_packet(m_codecContext, m_packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            // Need more input or end of stream
            break;
        } else if (ret < 0) {
            std::cerr << "VideoEncoder: Error encoding frame" << std::endl;
            return false;
        }
        
        // Scale packet timing
        av_packet_rescale_ts(m_packet, m_codecContext->time_base, m_videoStream->time_base);
        m_packet->stream_index = m_videoStream->index;
        
        // Write packet
        if (av_interleaved_write_frame(m_formatContext, m_packet) < 0) {
            std::cerr << "VideoEncoder: Error writing packet" << std::endl;
            av_packet_unref(m_packet);
            return false;
        }
        
        av_packet_unref(m_packet);
    }
    
    return true;
}

void VideoEncoder::finalize() {
    if (!m_initialized) {
        return;
    }
    
    // Flush encoder by sending null frame
    if (m_codecContext) {
        avcodec_send_frame(m_codecContext, nullptr);
        
        int ret;
        while ((ret = avcodec_receive_packet(m_codecContext, m_packet)) >= 0) {
            av_packet_rescale_ts(m_packet, m_codecContext->time_base, m_videoStream->time_base);
            m_packet->stream_index = m_videoStream->index;
            av_interleaved_write_frame(m_formatContext, m_packet);
            av_packet_unref(m_packet);
        }
    }
    
    // Write file trailer
    if (m_formatContext) {
        av_write_trailer(m_formatContext);
    }
    
    cleanup();
    m_initialized = false;
}

bool VideoEncoder::isInitialized() const {
    return m_initialized;
}

int VideoEncoder::getFrameCount() const {
    return m_frameCount;
}

void VideoEncoder::flipImageVertically(const uint8_t* src, uint8_t* dst, int width, int height) {
    int rowSize = width * 3; // RGB = 3 bytes per pixel
    for (int y = 0; y < height; y++) {
        const uint8_t* srcRow = src + (height - 1 - y) * rowSize;
        uint8_t* dstRow = dst + y * rowSize;
        std::memcpy(dstRow, srcRow, rowSize);
    }
}

void VideoEncoder::cleanup() {
    if (m_swsContext) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }
    
    if (m_frame) {
        av_frame_free(&m_frame);
    }
    
    if (m_packet) {
        av_packet_free(&m_packet);
    }
    
    if (m_codecContext) {
        avcodec_free_context(&m_codecContext);
    }
    
    if (m_formatContext) {
        if (m_formatContext->pb) {
            avio_closep(&m_formatContext->pb);
        }
        avformat_free_context(m_formatContext);
        m_formatContext = nullptr;
    }
    
    m_pixelBuffer.clear();
}