#include <winsock2.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <ppapi/cpp/module.h>
#include <ppapi/cpp/instance.h>
#include <ppapi/cpp/var.h>
#include <ppapi/cpp/var_dictionary.h>
#include <ppapi/cpp/input_event.h>
#include <ppapi/cpp/graphics_3d.h>
#include <ppapi/lib/gl/gles2/gl2ext_ppapi.h>
#include <ppapi/utility/completion_callback_factory.h>
#include "mpv/libmpv/client.h"
#include "mpv/libmpv/render_gl.h"

#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>

using namespace std::chrono_literals;

// Fix for MSVS.
#ifdef PostMessage
#undef PostMessage
#endif

#define QUOTE(arg) #arg
#define DIE(msg) { fprintf(stderr, "%s\n", msg); return false; }
#define GLCB(name) { QUOTE(gl##name), reinterpret_cast<void*>(gl##name) }

using pp::Var;

#define IDLE_EVENT  "mpv-idle"

static void dummyReadBuffer(GLenum) {}

// PPAPI GLES implementation doesn't provide getProcAddress.
static const std::unordered_map<std::string, void*> GL_CALLBACKS = {
  GLCB(GetString),
  GLCB(ActiveTexture),
  GLCB(AttachShader),
  GLCB(BindAttribLocation),
  GLCB(BindBuffer),
  GLCB(BindTexture),
  GLCB(BlendFuncSeparate),
  GLCB(BufferData),
  GLCB(BufferSubData),
  GLCB(Clear),
  GLCB(ClearColor),
  GLCB(CompileShader),
  GLCB(CreateProgram),
  GLCB(CreateShader),
  GLCB(DeleteBuffers),
  GLCB(DeleteProgram),
  GLCB(DeleteShader),
  GLCB(DeleteTextures),
  GLCB(Disable),
  GLCB(DisableVertexAttribArray),
  GLCB(DrawArrays),
  GLCB(Enable),
  GLCB(EnableVertexAttribArray),
  GLCB(Finish),
  GLCB(Flush),
  GLCB(GenBuffers),
  GLCB(GenTextures),
  GLCB(GetAttribLocation),
  GLCB(GetError),
  GLCB(GetIntegerv),
  GLCB(GetProgramInfoLog),
  GLCB(GetProgramiv),
  GLCB(GetShaderInfoLog),
  GLCB(GetShaderiv),
  GLCB(GetString),
  GLCB(GetUniformLocation),
  GLCB(LinkProgram),
  GLCB(PixelStorei),
  GLCB(ReadPixels),
  GLCB(Scissor),
  GLCB(ShaderSource),
  GLCB(TexImage2D),
  GLCB(TexParameteri),
  GLCB(TexSubImage2D),
  GLCB(Uniform1f),
  GLCB(Uniform2f),
  GLCB(Uniform3f),
  GLCB(Uniform1i),
  GLCB(UniformMatrix2fv),
  GLCB(UniformMatrix3fv),
  GLCB(UseProgram),
  GLCB(VertexAttribPointer),
  GLCB(Viewport),
  GLCB(BindFramebuffer),
  GLCB(GenFramebuffers),
  GLCB(DeleteFramebuffers),
  GLCB(CheckFramebufferStatus),
  GLCB(FramebufferTexture2D),
  GLCB(GetFramebufferAttachmentParameteriv),
  GLCB(GenQueriesEXT),
  GLCB(DeleteQueriesEXT),
  GLCB(BeginQueryEXT),
  GLCB(EndQueryEXT),
  // to make screenshot not crash (nullpoint)
  {"glReadBuffer", dummyReadBuffer},
  // Few functions are not available in PPAPI or doesn't work properly.
  {"glQueryCounterEXT", NULL},
  GLCB(IsQueryEXT),
  {"glGetQueryObjectivEXT", NULL},
  {"glGetQueryObjecti64vEXT", NULL},
  GLCB(GetQueryObjectuivEXT),
  {"glGetQueryObjectui64vEXT", NULL},
  {"glGetTranslatedShaderSourceANGLE", NULL}
};

