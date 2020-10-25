#pragma once

/**
 * Counter which automatically clamps the value between min and max.
 */
template <typename T, T min, T max>
class Counter
{
public:
    Counter(T val) : value(val >= min ? (val <= max ? val : max) : min) {}
    T operator=(T val)
    {
        value = val;
        return value;
    }
    operator T() const
    {
        return value;
    }

    void inc()
    {
        if (value < max)
        {
            value++;
        }
    }
    void dec()
    {
        if (value > min)
        {
            value--;
        }
    }

private:
    T value;
};

/**
 * Counter which keeps the value between given min and max by cyclically
 * reducing the number.
 */
template <typename T, T min, T max>
class CyclicCounter
{
public:
    // TODO: Should initial value also be computed cyclically?
    CyclicCounter(T val) : value(val >= min ? (val <= max ? val : max) : min) {}
    T operator=(T val)
    {
        value = val;
        return value;
    }
    operator T() const
    {
        return value;
    }

    void inc()
    {
        if (value < max)
        {
            value++;
        }
        else
        {
            value = min;
        }
    }
    void dec()
    {
        if (value > min)
        {
            value--;
        }
        else
        {
            value = max;
        }
    }

private:
    T value;
};
