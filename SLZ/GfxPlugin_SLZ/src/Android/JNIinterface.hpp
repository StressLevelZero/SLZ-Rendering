#ifndef JNI_INTERFACE
#define JNI_INTERFACE

#include <string>
#include <filesystem>

class jniInterface
{
public:
    void* m_JavaVM;
    void* m_JNIEnv;
    void* m_Context;
    std::filesystem::path externalCacheDir;
    std::filesystem::path externalFilesDir;
public:
    jniInterface();
};

extern std::unique_ptr<jniInterface> s_jni;

#endif
