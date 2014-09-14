#include <jni.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#include <GLES2/gl2.h>

#include <android/log.h>
#include <android_native_app_glue.h>

#include <assert.h>

#define LOG_TAG ("Android NativeActivity Test")
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN,  LOG_TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

struct ApplicationData
{
	android_app* app;
	EGLDisplay display;
	EGLSurface surface;
};

ApplicationData g_Application;

// シェーダー読み込み
GLuint loadShader(GLenum shaderType, const char* pSource, GLint* sourceLength = nullptr)
{
	// シェーダーコンパイル
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &pSource, sourceLength);
	glCompileShader(shader);

	// コンパイル結果をチェック
	GLint compileResult;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compileResult);

	if(compileResult == GL_FALSE)
	{
		// コンパイルが失敗したら、情報を吐き出す
		GLint infoLength;
		glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &infoLength);
		if(infoLength > 0)
		{
			GLchar* info = new GLchar[infoLength];
			glGetShaderInfoLog(shader, infoLength, nullptr, info);
			LOGE("Shader Compile Error:\n%s" ,info);
			delete info;
		}
		else
		{
			LOGE("Shader Compile Error No Info");
		}
	}

	return shader;
}

// シェーダー読み込み(ファイルから)
GLuint loadShaderFromFile(GLenum shaderType, const char* pSource)
{
	GLuint result = 0;

	// assetsフォルダから指定されたファイルを読み込み
	AAsset* asset = AAssetManager_open(g_Application.app->activity->assetManager, pSource,
			AASSET_MODE_BUFFER);

	if(asset)
	{
		// 実際にファイルが存在した時、読み込む
		GLint filesize = AAsset_getLength(asset);
		char* buffer = new char[filesize];

		AAsset_read(asset, buffer, filesize);
		AAsset_close(asset);

		// コンパイル
		result = loadShader(shaderType, buffer, &filesize);
		delete buffer;
	}
	else
	{
		LOGE("Failed Shader File:%s\n", pSource);
		assert(false);
	}

	return result;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource)
{
	GLuint vertexShader = loadShaderFromFile(GL_VERTEX_SHADER, pVertexSource);
	GLuint pixelShader = loadShaderFromFile(GL_FRAGMENT_SHADER,
			pFragmentSource);

	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, pixelShader);
	glLinkProgram(program);
	GLint linkStatus = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
	return program;
}

int init(ApplicationData* e)
{
	const EGLint attribs[] =
	{
	EGL_BLUE_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_RED_SIZE, 8,
	EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	EGL_NONE };
	const EGLint contextAttribs[] =
	{
	EGL_CONTEXT_CLIENT_VERSION, 2,
	EGL_NONE };

	EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(display, 0, 0);

	EGLConfig config;
	EGLint numConfigs;
	eglChooseConfig(display, attribs, &config, 1, &numConfigs);

	EGLint format;
	eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
	ANativeWindow_setBuffersGeometry(e->app->window, 0, 0, format);

	EGLSurface surface = eglCreateWindowSurface(display, config, e->app->window,
			nullptr);
	EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT,
			contextAttribs);

	if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
	{
		return -1;
	}

	EGLint w, h;
	eglQuerySurface(display, surface, EGL_WIDTH, &w);
	eglQuerySurface(display, surface, EGL_HEIGHT, &h);

	e->display = display;
	e->surface = surface;

	glViewport(0, 0, w, h);

	return 0;

}

// 描画
void draw(ApplicationData* e)
{
	GLuint gProgram = createProgram("shader/vertex/basic.vert",
			"shader/fragment/basic.frag");

	GLuint gvPositionHandle = glGetAttribLocation(gProgram, "vPosition");

	// 今後拡張するためにTriangleリストで描画
	const GLfloat vertices[] =
	{ -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f,
	   0.5f, -0.5f, -0.5f, -0.5f, -0.5f, 0.5f};

	glClearColor(0.f, 0.f, 1.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(gProgram);
	glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 0, vertices);
	glEnableVertexAttribArray(gvPositionHandle);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	eglSwapBuffers(e->display, e->surface);
}

// エントリーポイント
void android_main(android_app* state)
{
	app_dummy();

	g_Application.app = state;
	state->userData = &g_Application;
	state->onAppCmd = [](android_app* app, int32_t cmd)
	{
		auto e = static_cast<ApplicationData*>(app->userData);
		switch (cmd)
		{
			case APP_CMD_INIT_WINDOW:
			init(e);
			draw(e);
			break;
		}
	};

	int ident, events;
	android_poll_source* source;

	while (true)
	{
		// イベントをポーリング
		ident = ALooper_pollAll(0, nullptr, &events, reinterpret_cast<void**>(&source));
		while (ident >= 0)
		{
			if (source != nullptr)
			{
				source->process(state, source);
			}
			if (state->destroyRequested)
			{
				return;
			}
		}
	}
}

