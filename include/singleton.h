#ifndef SINGLETON_H
#define SINGLETON_H

template <typename T>
class Singleton
{
public:
    Singleton() = default;
    virtual ~Singleton() = default;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(const Singleton&) = delete;

    static T* instance()
    {
        static T* instance = new T;
        return instance;
    }
};

#endif // SINGLETON_H
