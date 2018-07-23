#include <vector>
#include <algorithm> // min, max
#include <iostream>
#include "LightMask.hpp"

LightMask::LightMask(int width, int height)
{
    this->width_ = width;
    this->height_ = height;
    mask = std::vector<float>(width * height, 0.0f);
    intensity_ = 50.0;
    falloff_ = 1/intensity_;
    max_blur_rad_ = 2;
    ambient_ = 0.0;

}

void LightMask::setIntensity(float i)
{
    // Must be at least 1
    i = std::max(1.0f, i);
    intensity_ = i;
    falloff_ = 1/i;
}

void LightMask::setAmbient(float a)
{
    ambient_ = std::max(0.0f, std::min(1.0f, a));
}

void LightMask::reset()
{
    // Reset the light mask
    std::fill(mask.begin(), mask.end(), ambient_);
}

void LightMask::addLight(int x, int y, float br)
{
    mask[idx(x,y)] = std::max(mask[idx(x,y)], br);
}

void LightMask::computeMask(const std::vector<float>& walls)
{
    // Add walls
    for (int i = 0; i < mask.size(); i++)
    {
        mask[i] = std::max(0.0f, mask[i] - walls[i]);
    }

    // 2 Iterations of forward and backward propagation
    forwardProp(walls);
    backwardProp(walls);
    forwardProp(walls);
    backwardProp(walls);

    // Add a small amount of light to all lit walls
    for (int i = 0; i < walls.size(); i++)
    {
        if (walls[i] > 0.0 && mask[i] > 0.0) mask[i] = std::min(1.0, mask[i] + 0.1);
    }

    // Max blur
    // To light walls and solid objects
    // To smooth out dark borders between lights
    // Prevents tiles near walls from getting dimmer
    static std::vector<float> blurMask1(mask.size(), 0.0f);
    blur(mask, blurMask1, max_blur_rad_);
    for (int i = 0; i < mask.size(); i++) mask[i] = std::max(mask[i], blurMask1[i]);

    // Standard blur
    // To smooth out lighting
    static std::vector<float> blurMask2(mask.size(), 0.0f);
    blur(mask, blurMask2, 1);
    for (int i = 0; i < mask.size(); i++)
    {
        mask[i] = blurMask2[i]; // Apply blur
    }

    // All open space should be at least ambient
    for (int i = 0; i < mask.size(); i++)
    {
        if (walls[i] == 0.0f)
        {
            mask[i] =  std::max(ambient_, mask[i]);
        }
    }
}

float LightMask::computeIntensity(float here, float neighbor1, float neighbor2, float wall)
{
    float local_falloff = std::min(1.0f, falloff_ + (wall/10.0f));

    neighbor1 = std::max(here, neighbor1);
    neighbor2 = std::max(here, neighbor2);
    return std::max(0.0f, std::max(neighbor1, neighbor2) - local_falloff);
}

void LightMask::forwardProp(const std::vector<float>& walls)
{
    for (int x = 1; x < width_; x++)
    {
        // Only compare to pixel on the left
        mask[idx(x,0)] = computeIntensity(mask[idx(x,0)], mask[idx(x-1, 0)], 0.0f, walls[idx(x,0)]);
    }
    for (int y = 1; y < height_; y++)
    {
        // First pixel
        // Only compare to pixel above
        mask[idx(0,y)] = computeIntensity(mask[idx(0,y)], mask[idx(0, y-1)], 0.0f, walls[idx(0,y)] );

        // All other pixels: compare to pixel above and to the left
        for (int x = 1; x < width_; x++)
        {
            mask[idx(x,y)] = computeIntensity(
                    mask[ idx(x,   y)],
                    mask[ idx(x-1, y)],
                    mask[ idx(x,   y-1)],
                    walls[idx(x,   y)] );
        }
    }
}


void LightMask::backwardProp(const std::vector<float>& walls)
{
    // Backward prop
    // First (bottom) row
    for (int x = width_-1; x >=0; x--)
    {
        int y = height_-1;
        // Only compare to pixel on the left
        mask[idx(x,y)] = computeIntensity(mask[idx(x,y)], mask[idx(x+1, y)], 0.0f, walls[idx(x,y)]);
    }
    for (int y = height_-2; y >= 0; y--)
    {
        int fx = width_ - 1; // first x
        // Last pixel
        // Only compare to pixel below
        mask[idx(fx,y)] = computeIntensity(mask[idx(fx,y)], mask[idx(fx, y+1)], 0.0f, walls[idx(fx,y)] );

        // All other pixels: compare to below and to the right
        for (int x = width_-2; x >= 0; x--)
        {
            mask[idx(x,y)] = computeIntensity(
                    mask[ idx(x,   y)],
                    mask[ idx(x+1, y)],
                    mask[ idx(x,   y+1)],
                    walls[idx(x,   y)] );
        }
    }
}


void LightMask::blur(const std::vector<float>& mat, std::vector<float>& to, int rad)
{
    // number of tiles in the kernel
    int numtiles = ((2 * rad) + 1) * ((2 * rad) + 1);

    //std::vector<float> blur(width*height, 0.0f);
    for (int i = rad; i < width_-rad; i++)
    {
        for (int j = rad; j < height_-rad; j++)
        {
            // Compute the sum of all values in the kernel
            float sum = 0.0f;
            for (int kx = i-rad; kx <= i+rad; kx++)
            {
                for (int ky = j-rad; ky <= j+rad; ky++)
                {
                    sum += mat[kx + ky * width_];
                }
            }

            // Average the value
            float avg = sum / numtiles;
            to[i + j * width_] = avg;
        }
    }
}