static Var node_to_var(const mpv_node* node) {
  if (node->format == MPV_FORMAT_NONE) {
    return Var::Null();
  } else if (node->format == MPV_FORMAT_STRING) {
    return Var(node->u.string);
  } else if (node->format == MPV_FORMAT_FLAG) {
    return Var(static_cast<bool>(node->u.flag));
  } else if (node->format == MPV_FORMAT_INT64) {
    return Var(static_cast<int32_t>(node->u.int64));
  } else if (node->format == MPV_FORMAT_DOUBLE) {
    return Var(node->u.double_);
  } else if (node->format == MPV_FORMAT_NODE_ARRAY) {
    pp::VarArray objects;
    for (uint32_t idx = 0; idx < node->u.list->num; idx++) {
      objects.Set(idx, node_to_var(&node->u.list->values[idx]));
    }
    return objects;
  } else if (node->format == MPV_FORMAT_NODE_MAP) {
    pp::VarDictionary objects;
    for (int idx = 0; idx < node->u.list->num; idx++) {
      objects.Set(node->u.list->keys[idx], node_to_var(&node->u.list->values[idx]));
    }
    return objects;
  }

  return Var::Null();
}

static std::string var_to_string(const Var& value) {
  if (value.is_string()) {
    return value.AsString();
  } else if (value.is_bool()) {
    int value_bool = value.AsBool();
    return value_bool ? "yes" : "no";
  } else if (value.is_int()) {
    int64_t value_int = value.AsInt();
    return std::to_string(value_int);
  } else if (value.is_double()) {
    double value_double = value.AsDouble();
    return std::to_string(value_double);
  }
  return "";
}

// clone from mpv_event_to_node
static Var mpv_event_to_js(const mpv_event* event, const char* evname) {
  pp::VarDictionary dst;
  dst.Set("event", Var(evname));

  if (!event) {
    return dst;
  }

  if (event->error < 0) {
    dst.Set("error", Var(mpv_error_string(event->error)));
  }

  if (event->reply_userdata)
    dst.Set("id", Var(static_cast<int>(event->reply_userdata)));

  switch (event->event_id) {
    case MPV_EVENT_START_FILE: {
      mpv_event_start_file *esf = static_cast<mpv_event_start_file*>(event->data);
      dst.Set("playlist_entry_id", Var(static_cast<int>(esf->playlist_entry_id)));
      break;
    }

    case MPV_EVENT_END_FILE: {
      mpv_event_end_file *eef = static_cast<mpv_event_end_file*>(event->data);

      const char *reason;
      switch (eef->reason) {
        case MPV_END_FILE_REASON_EOF: reason = "eof"; break;
        case MPV_END_FILE_REASON_STOP: reason = "stop"; break;
        case MPV_END_FILE_REASON_QUIT: reason = "quit"; break;
        case MPV_END_FILE_REASON_ERROR: reason = "error"; break;
        case MPV_END_FILE_REASON_REDIRECT: reason = "redirect"; break;
        default:
          reason = "unknown";
      }
      dst.Set("reason", Var(reason));
      dst.Set("playlist_entry_id", Var(static_cast<int>(eef->playlist_entry_id)));

      if (eef->playlist_insert_id) {
          dst.Set("playlist_insert_id", Var(static_cast<int>(eef->playlist_insert_id)));
          dst.Set("playlist_insert_num_entries", Var(static_cast<int>(eef->playlist_insert_num_entries)));
      }

      if (eef->reason == MPV_END_FILE_REASON_ERROR) {
        dst.Set("file_error", Var(mpv_error_string(eef->error)));
      }

      break;
    }

    case MPV_EVENT_LOG_MESSAGE: {
      mpv_event_log_message *msg = static_cast<mpv_event_log_message*>(event->data);
      dst.Set("prefix", Var(msg->prefix));
      dst.Set("level", Var(msg->level));
      dst.Set("text", Var(msg->text));
      break;
    }

    case MPV_EVENT_CLIENT_MESSAGE: {
      mpv_event_client_message *msg = static_cast<mpv_event_client_message*>(event->data);
      pp::VarArray args;

      for (uint32_t n = 0; n < msg->num_args; n++) {
        args.Set(n, Var((char *)msg->args[n]));
      }
      break;
    }

    case MPV_EVENT_GET_PROPERTY_REPLY:
    case MPV_EVENT_PROPERTY_CHANGE: {
      mpv_event_property *prop = static_cast<mpv_event_property*>(event->data);

      dst.Set("name", Var(prop->name));
      switch (prop->format) {
        case MPV_FORMAT_NODE:
          dst.Set("data", node_to_var(static_cast<mpv_node*>(prop->data)));
          break;
        case MPV_FORMAT_DOUBLE:
          dst.Set("data", Var(*(double *)prop->data));
          break;
        case MPV_FORMAT_FLAG:
          dst.Set("data", Var(*(int *)prop->data));
          break;
        case MPV_FORMAT_STRING:
          dst.Set("data", Var(*(char **)prop->data));
          break;
        default:;
      }
      break;
    }

    case MPV_EVENT_COMMAND_REPLY: {
      mpv_event_command *cmd = static_cast<mpv_event_command*>(event->data);
      dst.Set("result", Var(node_to_var(&cmd->result)));
      break;
    }

    case MPV_EVENT_HOOK: {
      mpv_event_hook *hook = static_cast<mpv_event_hook*>(event->data);
      dst.Set("hook_id", Var(static_cast<int>(hook->id)));
      break;
    }
  }

  return dst;
}

