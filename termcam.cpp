// Build with: g++ termcam.cpp -ljpeg -lz -o termcam

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

struct Pixel {
    unsigned int r;
    unsigned int g;
    unsigned int b;

    Pixel &operator=(unsigned int n) {
        r = 0;
        g = 0;
        b = 0;
        return *this;
    }

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
        p += std::to_string(r);
        p += ";";
        p += std::to_string(g);
        p += ";";
        p += std::to_string(b);
        p += "m ";
        return p;
    }
};

class Frame {
   private:
    const size_t WIDTH = 640;
    const size_t HEIGHT = 480 - 20;

    const size_t H_SCALE = 10;
    const size_t W_SCALE = 5;

    std::vector<std::vector<Pixel>> pixels;

    void reset() {
        for (size_t i = 0; i < pixels.size(); i++) {
            for (size_t j = 0; j < pixels[0].size(); j++) {
                pixels[i][j] = 0;
            }
        }
    }

    void compress() {
        for (size_t i = 0; i < pixels.size(); i++) {
            for (size_t j = 0; j < pixels[0].size(); j++) {
                pixels[i][j] /= H_SCALE * W_SCALE;
            }
        }
    }

   public:
    Frame() {
        std::vector<Pixel> row(WIDTH / W_SCALE);
        pixels = std::vector<std::vector<Pixel>>(HEIGHT / H_SCALE, row);
    }

    void print() {
        std::string picture = "\033[48;2;0;0;0m";

        for (size_t i = 0; i < pixels.size(); i++) {
            for (size_t j = pixels[0].size(); j-- > 0;) {
                picture += pixels[i][j].print();
            }
            picture += "\033[48;2;0;0;0m\n";
        }
        system("clear");
        std::cout << picture;
    }

    void shoot() {
        // Take a picture
        system("mkdir temp 2>/dev/null");
        system(
            "fswebcam -r 640x480 --jpeg 85 -D 0 temp/web-cam-shot.jpg --quiet");

        // Load image
        cimg_library::CImg<unsigned char> img("temp/web-cam-shot.jpg");

        // Delete the picture
        system("rm temp/web-cam-shot.jpg");

        reset();
        // Dump all pixels
        for (size_t y = 0; y < HEIGHT; y++) {
            for (size_t x = 0; x < WIDTH; x++) {
                pixels[y / H_SCALE][x / W_SCALE] +=
                    Pixel({img(x, y, 0), img(x, y, 1), img(x, y, 2)});
            }
        }

        compress();
    }

    void write(std::ofstream &os) {
        for (size_t i = 0; i < pixels.size(); i++) {
            for (size_t j = 0; j < pixels[0].size(); j++) {
                os << pixels[i][j].r << " " << pixels[i][j].g << " "
                   << pixels[i][j].b << "\n";
            }
        }
    }

    bool read(std::ifstream &is) {
        reset();
        for (size_t i = 0; i < pixels.size(); i++) {
            for (size_t j = 0; j < pixels[0].size(); j++) {
                if (!(is >> pixels[i][j].r)) return false;
                is >> pixels[i][j].g >> pixels[i][j].b;
            }
        }

        return true;
    }
};

const std::string USAGE_MSG =
    "Usage:\n\t./termcam -s\n\t./termcam -r <number of frames> "
    "<filename>\n\t./termcam -p <filename>";

int main(int argc, char **argv) {
    Frame frame;

    if (argc == 4 && !strcmp(argv[1], "-r")) {  // record
        system("mkdir records 2>/dev/null");
        std::string filename = "records/" + std::string(argv[3]);
        std::ofstream out_file(filename);
        size_t no_frames = std::stoul(argv[2]);

        for (size_t i = 0; i < no_frames; i++) {
            frame.shoot();
            frame.write(out_file);
            frame.print();
        }
    } else if (argc == 3 && !strcmp(argv[1], "-p")) {  // play
        std::ifstream in_file(std::string("records/") + std::string(argv[2]));
        if (!in_file.is_open()) {
            std::cerr << "File doesn't exist" << std::endl;
            exit(EXIT_FAILURE);
        }
        while (frame.read(in_file)) {
            usleep(500000);
            frame.print();
        }
    } else if (argc == 2 && !strcmp(argv[1], "-s")) {  // stream
        for (;;) {
            frame.shoot();
            frame.print();
        }
    } else {
        std::cerr << USAGE_MSG << std::endl;
        exit(EXIT_FAILURE);
    }
}