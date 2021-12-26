#pragma once

#include <algorithm> // max, min
#include <vector>

//
// LightMask.hpp
// A tiny header-only flood-fill lighting engine
// @_npaul
//

class LightMask {

    //////////////////
    // Data Members //
    //////////////////

public:

    // The mask: All values range from 0 to 1
    std::vector<float> mask;

private:

    int width_;          // width of the height mask
    int height_;         // height of the light mask
    float intensity_;    // How far light spreads
    float falloff_;      // 1 / intensity
    int max_blur_rad_;   // blur radius of initial max blur
    float ambient_;      // 0.0-1.0, Ambient light level, all open tiles will be at least this bright

public:

    ////////////////////////////////////
    // Construction and configuration //
    ////////////////////////////////////

    LightMask(int width, int height)
        : mask(std::vector<float>(width * height, 0.0f)),
          width_(width),
          height_(height),
          intensity_(50.0f),
          falloff_(1.0f / intensity_),
          max_blur_rad_(2),
          ambient_(0.0f)
    { }

    // Reset the mask for redrawing
    void reset()
    {
        std::fill(mask.begin(), mask.end(), ambient_);
    }

    // Add a light to the mask
    void addLight(int x, int y, float br)
    {
        mask[idx(x,y)] = std::max(mask[idx(x,y)], br);
    }

    // Set global intensity of the light sources
    // Intensity is a measure of how far light spreads
    void setIntensity(float i)
    {
        // Must be at least 1
        i = std::max(1.0f, i);
        intensity_ = i;
        falloff_ = 1/i;
    }

    // Set ambient light level
    // All open tiles will be at least this bright
    void setAmbient(float ambient)
    {
        // Clip between 0.0 and 1.0
        ambient_ = std::max(0.0f, std::min(1.0f, ambient));
    }

private:

    ///////////////////////////////////////
    // Mask Computation Helper Functions //
    ///////////////////////////////////////

    // Helper function for accessing 1d arrays using 2d coordinates
    inline int idx(int x, int y) {return x + y * width_;}

    // Compute light intensity of a given tile given its neighbors
    float computeIntensity(float here, float neighbor1, float neighbor2, float wall)
    {
        float local_falloff = std::min(1.0f, falloff_ + (wall/10.0f));

        neighbor1 = std::max(here, neighbor1);
        neighbor2 = std::max(here, neighbor2);
        
        return std::max(0.0f, std::max(neighbor1, neighbor2) - local_falloff);
    }

    // Propagate down and to the right
    void forwardProp(const std::vector<float>& walls)
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
    // Propagate up and to the left
    void backwardProp(const std::vector<float>& walls)
    {
        // Backward prop
        // First (bottom) row
        for (int x = width_-2; x >=0; x--)
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

    // Apply a simple average blur of `from` onto `to`
    void blur(const std::vector<float>& from, std::vector<float>& to, int rad)
    {
        // number of tiles in the kernel
        const int numtiles = ((2 * rad) + 1) * ((2 * rad) + 1);

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
                        sum += from[kx + ky * width_];
                    }
                }

                // Average the value
                float avg = sum / numtiles;
                to[i + j * width_] = avg;
            }
        }
    }


public:

    //
    // Compute the mask
    // Compute light intensity of a given tile given its neighbors
    // Apply smoothing functions to make the light less uniform
    //
    void computeMask(const std::vector<float>& walls)
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

};

