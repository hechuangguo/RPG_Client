/**
 * @file    Random.cpp
 * @brief   客户端随机数工具实现
 */

#include "Random.h"

#include <algorithm>

std::mt19937& Random::engine()
{
    static std::mt19937 eng{std::random_device{}()};
    return eng;
}

void Random::seed(unsigned int seed)
{
    engine().seed(seed);
}

int Random::randInt(int minV, int maxV)
{
    if (minV > maxV)
    {
        std::swap(minV, maxV);
    }
    std::uniform_int_distribution<int> dist(minV, maxV);
    return dist(engine());
}

float Random::randFloat(float minV, float maxV)
{
    if (minV > maxV)
    {
        std::swap(minV, maxV);
    }
    std::uniform_real_distribution<float> dist(minV, maxV);
    return dist(engine());
}
