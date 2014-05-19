#include <wx/android/wxjni.h>

namespace wxAppMgr
{
	JNIEnv* Env;
	jobject Activity;

	wxApp* Application;
	std::stack<jobject*> ActivityStack;
};

void setApp(wxApp* application) {
	wxAppMgr::Application = application;
}

jint
Java_org_wxwidgets_MainActivity_wxStart( JNIEnv* env,
													  jobject thiz)
{
	wxAppMgr::Env = env;
	wxAppMgr::Activity = env->NewGlobalRef(thiz);

	if(wxAppMgr::ActivityStack.size() == 0) {
		if(wxAppMgr::Application) {
			wxAppMgr::Application->OnInit();
		}
	}
	else {

	}

	wxAppMgr::ActivityStack.push(&thiz);
	return wxAppMgr::ActivityStack.size();
}

void
Java_org_wxwidgets_MainActivity_handleEvent( JNIEnv* env,jobject thiz,
													  jint code,jobject obj)
{
}

void
Java_org_wxwidgets_MainActivity_onCreateOMenu( JNIEnv* env,jobject thiz,
													  jobject obj)
{
}
