#include "JNIinterface.hpp"
#include <string>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <android/log.h>
#include <jni.h>
#define PRINTF(...) __android_log_print(ANDROID_LOG_INFO, "SlzGfx", __VA_ARGS__)
#define PRINTF_ERROR(...) __android_log_print(ANDROID_LOG_ERROR, "SlzGfx", __VA_ARGS__)
#define RETURN_ON_NULL(var) if (!var) { PRINTF_ERROR(("JNI Hook: Failed to get " #var "\n")); return JNI_VERSION_1_6;}

namespace fs = std::filesystem;

jniInterface::jniInterface()
{
    m_JavaVM = NULL;
    m_JNIEnv = NULL;
    m_Context = NULL;

    externalCacheDir = "";
    externalFilesDir = "";
}

std::unique_ptr<jniInterface> s_jni;

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) 
{
    PRINTF("JNI_OnLoad Invoked!");
    s_jni = std::make_unique<jniInterface>();
    s_jni->m_JavaVM = vm;

    JNIEnv* env; 
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK)
    {
        return JNI_ERR;
    }
    s_jni->m_JNIEnv = env;
    
    // Get the current application context
    jclass activityThread = env->FindClass("android/app/ActivityThread");
    RETURN_ON_NULL(activityThread);
    jmethodID currentActivityThread = env->GetStaticMethodID(activityThread, "currentActivityThread", "()Landroid/app/ActivityThread;");
    RETURN_ON_NULL(currentActivityThread);
    jobject activityThreadObj = env->CallStaticObjectMethod(activityThread, currentActivityThread);
    RETURN_ON_NULL(activityThreadObj);

    jmethodID getApplication = env->GetMethodID(activityThread, "getApplication", "()Landroid/app/Application;");
    RETURN_ON_NULL(getApplication);
    jobject context = env->CallObjectMethod(activityThreadObj, getApplication);
    s_jni->m_Context = context;

    jclass contextClass = env->FindClass("android/content/Context");
    RETURN_ON_NULL(contextClass);

    // Get the java file class the method for extracting its path string
    jclass fileClass = env->FindClass("java/io/File");
    RETURN_ON_NULL(fileClass);
    jmethodID getPath = env->GetMethodID(fileClass, "getPath", "()Ljava/lang/String;");
    RETURN_ON_NULL(getPath);

    // Get the external cache directory
    jmethodID getExternalCacheDir = env->GetMethodID(contextClass, "getExternalCacheDir", "()Ljava/io/File;");
    RETURN_ON_NULL(getExternalCacheDir);
    jobject cacheDir = env->CallObjectMethod(context, getExternalCacheDir);
    RETURN_ON_NULL(cacheDir);
    jstring cachePathJString = (jstring)env->CallObjectMethod(cacheDir, getPath);
    RETURN_ON_NULL(cachePathJString);

    const char* cachePathCString = env->GetStringUTFChars(cachePathJString, NULL);
    s_jni->externalCacheDir = cachePathCString;
    __android_log_print(ANDROID_LOG_INFO, "SlzGfx", "Sucessfully got cache folder: %s\n", s_jni->externalCacheDir.c_str());
    env->ReleaseStringUTFChars(cachePathJString, cachePathCString);
    
    // Get the external persistent files directory
    jmethodID getExternalFilesDir = env->GetMethodID(contextClass, "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;");
    RETURN_ON_NULL(getExternalFilesDir);
    jobject filesDir = env->CallObjectMethod(context, getExternalFilesDir, NULL);
    RETURN_ON_NULL(filesDir);
    jstring filesPathJString = (jstring)env->CallObjectMethod(filesDir, getPath);
    RETURN_ON_NULL(filesPathJString);

    const char* filesPathCString = env->GetStringUTFChars(filesPathJString, NULL);
    s_jni->externalFilesDir = fs::path((const char8_t*)filesPathCString);
    __android_log_print(ANDROID_LOG_INFO, "SlzGfx", "Sucessfully got persistent files folder: %s\n", s_jni->externalFilesDir.c_str());
    env->ReleaseStringUTFChars(filesPathJString, filesPathCString);
    


    //----------------------

    jclass unityClass = env->FindClass("com/unity3d/player/UnityPlayer");
    RETURN_ON_NULL(unityClass);

    jfieldID getActivity = env->GetStaticFieldID(unityClass, "currentActivity", "Landroid/app/Activity;");
    RETURN_ON_NULL(getActivity);

    jclass activityClass = env->FindClass("android/app/Activity");
    RETURN_ON_NULL(activityClass)

    jobject activityObj = env->GetStaticObjectField(unityClass, getActivity);
    RETURN_ON_NULL(activityObj);

    jclass intentClass = env->FindClass("android/content/Intent");
    RETURN_ON_NULL(intentClass);

    jmethodID getIntent = env->GetMethodID(activityClass, "getIntent", "()Landroid/content/Intent;");
    RETURN_ON_NULL(getIntent);

    jobject intentObj = env->CallObjectMethod(activityObj, getIntent);
    RETURN_ON_NULL(intentObj);

    jmethodID getIntExtra = env->GetMethodID(intentClass, "getIntExtra", "(Ljava/lang/String;I)I");
    RETURN_ON_NULL(getIntExtra);

    /*
    jclass bundleClass = env->FindClass("android/os/BaseBundle");
    RETURN_ON_NULL(bundleClass);
    jmethodID containsKey = env->GetMethodID(bundleClass, "containsKey", "(Ljava/lang/String;)Ljava/lang/boolean;");
    RETURN_ON_NULL(getExtras);
    */

    const char* argCreateDirs = "createExternalFilesDir";
    jstring argCreateDirsJS = env->NewStringUTF(argCreateDirs);
    

    jint keyVal = env->CallIntMethod(intentObj, getIntExtra, argCreateDirsJS, jint(0));

    if (keyVal)
    {
        PRINTF("createExternalFilesDir intent found");
    }
    else
    {
        PRINTF("createExternalFilesDir intent NOT found");
    }

    //Don't release, argCreateDirs is a constant
    //env->ReleaseStringUTFChars(argCreateDirsJS, argCreateDirs);

    return JNI_VERSION_1_6;
}

