// Copyright (c) 2011-2012 Zeex
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "plugin.h"
#include "plugincommon.h"
#include "amx/amx.h"

#ifdef _WIN32
  #include <windows.h>
#else
  #include <dlfcn.h>
#endif

Plugin::Plugin()
 : is_loaded_(false),
   handle_(0),
   AmxLoad_(0),
   AmxUnload_(0),
   ProcessTick_(0)
{
}

Plugin::Plugin(const std::string &path)
 : path_(path),
   is_loaded_(false),
   handle_(0),
   AmxLoad_(0),
   AmxUnload_(0),
   ProcessTick_(0)
{
}

Plugin::~Plugin() {
  Unload();
}

PluginError Plugin::Load(void **ppData) {
  if (!path_.empty()) {
    return Load(path_, ppData);
  }
  return PLUGIN_ERROR_FAILED;
}

PluginError Plugin::Load(const std::string &filename, void **ppData) {
  if (!is_loaded_) {
    #ifdef _WIN32
      handle_ = (void*)LoadLibrary(filename.c_str());
    #else
      handle_ = dlopen(filename.c_str(), RTLD_NOW);
    #endif

    if (handle_ == 0) {
      #ifdef _WIN32
        DWORD error = GetLastError();
        DWORD flags = FORMAT_MESSAGE_ARGUMENT_ARRAY
                    | FORMAT_MESSAGE_FROM_SYSTEM
                    | FORMAT_MESSAGE_ALLOCATE_BUFFER
                    | FORMAT_MESSAGE_IGNORE_INSERTS
                    ;
        DWORD lang = LANG_SYSTEM_DEFAULT;
        LPSTR text = NULL;
        if (FormatMessageA(flags, NULL, error, lang, (LPSTR)&text, 0, NULL) != 0) {
          failmsg_.assign(text);
          failmsg_.erase(failmsg_.length() - 2, 2); // remove trailing \r\n
          LocalFree((HLOCAL)text);
        }
      #else
        failmsg_.assign(dlerror());
      #endif
      return PLUGIN_ERROR_FAILED;
    }

    Supports_t Supports = (Supports_t)GetSymbol("Supports");
    if (Supports == 0) {
      return PLUGIN_ERROR_FAILED;
    }

    supports_flags_ = Supports();

    if ((supports_flags_ & SUPPORTS_VERSION_MASK) > SUPPORTS_VERSION) {
      return PLUGIN_ERROR_VERSION;
    }

    if ((supports_flags_ & SUPPORTS_AMX_NATIVES) != 0) {
      if ((AmxLoad_ = (AmxLoad_t)GetSymbol("AmxLoad")) == 0
          || (AmxUnload_ = (AmxUnload_t)GetSymbol("AmxUnload")) == 0) {
        return PLUGIN_ERROR_API;
      }
    }

    if ((supports_flags_ & SUPPORTS_PROCESS_TICK) != 0) {
      ProcessTick_ = (ProcessTick_t)GetSymbol("ProcessTick");
      if (ProcessTick_ == 0) {
        return PLUGIN_ERROR_API;
      }
    }

    Load_t Load = (Load_t)GetSymbol("Load");
    if (Load == 0) {
      return PLUGIN_ERROR_API;
    }

    if (Load(ppData)) {
      path_ = filename;
      is_loaded_ = true;
      return PLUGIN_ERROR_OK;
    }
  }

  return PLUGIN_ERROR_FAILED;
}

void Plugin::Unload() {
  if (is_loaded_) {
    Unload_t Unload = (Unload_t)GetSymbol("Unload");
    if (Unload != 0) {
      Unload();
    }
    #ifdef _WIN32
      FreeLibrary((HMODULE)handle_);
    #else
      dlclose(handle_);
    #endif
  }
}

void *Plugin::GetSymbol(const std::string &name) const {
  #ifdef _WIN32
    return (void*)GetProcAddress((HMODULE)handle_, name.c_str());
  #else
    return dlsym(handle_, name.c_str());
  #endif
}

int Plugin::AmxLoad(AMX *amx) const {
  if (AmxLoad_ != 0) {
    return AmxLoad_(amx);
  }
  return AMX_ERR_NONE;
}

int Plugin::AmxUnload(AMX *amx) const {
  if (AmxUnload_ != 0) {
    return AmxUnload_(amx);
  }
  return AMX_ERR_NONE;
}

void Plugin::ProcessTick() const {
  if (ProcessTick_ != 0) {
    ProcessTick_();
  }
}
