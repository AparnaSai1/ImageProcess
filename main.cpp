#include "manipulation.h"
#include <iostream>
#include <fstream>
using namespace std;

namespace fs = std::filesystem;

vector<uint8_t> ImageManipulator::ReadTGA(const string& filename, TGAHeader& header) {
    ifstream inputFile(filename, ios::binary);
    if (!inputFile.is_open()) {
        cout << "Errorr!" << filename << endl;
        return {};
    }

    inputFile.read(reinterpret_cast<char*>(&header), sizeof(TGAHeader)); //read the header
    if (header.bitsPerPixel != 8 && header.bitsPerPixel != 24 && header.bitsPerPixel != 32) {
        cout << "Unsupported image format! if its not 24 or 32" << endl;
        return {};
    }
    inputFile.ignore(header.idLength);
    const size_t bytesPerPixel = header.bitsPerPixel / 8;
    const size_t imageSize = header.width * header.height * bytesPerPixel;
    vector<uint8_t> imageData(imageSize);
    inputFile.read(reinterpret_cast<char*>(imageData.data()), imageSize);
    inputFile.close();
    return imageData;
}


bool ImageManipulator::WriteTGA(const string& filename, const vector<uint8_t>& imageData, const TGAHeader& header)
{
    ofstream outputFile(filename, ios::binary); //ofstream bc output
    if (!outputFile.is_open()) {
        cout << "Error opening file: " << filename << endl;
        return false;
    }

    outputFile.write(reinterpret_cast<const char*>(&header), sizeof(TGAHeader)); //writes the header
    outputFile.write(reinterpret_cast<const char*>(imageData.data()), imageData.size()); //writes the pixel data
    outputFile.close(); //closes file
    return true;
}

vector<uint8_t> ImageManipulator::Multiply(const vector<uint8_t>& topLayer, const vector<uint8_t>& bottomLayer)
{
    size_t imageSize = topLayer.size(); //does the math to multiply the layers
    vector<uint8_t> blendedImage(imageSize);
    for (size_t i = 0; i < imageSize; i += 3)
    {
        float b1 = bottomLayer[i] / 255.0f;
        float g1 = bottomLayer[i + 1] / 255.0f;
        float r1 = bottomLayer[i + 2] / 255.0f;

        float b2 = topLayer[i] / 255.0f;
        float g2 = topLayer[i + 1] / 255.0f;
        float r2 = topLayer[i + 2] / 255.0f;

        float b3 = b1 * b2;
        float g3 = g1 * g2;
        float r3 = r1 * r2;

        // Clamp the values to the range [0, 1]
        b3 = max(0.0f, min(b3, 1.0f));
        g3 = max(0.0f, min(g3, 1.0f));
        r3 = max(0.0f, min(r3, 1.0f));

        // Convert back to the range [0, 255]
        uint8_t b3Byte = static_cast<uint8_t>(b3 * 255 + 0.5f);
        uint8_t g3Byte = static_cast<uint8_t>(g3 * 255 + 0.5f);
        uint8_t r3Byte = static_cast<uint8_t>(r3 * 255 + 0.5f);

        blendedImage[i] = b3Byte;
        blendedImage[i + 1] = g3Byte;
        blendedImage[i + 2] = r3Byte;
    }

    return blendedImage;
}

vector<uint8_t> ImageManipulator::SubtractBlend(const vector<uint8_t>& bottomLayer, const vector<uint8_t>& topLayer) {
    vector<uint8_t> blendedImage;

    // Check if the layer sizes match
    if (bottomLayer.size() != topLayer.size()) {
        cout << "Error: Layer sizes do not match." << endl;
        return blendedImage;
    }

    // Subtract the pixel values
    blendedImage.reserve(bottomLayer.size());
    for (size_t i = 0; i < bottomLayer.size(); i++) {
        uint8_t result = (topLayer[i] > bottomLayer[i]) ? (topLayer[i] - bottomLayer[i]) : 0;
        blendedImage.push_back(result);
    }

    return blendedImage;
}

//Screen Implementation

vector<uint8_t> ImageManipulator::Screen(const vector<uint8_t>& bottomLayer, const vector<uint8_t>& topLayer) {
    size_t imageSize = bottomLayer.size();
    vector<uint8_t> blendedImage(imageSize);

    for (size_t i = 0; i < imageSize; i++) {
        float b1 = bottomLayer[i] / 255.0f;
        float b2 = topLayer[i] / 255.0f;

        float blendedPixel = 1.0f - ((1.0f - b1) * (1.0f - b2));

        // Convert the blended pixel back to the range [0, 255]
        uint8_t blendedPixelByte = static_cast<uint8_t>(blendedPixel * 255.0f + 0.5f);

        blendedImage[i] = blendedPixelByte;
    }

    return blendedImage;
}

//Overlay Implentation

vector<uint8_t> ImageManipulator::Overlay(const vector<uint8_t>& bottomLayer, const vector<uint8_t>& topLayer) {
    size_t imageSize = bottomLayer.size();
    vector<uint8_t> blendedImage(imageSize);

    for (size_t i = 0; i < imageSize; i++) {
        float p1 = bottomLayer[i] / 255.0f;
        float p2 = topLayer[i] / 255.0f;

        float blendedPixel;
        if (p2 <= 0.5f) {
            blendedPixel = 2.0f * p1 * p2;
        } else {
            blendedPixel = 1.0f - 2.0f * (1.0f - p1) * (1.0f - p2);
        }

        // Convert back to the range [0, 255]
        uint8_t blendedPixelByte = static_cast<uint8_t>(blendedPixel * 255.0f + 0.5f);

        blendedImage[i] = blendedPixelByte;
    }

    return blendedImage;
}

//Addition Implementation

vector<uint8_t> ImageManipulator::Addition(const vector<uint8_t>& image, uint8_t value) {
    vector<uint8_t> result;
    result.reserve(image.size());

    for (uint8_t pixel : image) {
        int sum = pixel + value;
        uint8_t clampedSum = (sum < pixel) ? 255 : (sum > 255) ? 255 : sum;
        result.push_back(clampedSum);
    }

    return result;
}

//Checker

vector<uint8_t> ImageManipulator::CompareImages(const vector<uint8_t>& image1, const vector<uint8_t>& image2) {
    vector<uint8_t> differences;

    if (image1.size() != image2.size()) {
        cout << "Images have different sizes." << endl;
        return differences;
    }

    differences.reserve(image1.size());
    for (size_t i = 0; i < image1.size(); ++i) {
        if (image1[i] != image2[i]) {
            differences.push_back(255);  // Mark the pixel as different (white color)
        } else {
            differences.push_back(0);    // Mark the pixel as identical (black color)
        }
    }

    return differences;
}

