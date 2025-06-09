#ifndef MP4ENCODER_H
#define MP4ENCODER_H

#include <string>
#include <vector>
#include <stdexcept>

#include "../libs/glad/include/glad/glad.h"         // for GLuint7
#include "2D renderer/Framebuffer.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

class MP4Encoder {
public:
    /**
     * @param filename   Output MP4 file path
     * @param width      Video width (even number)
     * @param height     Video height (even number)
     * @param fps        Frames per second
     * @param sample_rate Audio sample rate (Hz)
     * @param channels   Number of audio channels (1=mono,2=stereo)
     */
    MP4Encoder(const std::string& filename,
               int width, int height, int fps,
               int sample_rate, int channels = 1);
    ~MP4Encoder();

    /**
     * Feed audio samples into encoder. samples.size() can be arbitrary; internal buffering handles frame-size slicing.
     */
    void addAudio(const std::vector<float>& samples);

    /**
     * Capture and encode one OpenGL frame from given FBO.
     *
     * @param fbo     Framebuffer object ID
     */
    void addOpenGLFrame(const Odin::FrameBuffer& fbo);

    /**
     * Finalize encoding, flush encoders, and close file.
     */
    void finalize();

private:
    void initOutput();
    void initVideoStream();
    void initAudioStream();
    void writeHeader();
    void writeTrailer();
    void flushEncoder(AVCodecContext* ctx, AVStream* stream);

    std::string _filename;
    int _width, _height;
    int _fps;
    int _sampleRate;
    int _channels;

    AVFormatContext* _fmtCtx = nullptr;
    AVStream* _videoStream = nullptr;
    AVStream* _audioStream = nullptr;
    AVCodecContext* _videoCtx = nullptr;
    AVCodecContext* _audioCtx = nullptr;
    SwsContext* _swsCtx = nullptr;
    SwrContext* _swrCtx = nullptr;

    AVFrame* _videoFrame = nullptr;
    AVFrame* _audioFrame = nullptr;
    AVPacket* _pkt = nullptr;

    int64_t _videoPts = 0;
    int64_t _audioPts = 0;

    std::vector<uint8_t> _rgbBuffer;
};

// Utility macro for error checking
inline char err_buf[AV_ERROR_MAX_STRING_SIZE];
#define CHECK_ERR(err) if (err < 0) throw std::runtime_error(av_make_error_string(err_buf, AV_ERROR_MAX_STRING_SIZE, err));

#endif // MP4ENCODER_H
