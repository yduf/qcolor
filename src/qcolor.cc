#include <vips/vips8>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

#include "myFunc/elapsed.hh"

using namespace std;
using namespace vips;

struct Color {
    int r, g, b;
};

struct Box {
    int rmin, rmax, gmin, gmax, bmin, bmax;
    vector<Color> colors;
};

int getLongestDimension(Box &box) {
    int r_len = box.rmax - box.rmin;
    int g_len = box.gmax - box.gmin;
    int b_len = box.bmax - box.bmin;

    if (r_len >= g_len && r_len >= b_len)
        return 0;
    else if (g_len >= r_len && g_len >= b_len)
        return 1;
    else
        return 2;
}

void medianCut(Box &box, int depth, vector<Color> &palette, int maxDepth) {
    if (depth == maxDepth || box.colors.size() == 1) {
        Color avgColor = {0, 0, 0};
        for (Color &c : box.colors) {
            avgColor.r += c.r;
            avgColor.g += c.g;
            avgColor.b += c.b;
        }
        avgColor.r /= box.colors.size();
        avgColor.g /= box.colors.size();
        avgColor.b /= box.colors.size();
        palette.push_back(avgColor);
        return;
    }

    int dim = getLongestDimension(box);

    sort(box.colors.begin(), box.colors.end(), [dim](const Color &a, const Color &b) {
        return (dim == 0) ? a.r < b.r : (dim == 1) ? a.g < b.g : a.b < b.b;
    });

    int median = box.colors.size() / 2;

    Box left, right;

    left.colors.assign(box.colors.begin(), box.colors.begin() + median);
    right.colors.assign(box.colors.begin() + median, box.colors.end());

    left.rmin = left.gmin = left.bmin = 255;
    left.rmax = left.gmax = left.bmax = 0;
    right.rmin = right.gmin = right.bmin = 255;
    right.rmax = right.gmax = right.bmax = 0;

    for (Color &c : left.colors) {
        left.rmin = min(left.rmin, c.r);
        left.rmax = max(left.rmax, c.r);
        left.gmin = min(left.gmin, c.g);
        left.gmax = max(left.gmax, c.g);
        left.bmin = min(left.bmin, c.b);
        left.bmax = max(left.bmax, c.b);
    }

    for (Color &c : right.colors) {
        right.rmin = min(right.rmin, c.r);
        right.rmax = max(right.rmax, c.r);
        right.gmin = min(right.gmin, c.g);
        right.gmax = max(right.gmax, c.g);
        right.bmin = min(right.bmin, c.b);
        right.bmax = max(right.bmax, c.b);
    }

    medianCut(left, depth + 1, palette, maxDepth);
    medianCut(right, depth + 1, palette, maxDepth);
}

vector<Color> quantize(vector<Color> &colors, int numColors) {
    Box initialBox;
    initialBox.rmin = initialBox.gmin = initialBox.bmin = 255;
    initialBox.rmax = initialBox.gmax = initialBox.bmax = 0;
    initialBox.colors = colors;

    for (Color &c : colors) {
        initialBox.rmin = min(initialBox.rmin, c.r);
        initialBox.rmax = max(initialBox.rmax, c.r);
        initialBox.gmin = min(initialBox.gmin, c.g);
        initialBox.gmax = max(initialBox.gmax, c.g);
        initialBox.bmin = min(initialBox.bmin, c.b);
        initialBox.bmax = max(initialBox.bmax, c.b);
    }

    vector<Color> palette;
    medianCut(initialBox, 0, palette, log2(numColors));

    return palette;
}

double colorDistance(const Color &c1, const Color &c2) {
    return sqrt(pow(c1.r - c2.r, 2) + pow(c1.g - c2.g, 2) + pow(c1.b - c2.b, 2));
}

Color findClosestColor(const Color &pixel, const vector<Color> &palette) {
    Color closest = palette[0];
    double minDist = colorDistance(pixel, closest);

    for (const Color &color : palette) {
        double dist = colorDistance(pixel, color);
        if (dist < minDist) {
            minDist = dist;
            closest = color;
        }
    }
    return closest;
}

