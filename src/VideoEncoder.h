#ifndef VIDEO_ENCODER_H
#define VIDEO_ENCODER_H

#include <vector>
#include <string>
#include <cstdint>
#include "2D renderer/Framebuffer.h"

// Forward declarations to avoid including FFmpeg headers in header file
struct AVFormatContext;
struct AVCodecContext;
struct AVStream;
struct AVFrame;
struct AVPacket;
struct SwsContext;

class VideoEncoder {
public:
    VideoEncoder();
    ~VideoEncoder();

    // Delete copy constructor and assignment operator
    VideoEncoder(const VideoEncoder&) = delete;
    VideoEncoder& operator=(const VideoEncoder&) = delete;

    /**
     * Initialize the video encoder
     * @param filename Output video file path
     * @param width Video width in pixels
     * @param height Video height in pixels
     * @param fps Frames per second
     * @param bitrate Video bitrate (default: 4Mbps)
     * @return true if initialization successful, false otherwise
     */
    bool initialize(const std::string& filename, int width, int height, int fps, int bitrate = 4000000);

    /**
     * Add a frame to the video from RGB pixel data
     * @param rgbData Pointer to RGB pixel data (width * height * 3 bytes)
     * @param width Frame width
     * @param height Frame height
     * @return true if frame added successfully, false otherwise
     */
    bool addFrame(const uint8_t* rgbData, int width, int height);

    /**
     * Add a frame to the video from OpenGL framebuffer
     * Automatically handles pixel reading and vertical flipping
     * @param width Framebuffer width
     * @param height Framebuffer height
     * @return true if frame added successfully, false otherwise
     */
    bool addFrameFromOpenGL(Odin::FrameBuffer& fbo, int width, int height);

    /**
     * Finalize the video encoding and write the file
     * Must be called before destruction to properly close the video file
     */
    void finalize();

    /**
     * Check if the encoder is currently initialized and ready
     * @return true if encoder is ready, false otherwise
     */
    bool isInitialized() const;

    /**
     * Get the current frame count
     * @return Number of frames added to the video
     */
    int getFrameCount() const;

private:
    // FFmpeg contexts
    AVFormatContext* m_formatContext;
    AVCodecContext* m_codecContext;
    AVStream* m_videoStream;
    AVFrame* m_frame;
    AVPacket* m_packet;
    SwsContext* m_swsContext;

    // Video parameters
    int m_width;
    int m_height;
    int m_frameCount;
    bool m_initialized;

    // Temporary buffer for pixel data manipulation
    std::vector<uint8_t> m_pixelBuffer;

    // Internal helper methods
    bool setupEncoder(const std::string& filename, int fps, int bitrate);
    bool setupFrame();
    bool encodeFrame();
    void cleanup();
    void flipImageVertically(const uint8_t* src, uint8_t* dst, int width, int height);
};

#endif // VIDEO_ENCODER_H
