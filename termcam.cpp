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

namespace {
// Outputs error message and exits with code 1
void fatal(const std::string &msg) {
    std::cerr << msg << std::endl;
    exit(EXIT_FAILURE);
}

// Commands and paths
const char *MK_TEMP_CMD = "mkdir temp 2>/dev/null";
const char *PHOTO_CMD =
    "fswebcam --no-banner -r 640x480 --flip h  --jpeg 85 -D 0 "
    "temp/web-cam-shot.jpg -q";
const char *RM_PHOTO_CMD = "rm temp/web-cam-shot.jpg";
const char *CLEAR_CMD = "clear";
const char *MK_RECORDS_CMD = "mkdir records 2>/dev/null";

// Paths
const char *PHOTO_PATH = "temp/web-cam-shot.jpg";
const std::string RECORDS_DIR = "records/";

// Error messages
const std::string USAGE_MSG =
    "Usage:\n\t./termcam -s\n\t./termcam -r <number of frames> "
    "<filename>\n\t./termcam -p <filename>";
const std::string FILE_NOT_FOUND = "File doesn't exist";
const std::string WRONG_NO_FRAMES = "Incorrect number of frames";
const std::string TOO_MANY_FRAMES =
    "Number of frames can't be greater than 500";
const std::string FAIL_CREATE_FILE = "Failed to create a file";

// Colors
const char *PIXEL_TEMPLATE = "\033[48;2;%u;%u;%um";
const std::string BACKGROUND = "\033[48;2;0;0;0m";
}  // namespace

class Pixel {
   private:
    unsigned int r;
    unsigned int g;
    unsigned int b;

   public:
    Pixel(unsigned int r, unsigned int g, unsigned int b) : r(r), g(g), b(b) {}
    Pixel() : r(0), g(0), b(0) {}

    Pixel &operator=(unsigned int n) {
        r = n;
        g = n;
        b = n;
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

    friend std::ofstream &operator<<(std::ofstream &os, const Pixel &p);

    friend std::ifstream &operator>>(std::ifstream &is, Pixel &p);

    // Returns a string which is a colored space
    std::string print() const {
        char buff[17];  // 17 is the size of PIXEL_TEMPLATE
        sprintf(buff, PIXEL_TEMPLATE, r, g, b);
        return std::string(buff) + std::string(" ");
    }
};

std::ofstream &operator<<(std::ofstream &os, const Pixel &p) {
    os << p.r << " " << p.g << " " << p.b;
    return os;
}

std::ifstream &operator>>(std::ifstream &is, Pixel &p) {
    is >> p.r >> p.g >> p.b;
    return is;
}

class Frame {
   private:
    // Original size of the picture
    const size_t WIDTH = 640;
    const size_t HEIGHT = 480;

    // How many original pixels fit in one frame pixel
    // Should be a divisor of acorrdingly WIDTH or HEIGHT
    const size_t W_COMPR = 5;
    const size_t H_COMPR = 10;

    std::vector<std::vector<Pixel>> pixels;

    // Sets all pixels to black
    void reset() {
        for (size_t i = 0; i < pixels.size(); i++)
            for (size_t j = 0; j < pixels[0].size(); j++) pixels[i][j] = 0;
    }

    // Averages the pixels
    void compress() {
        for (size_t i = 0; i < pixels.size(); i++) {
            for (size_t j = 0; j < pixels[0].size(); j++) {
                pixels[i][j] /= H_COMPR * W_COMPR;
            }
        }
    }

    // Prints the frame in terminal
    void print() {
        std::string picture = BACKGROUND;

        for (size_t i = 0; i < pixels.size(); i++) {
            for (size_t j = 0; j < pixels[0].size(); j++)
                picture += pixels[i][j].print();

            picture += BACKGROUND + std::string("\n");
        }
        system(CLEAR_CMD);
        std::cout << picture;
    }

    // Takes a picture and compresses it
    void shoot() {
        // Take a picture
        system(MK_TEMP_CMD);
        system(PHOTO_CMD);

        // Load image
        cimg_library::CImg<unsigned char> img(PHOTO_PATH);

        // Delete the picture
        system(RM_PHOTO_CMD);

        reset();
        for (size_t y = 0; y < HEIGHT; y++) {
            for (size_t x = 0; x < WIDTH; x++) {
                pixels[y / H_COMPR][x / W_COMPR] +=
                    Pixel(img(x, y, 0), img(x, y, 1), img(x, y, 2));
            }
        }

        compress();
    }

    // Encodes the frame in a recording file
    void write(std::ofstream &os) {
        for (size_t i = 0; i < pixels.size(); i++)
            for (size_t j = 0; j < pixels[0].size(); j++)
                os << pixels[i][j] << std::endl;
    }

    // Reads a frame from a recording file
    bool read(std::ifstream &is) {
        reset();
        for (size_t i = 0; i < pixels.size(); i++)
            for (size_t j = 0; j < pixels[0].size(); j++)
                if (!(is >> pixels[i][j])) return false;

        return true;
    }

   public:
    Frame() {
        std::vector<Pixel> row(WIDTH / W_COMPR);
        pixels = std::vector<std::vector<Pixel>>(HEIGHT / H_COMPR, row);
    }

    // Plays a recording
    void play(std::ifstream &is) {
        while (read(is)) {
            // Has to sleep because reading is much faster than recording
            usleep(500000);
            print();
        }
    }

    // Streams no_frames frames from camera and saves them in a recording file
    void record(std::ofstream &os, size_t no_frames) {
        for (size_t i = 0; i < no_frames; i++) {
            shoot();
            write(os);
            print();
        }
    }

    // Streams frames from camera
    void stream() {
        for (;;) {
            shoot();
            print();
        }
    }
};

int main(int argc, char **argv) {
    Frame frame;

    if (argc == 4 && !strcmp(argv[1], "-r")) {  // record
        system(MK_RECORDS_CMD);

        std::string filename = RECORDS_DIR + std::string(argv[3]);
        std::ofstream out_file(filename);
        if (!out_file.is_open()) fatal(FAIL_CREATE_FILE);

        size_t no_frames;
        try {
            no_frames = std::stoul(argv[2]);
        } catch (...) {
            fatal(WRONG_NO_FRAMES);
        }

        if (no_frames > 500) fatal(TOO_MANY_FRAMES);

        frame.record(out_file, no_frames);
    } else if (argc == 3 && !strcmp(argv[1], "-p")) {  // play
        std::ifstream in_file(RECORDS_DIR + std::string(argv[2]));
        if (!in_file.is_open()) fatal(FILE_NOT_FOUND);

        frame.play(in_file);
    } else if (argc == 2 && !strcmp(argv[1], "-s")) {  // stream
        frame.stream();
    } else {
        fatal(USAGE_MSG);
    }
}