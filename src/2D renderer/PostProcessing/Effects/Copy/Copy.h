//
// Created by victor on 08/06/25.
//

#ifndef COPY_H
#define COPY_H
#include "../Effect.h"

namespace Odin {

class Copy : public Effect {

    public:
        void Init() override;
        void Apply(FrameBuffer &fbo_in, FrameBuffer &fbo_out, int w_width, int w_height) override;
};

} // Odin

#endif //COPY_H
