#pragma once

template<typename T>
class Singleton
{
public:
    Singleton() = default;
    virtual ~Singleton() = default;

    static T* Get()
    {
        static T* instance = new T();
        return instance;
    }

    Singleton &operator=(const Singleton&) = delete;
    Singleton(const Singleton&) = delete;
};