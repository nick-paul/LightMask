#pragma once

#include <algorithm>
#include <vector>

class LightMask {
public:
    LightMask(int width, int height);

    // Reset the mask for redrawing
    void reset();

    // Add a light to the mask
    void addLight(int x, int y, float br);

    // Compute light values for the mask
    void computeMask(const std::vector<float>& walls);

    // The mask: All values range from 0 to 1
    std::vector<float> mask;

    // Set global intensity of the light sources
    void setIntensity(float i);

    // Set ambient light level
    void setAmbient(float ambient);

private:
    // Compute light intensity of a given tile given its neighbors
    float computeIntensity(float here, float neighbor1, float neighbor2, float wall);
    // Propagate down and to the right
    void forwardProp(const std::vector<float>& walls);
    // Propagate up and to the left
    void backwardProp(const std::vector<float>& walls);
    // Apply a simple average blur of `from` onto `to`
    void blur(const std::vector<float>& from, std::vector<float>& to, int rad);

    // Helper function for accessing 1d arrays using 2d coordinates
    inline int idx(int x, int y) {return x + y * width_;}

    int width_;          // width of the height mask
    int height_;         // height of the light mask
    float intensity_;    // global light intensity
    float falloff_;      // 1 / intensity
    int max_blur_rad_;   // blur radius of initial max blur
    float ambient_;
};

