/**
 * @file    Random.h
 * @brief   客户端随机数工具
 *
 * 基于 std::mt19937 提供整数与浮点随机，用于氛围 NPC、飞鸟等客户端表现。
 *
 * 线程：仅主线程调用，非线程安全。
 */

#pragma once

#include <random>

/**
 * @brief 随机数工具类（静态方法，内部 std::mt19937 引擎）
 */
class Random
{
public:
    /**
     * @brief 设置随机引擎种子
     * @param seed 种子值（相同种子产生可复现序列）
     */
    static void seed(unsigned int seed);

    /**
     * @brief 闭区间整数随机
     * @param minV 下界（含）
     * @param maxV 上界（含）
     * @return [minV, maxV] 内的随机整数
     */
    static int randInt(int minV, int maxV);

    /**
     * @brief 半开区间浮点随机
     * @param minV 下界（含），默认 0
     * @param maxV 上界（不含），默认 1
     * @return [minV, maxV) 内的随机浮点
     */
    static float randFloat(float minV = 0.0f, float maxV = 1.0f);

private:
    static std::mt19937& engine();
};
