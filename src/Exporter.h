//
// Created by victor on 29/05/25.
//

#ifndef EXPORTER_H
#define EXPORTER_H

#include "JsonHandler.h"

inline const std::string tab = "    ";
inline const std::string new_line = "\n";

// TODO : fill placeholders
inline const char* python_header = R""""(from Sequencer.builder_utils import *

metadata = {
    "title"      : "placeholder",
    "music"      : "./Music/le_z.mp3",
    "priority "  : -1
}

def build():
    reset_builder()

)"""";

std::string get_python_interpolation(const GradientKind& kind);

std::string get_python_color(const Color& color);

int sample_to_ms(int sample, int sample_rate);

std::string get_group_str(size_t group_id);

std::string get_python_command(const Command& command, int sample_rate);

// Important : the keyframe list must be sorted, and every command must be retimed before generating the python script
std::string generate_python_script(const ProjectData& data);

void save_python_script(const std::string& path, const std::string& script);



#endif //EXPORTER_H