int CopyGraphicsIniFromTemp()
{
    const char* tempSettingsPath = "/data/local/tmp/slz_graphics.ini";
    std::string outSettingsPath = s_jni->externalFilesDir / fs::path(u8"/graphics.ini");
    if ((access(tempSettingsPath, F_OK) == 0) && (access(outSettingsPath.c_str(), F_OK) != 0))
    {

        int tempHandle = open(tempSettingsPath, O_RDONLY);
        if (tempHandle == -1)
        {
            PRINTF_ERROR("Failed to open temp settings file. errno is: %s", strerror(errno));
            goto error;
        }
        struct stat tempStats = {};
        int returnCode = fstat(tempHandle, &tempStats);
        if (returnCode != 0)
        {
            PRINTF_ERROR("Failed to get temp settings file size. errno is: %s", strerror(errno));
            close(tempHandle);
            goto error;
        }

        size_t fileSize = tempStats.st_size;
        int outHandle = creat(outSettingsPath.c_str(), O_WRONLY | O_CREAT);
        if (outHandle == -1)
        {
            PRINTF_ERROR("Failed to create settings file. errno is: %s", strerror(errno));
            close(tempHandle);
            goto error;
        }

        sendfile(tempHandle, outHandle, 0, fileSize);
        close(tempHandle);
        close(outHandle);
        remove(tempSettingsPath);
        PRINTF("Copied slz_graphics.ini from /data/local/tmp to graphics.ini in this app's persistent files folder");
        return 0;
    }

error:
    return 1;
}
