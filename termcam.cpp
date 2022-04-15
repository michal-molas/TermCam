// Build with: g++ termcam.cpp -ljpeg -lz -o termcam

#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#define cimg_use_jpeg  // Do this if you want CImg to process JPEGs itself
                       // without resorting to ImageMagick
// Do this if you don't need a GUI and don't want to link to GDI32 or X11
#define cimg_display 0

#include "CImg.h"

using namespace cimg_library;
using namespace std;

struct Pixel {
    unsigned int r;
    unsigned int g;
    unsigned int b;

    Pixel &operator+=(const Pixel &other) {
        r += other.r;
        g += other.g;
        b += other.b;
        return *this;
    }

    Pixel &operator/=(unsigned int n) {
        r /= n;
        g /= n;
        b /= n;
        return *this;
    }

    std::string print() const {
        std::string p = "\033[48;2;";
        p += to_string(r);
        p += ";";
        p += to_string(g);
        p += ";";
        p += to_string(b);
        p += "m ";
        return p;
    }
};

using frame_t = std::vector<std::vector<Pixel>>;

const size_t WIDTH = 640;
const size_t HEIGHT = 480 - 20;

const size_t H_SCALE = 10;
const size_t W_SCALE = 5;

void print_frame(const frame_t &frame) {
    std::string picture = "\033[48;2;0;0;0m";

    for (size_t i = 0; i < frame.size(); i++) {
        for (size_t j = frame[0].size(); j-- > 0;) {
            picture += frame[i][j].print();
        }
        picture += "\033[48;2;0;0;0m\n";
    }
    picture += "\033[48;2;0;0;0m\n";
    system("clear");
    std::cout << picture;
}

void normalize(frame_t &frame) {
    for (size_t i = 0; i < frame.size(); i++) {
        for (size_t j = 0; j < frame[0].size(); j++) {
            frame[i][j] /= H_SCALE * W_SCALE;
        }
    }
}

void shoot_frame(frame_t &frame) {
    // Take a picture
    system("mkdir temp 2>/dev/null");
    system("fswebcam -r 640x480 --jpeg 85 -D 0 temp/web-cam-shot.jpg --quiet");

    // Load image
    CImg<unsigned char> img("temp/web-cam-shot.jpg");

    // Delete the picture
    system("rm temp/web-cam-shot.jpg");

    // Dump all pixels
    for (size_t y = 0; y < HEIGHT; y++) {
        for (size_t x = 0; x < WIDTH; x++) {
            frame[y / H_SCALE][x / W_SCALE] +=
                Pixel({img(x, y, 0), img(x, y, 1), img(x, y, 2)});
        }
    }

    normalize(frame);
}

void write_frame(std::ofstream &os, const frame_t &frame) {
    for (size_t i = 0; i < frame.size(); i++) {
        for (size_t j = 0; j < frame[0].size(); j++) {
            os << frame[i][j].r << " " << frame[i][j].g << " " << frame[i][j].b
               << "\n";
        }
    }
}

bool read_frame(std::ifstream &is, frame_t &frame) {
    for (size_t i = 0; i < frame.size(); i++) {
        for (size_t j = 0; j < frame[0].size(); j++) {
            if (!(is >> frame[i][j].r)) return false;
            is >> frame[i][j].g >> frame[i][j].b;
        }
    }

    return true;
}

const std::string USAGE_MSG =
    "Usage:\n\t./termcam -s\n\t./termcam -r <number of frames> "
    "<filename>\n\t./termcam -p <filename>";

int main(int argc, char **argv) {
    std::vector<Pixel> row(WIDTH / W_SCALE);
    frame_t frame(HEIGHT / H_SCALE, row);

    if (argc == 4 && !strcmp(argv[1], "-r")) {  // record
        system("mkdir records 2>/dev/null");
        std::string filename = "records/" + std::string(argv[3]);
        std::ofstream out_file(filename);
        size_t no_frames = std::stoul(argv[2]);

        for (size_t i = 0; i < no_frames; i++) {
            shoot_frame(frame);
            write_frame(out_file, frame);
            print_frame(frame);
        }
        // TODO: handling ctrl + c
    } else if (argc == 3 && !strcmp(argv[1], "-p")) {  // play
        std::ifstream in_file(std::string("records/") + std::string(argv[2]));
        if (!in_file.is_open()) {
            std::cerr << "File doesn't exist" << std::endl;
            exit(EXIT_FAILURE);
        }
        while (read_frame(in_file, frame)) {
            usleep(500000);
            print_frame(frame);
        }
        // TODO: safer reading (eg. by number of frames in first line)
    } else if (argc == 2 && !strcmp(argv[1], "-s")) {  // stream
        for (;;) {
            shoot_frame(frame);
            print_frame(frame);
        }
    } else {
        std::cerr << USAGE_MSG << std::endl;
        exit(EXIT_FAILURE);
    }
}