extern "C" {
  int mpv_command_json_async(mpv_handle *ctx, uint64_t ud, const char *jsonstr);
}

class MPVInstance : public pp::Instance {
 public:
  explicit MPVInstance(PP_Instance instance)
      : pp::Instance(instance),
        callback_factory_(this),
        mpv_(NULL),
        mpv_gl_(NULL) {
    // RequestInputEvents(PP_INPUTEVENT_CLASS_MOUSE);
  }

  virtual ~MPVInstance() {
    if (mpv_gl_) {
      glSetCurrentContextPPAPI(context_.pp_resource());
      mpv_render_context_free(mpv_gl_);
    }
    mpv_terminate_destroy(mpv_);
  }

  bool Init(uint32_t argc, const char *argn[], const char *argv[]) override {
    bool result = InitGL() && InitMPV();

    pp::VarDictionary dict;
    dict.Set(pp::Var("type"), pp::Var("ready"));
    dict.Set(pp::Var("data"), Var(result));
    PostMessage(dict);
    return result;
  }

  /*
  bool HandleInputEvent(const pp::InputEvent& event) override {
    switch (event.GetType()) {
      case PP_INPUTEVENT_TYPE_MOUSEDOWN:
        printf("--------------------PP_INPUTEVENT_TYPE_MOUSEDOWN-----\n");
        return false;
      case PP_INPUTEVENT_TYPE_MOUSEMOVE:
        printf("--------------------PP_INPUTEVENT_TYPE_MOUSEMOVE-----\n");
        return false;
      default:
        return false;
    }
    return false;
  }
  */

  void HandleMessage(const Var& msg) override {
    pp::VarDictionary dict(msg);
    std::string type = dict.Get("type").AsString();
    pp::Var data = dict.Get("data");
    pp::Var var_id = dict.Get("id");
    const uint64_t id = var_id.is_number() ? (uint64_t)var_id.AsInt(): 0;

    if (type == "command") {
      if (data.is_string()) {
        std::string jsonstr = data.AsString();
        int rc = mpv_command_json_async(mpv_, id, jsonstr.c_str());
        if (rc < 0) {
          PostCommandFail(id, rc, nullptr);
        }
      } else if (data.is_array()) {
        pp::VarArray args(data);
        uint32_t len = args.GetLength();
        std::vector<std::string> args_str(len);
        std::vector<const char*> args_ptr(len + 1);
        for (uint32_t i = 0; i < len; i++) {
          args_str[i] = args.Get(i).AsString();
          args_ptr[i] = args_str[i].c_str();
        }
        args_ptr[len] = NULL;
        
        int rc = mpv_command_async(mpv_, id, args_ptr.data());
        if (rc < 0) {
          PostCommandFail(id, rc, nullptr);
        }
      } else {
        PostCommandFail(id, -1, "bad command format");
      }
    }  else if (type == "set_property") {
      pp::VarDictionary data_dict(data);
      std::string name = data_dict.Get("name").AsString();
      pp::Var value = data_dict.Get("value");
      if (value.is_string()) {
        std::string value_string = value.AsString();
        const char* value_cstr = value_string.c_str();
        mpv_set_property_async(mpv_, id, name.c_str(), MPV_FORMAT_STRING, &value_cstr);
      } else if (value.is_bool()) {
        int value_bool = value.AsBool();
        mpv_set_property_async(mpv_, id, name.c_str(), MPV_FORMAT_FLAG, &value_bool);
      } else if (value.is_int()) {
        int64_t value_int = value.AsInt();
        mpv_set_property_async(mpv_, id, name.c_str(), MPV_FORMAT_INT64, &value_int);
      } else if (value.is_double()) {
        double value_double = value.AsDouble();
        mpv_set_property_async(mpv_, id, name.c_str(), MPV_FORMAT_DOUBLE, &value_double);
      }
    } else if (type == "observe_property") {
      std::string name = data.AsString();
      mpv_observe_property(mpv_, id, name.c_str(), MPV_FORMAT_NODE);
    }  else if (type == "unobserve_property") {
      uint64_t id = data.AsInt();
      mpv_unobserve_property(mpv_, id);
    } else if (type == "get_property_async") {
      std::string name = data.AsString();
      mpv_get_property_async(mpv_, id, name.c_str(), MPV_FORMAT_NODE);
    } else if (type == "request_event") {
      pp::VarDictionary data_dict(data);
      std::string event = data_dict.Get("event").AsString();
      int enable = data_dict.Get("enable").AsBool();

      int rc = -1;
      // special event fror js plugin
      if (event == IDLE_EVENT) {
        rc = MPV_ERROR_SUCCESS;
      } else {
        const char *name;
        for (int n = 0; n < 256 && (name = mpv_event_name(static_cast<mpv_event_id>(n))); n++) {
          if (event == name) {
            rc = mpv_request_event(mpv_, static_cast<mpv_event_id>(n), enable);
            break;
          }
        }
      }

      if (rc == MPV_ERROR_SUCCESS) {
        if (enable) {
          registed_events_.insert(event);
        } else {
          registed_events_.erase(event);
        }
      }
    } else if (type == "hook_continue") {
      mpv_hook_continue(mpv_, data.AsInt());
    } else if (type == "hook_add") {
      pp::VarDictionary data_dict(data);
      std::string name = data_dict.Get("name").AsString();
      uint64_t id = data_dict.Get("id").AsInt();
      int priority = data_dict.Get("priority").AsInt();
      mpv_hook_add(mpv_, id, name.c_str(), priority);
    } else if (type == "set_option") {
      pp::VarDictionary data_dict(data);
      std::string name = data_dict.Get("name").AsString();
      std::string value = data_dict.Get("value").AsString();
      mpv_set_option_string(mpv_, name.c_str(), value.c_str());
    }
  }

