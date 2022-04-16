# TermCam
## What is it?
TermCam is an application that allows user to play footage from webcam in terminal.
## How to use
Clone this repository and compile with: `g++ termcam.cpp -ljpeg -lz -o termcam`

Run with:
- `./termcam -s` - to stream
- `./termcam -r <number of frames> <filename>` - to stream and record a video
- `./termcam -p <filename>` - to play a recorded video

Examples: 
- `./termcam -s`
- `./termcam -r 20 file.rec`
- `./termcam -p file.rec`

## Libraries
TermCam uses CImg (http://cimg.eu/). 

The only thing you need to do is have the CImg.h file (download from website above) in the same directory as termcam.cpp and compile with `-ljpeg`, `-lz` flags.