void saveQuantizedImage(const VImage &image, const vector<Color> &palette, const string &outputPath) {
    int width = image.width();
    int height = image.height();

    vector<unsigned char> outputPixels(width * height * 3);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            vector<double> pixel = image(x, y);
            Color c = {static_cast<int>(pixel[0]), static_cast<int>(pixel[1]), static_cast<int>(pixel[2])};

            Color closestColor = findClosestColor(c, palette);

            int index = (y * width + x) * 3;
            outputPixels[index] = static_cast<unsigned char>(closestColor.r);
            outputPixels[index + 1] = static_cast<unsigned char>(closestColor.g);
            outputPixels[index + 2] = static_cast<unsigned char>(closestColor.b);
        }
    }

    VImage output = VImage::new_from_memory(outputPixels.data(), width * height * 3,
                                            width, height, 3, VIPS_FORMAT_UCHAR);
    output.write_to_file(outputPath.c_str());
}


// reduce image size to be able to sample all colors in memory
// convert to quantisation color space
VImage reduceImageAndPalette( const string& input, 
                            int targetSample = 100'000,
                            bool debug=true ) {
    VImage image = VImage::new_from_file( input.c_str());
    int width = image.width();
    int height = image.height();
    cerr << "Image size " << width << "x" << height << "\n";

    if( image.get_typeof("interlaced") != 0)
      cerr << "Interlaced image, maybe too big\n";

    if( width*height > targetSample) {
        double H = sqrt( (double) height / width * targetSample);
        double W = targetSample / H;

        auto iH = round( H);
        auto iW = round( W);

        cerr << "Shrinking image to " << iW << "x" << iH << "\n";

        image = VImage::thumbnail( input.c_str(), iW,
                                            VImage::option()
                                            ->set( "size", VIPS_SIZE_FORCE)
                                            ->set( "height", iH )
                                            ->set( "no_rotate", true)
                                        );

        VImage cached = VImage::new_memory();
        image.write( cached);

        if( debug) cached.write_to_file ("reduced.ppm");

        return cached;
    }

    return image;
}

string format2s( int BandFormat) {
    switch( BandFormat) {
    default: return "unknown";
    case VIPS_FORMAT_UCHAR: return "uchar";
    case VIPS_FORMAT_CHAR:  return " char";
    }
}

vector<Color> extractColorVector( const VImage& image ) {
    vector<Color> colors;

    //
    int band = image.bands();
    int format = image.format();
    cerr << "This image has " << band << "band(s) of format " << format2s(format) << "\n";

    if( format != 0) {
        cerr << "Band format not supported\n";
    }

    if( band != 3) {
        cerr << "unexpected band number " << band << "(expected 3)\n";
    }

    // scan image by line - in case it doesn't fit in memory
    auto region = image.region();
    int width = image.width();
    int height = image.height();

    for (int y = 0; y < height; y++) {
        auto row = VipsRect{ 0, y, width, 1 };
        region.prepare( &row);

        auto* p = (uint8_t*) region.addr( 0, y);
        for (int x = 0; x < width; x++) {
            int r = *p++;
            int g = *p++;
            int b = *p++;

            colors.push_back( Color{ r, g, b});
        }
    }

    return colors;
}

int main(int argc, char **argv) {
    if (VIPS_INIT(argv[0])) {
        vips_error_exit(nullptr);
        return 1;
    }

    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <input image> <number of colors> <output image>" << endl;
        return 1;
    }

    VImage image = reduceImageAndPalette(argv[1]);
    vector<Color> colors = extractColorVector( image);

    auto stop = elapsed();
    start = stop.now;

    cerr << "Load & Color extract " << stop.ms_used << "ms\n";
    int numColors = stoi(argv[2]);
    vector<Color> palette = quantize(colors, numColors);

    stop = elapsed();
    start = stop.now;
    cerr << "Color quantisation " << stop.ms_used << "ms\n";

    // Display the palette
    for(Color &c : palette) {
        cout << "Color: (" << c.r << ", " << c.g << ", " << c.b << ")" << endl;
    }

    // this is very slow
    if( false) {
    saveQuantizedImage(image, palette, argv[3]);

    stop = elapsed();
    cerr << "Saved " << stop.ms_used << "ms\n";
    cerr << "Quantized image saved to " << argv[3] << endl;
    }

    vips_shutdown();

    return 0;
}