 private:
  static void* GetProcAddressMPV(void* fn_ctx, const char* name) {
    auto search = GL_CALLBACKS.find(name);
    if (search == GL_CALLBACKS.end()) {
      fprintf(stderr, "FIXME: missed GL function %s\n", name);
      return NULL;
    } else {
      return search->second;
    }
  }

  void HandleMPVEvents(int32_t) {
    for (;;) {
      mpv_event* event = mpv_wait_event(mpv_, 0);
      // printf("@@@ EVENT %d\n", event->event_id);
      if (event->event_id == MPV_EVENT_NONE) break;

      const char* evname = mpv_event_name(event->event_id);
      if (evname) {
        DispatchEvent(event, evname);
      }
    }

    DispatchEvent(NULL, IDLE_EVENT);
  }

  void DispatchEvent(mpv_event* event, const char* evname) {
    if (0 == registed_events_.count(evname)) {
      return;
    }

    PostMessage(mpv_event_to_js(event, evname));
  }

  void PostCommandFail(uint64_t id, int code, const char* err) {
    pp::VarDictionary dst;
    dst.Set("event", Var("command-reply"));
    dst.Set("id", Var(static_cast<int>(id)));

    if (err) {
      dst.Set("error", Var(err));
    } else {
      dst.Set("error", Var(std::to_string(code)));
    }

    PostMessage(dst);
  }

  static void HandleMPVWakeup(void* ctx) {
    MPVInstance* b = static_cast<MPVInstance*>(ctx);
    pp::Module::Get()->core()->CallOnMainThread(
        0, b->callback_factory_.NewCallback(&MPVInstance::HandleMPVEvents));
  }

  static void HandleMPVUpdate(void* ctx) {
    // printf("@@@ UPDATE\n");
    MPVInstance* b = static_cast<MPVInstance*>(ctx);
    pp::Module::Get()->core()->CallOnMainThread(
        0, b->callback_factory_.NewCallback(&MPVInstance::OnGetFrame));
  }

  bool InitGL() {
    if (!glInitializePPAPI(pp::Module::Get()->get_browser_interface()))
      DIE("unable to initialize GL PPAPI");

    const int32_t attrib_list[] = {
      PP_GRAPHICS3DATTRIB_ALPHA_SIZE, 8,
      PP_GRAPHICS3DATTRIB_DEPTH_SIZE, 24,
      PP_GRAPHICS3DATTRIB_NONE
    };

    context_ = pp::Graphics3D(this, attrib_list);
    if (!BindGraphics(context_))
      DIE("unable to bind 3d context");

    return true;
  }

