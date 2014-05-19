#ifndef __WX_JNI
#define __WX_JNI

#include <jni.h>
#include <stack>

#include <stdio.h>
#include <string.h>

#include <wx/app.h>

#include <android/log.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__arm__)
	#if defined(__ARM_ARCH_7A__)
		#if defined(__ARM_NEON__)
			#define ABI "armeabi-v7a/NEON"
		#else
		  	  #define ABI "armeabi-v7a"
		#endif
	#else
	   	#define ABI "armeabi"
	#endif
#elif defined(__i386__)
	#define ABI "x86"
#elif defined(__mips__)
	#define ABI "mips"
#else
	#define ABI "unknown"
#endif

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

// temploraly for testing purposes
#define IMPLEMENT_ANDROID_APP(appname) \
		jint JNI_OnLoad(JavaVM* vm, void* reserved) { 		\
		setApp(new appname); \
		return load(vm, reserved); }


namespace wxAppMgr
{
	extern JNIEnv* Env;
	extern jobject Activity;

	extern wxApp* Application;
	extern std::stack<jobject*> ActivityStack;
};

jint load(JavaVM* vm, void* reserved)
{
	JNIEnv* env;
	if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
		return -1;
	}
	return JNI_VERSION_1_6;
}

void setApp(wxApp* application);


jint
Java_org_wxwidgets_MainActivity_wxStart( JNIEnv* env,
													  jobject thiz);


void
Java_org_wxwidgets_MainActivity_handleEvent( JNIEnv* env,
													  jobject thiz,
													  jint code,
													  jobject obj);

void
Java_org_wxwidgets_MainActivity_onCreateOMenu( JNIEnv* env,jobject thiz,
													  jobject obj);

#ifdef __cplusplus
}
#endif
#endif // __WX_JNI