  bool InitMPV() {
    setlocale(LC_NUMERIC, "C");
    mpv_ = mpv_create();
    if (!mpv_)
      DIE("context init failed");

    char* terminal = getenv("MPVJS_TERMINAL");
    if (terminal && strlen(terminal))
      mpv_set_option_string(mpv_, "terminal", "yes");
    char* verbose = getenv("MPVJS_VERBOSE");
    if (verbose && strlen(verbose))
      mpv_set_option_string(mpv_, "msg-level", "all=v");

    // Can't be set after initialize in mpv 0.18.
    mpv_set_option_string(mpv_, "input-default-bindings", "yes");
    // mpv_set_option_string(mpv_, "pause", "yes");

    mpv_set_option_string(mpv_, "idle", "yes");

    if (mpv_initialize(mpv_) < 0)
      DIE("mpv init failed");

    glSetCurrentContextPPAPI(context_.pp_resource());

    mpv_opengl_init_params gl_init_params{GetProcAddressMPV, nullptr, nullptr};
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    if (mpv_render_context_create(&mpv_gl_, mpv_, params) < 0)
      DIE("failed to initialize mpv GL context");

    // Some convenient defaults. Can be always changed on ready event.
    mpv_set_option_string(mpv_, "stop-playback-on-init-failure", "no");
    mpv_set_option_string(mpv_, "audio-file-auto", "no");
    mpv_set_option_string(mpv_, "sub-auto", "no");
    mpv_set_option_string(mpv_, "volume-max", "100");
    mpv_set_option_string(mpv_, "keep-open", "no");
    mpv_set_option_string(mpv_, "keep-open-pause", "no");
    mpv_set_option_string(mpv_, "osd-bar", "no");
    mpv_set_option_string(mpv_, "reset-on-next-file", "pause");

    mpv_set_option_string(mpv_, "force-window", "immediate");

    LoadMPV();
    return true;
  }

  void LoadMPV() {
    mpv_set_wakeup_callback(mpv_, HandleMPVWakeup, this);
    mpv_render_context_set_update_callback(mpv_gl_, HandleMPVUpdate, this);
  }

  void PaintFinished(int32_t) {
    is_painting_ = false;
    if (needs_paint_)
      OnGetFrame(0);
  }

  void SwapBuffers() {
    context_.SwapBuffers(
        callback_factory_.NewCallback(&MPVInstance::PaintFinished));
  }

  void DidChangeView(const pp::View& view) override {
    int32_t new_width = static_cast<int32_t>(
        view.GetRect().width() * view.GetDeviceScale());
    int32_t new_height = static_cast<int32_t>(
        view.GetRect().height() * view.GetDeviceScale());
    // printf("@@@ RESIZE %d %d\n", new_width, new_height);

    // Always called on main thread so don't need locks.
    context_.ResizeBuffers(new_width, new_height);
    width_ = new_width;
    height_ = new_height;
    OnGetFrame(0);
  }

  void OnGetFrame(int32_t) {
    // Always called on main thread so don't need locks.
    if (is_painting_) {
      needs_paint_ = true;
    } else {
      is_painting_ = true;
      needs_paint_ = false;
      Render();
    }
  }

  void Render() {
    glSetCurrentContextPPAPI(context_.pp_resource());

    mpv_opengl_fbo mpfbo{static_cast<int>(0), width_, height_, 0};
    int flip_y{1};
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
        {MPV_RENDER_PARAM_FLIP_Y, &flip_y},
        {MPV_RENDER_PARAM_INVALID, nullptr}
    };

    mpv_render_context_render(mpv_gl_, params);
    SwapBuffers();
  }

  pp::CompletionCallbackFactory<MPVInstance> callback_factory_;
  mpv_handle* mpv_;
  std::unordered_set<std::string> registed_events_;
  mpv_render_context* mpv_gl_;
  pp::Graphics3D context_;
  bool is_painting_{false};
  bool needs_paint_{false};
  int32_t width_{0};
  int32_t height_{0};
};

class MPVModule : public pp::Module {
 public:
  MPVModule() : pp::Module() {}
  virtual ~MPVModule() {}

  virtual pp::Instance* CreateInstance(PP_Instance instance) {
    return new MPVInstance(instance);
  }
};

namespace pp {
Module* CreateModule() {
  return new MPVModule();
}
}  // namespace pp
