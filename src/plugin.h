// Copyright (c) 2011-2012, Zeex
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

#ifndef PLUGIN_H
#define PLUGIN_H

#include <string>
#include "plugincommon.h"
#include "amx/amx.h"

enum PluginError {
  PLUGIN_ERROR_OK,
  PLUGIN_ERROR_FAILED,
  PLUGIN_ERROR_VERSION,
  PLUGIN_ERROR_API
};

class Plugin {
 public:
  typedef unsigned int (PLUGIN_CALL *Supports_t)();
  typedef bool (PLUGIN_CALL *Load_t)(void **ppData);
  typedef void (PLUGIN_CALL *Unload_t)();
  typedef int (PLUGIN_CALL *AmxLoad_t)(AMX *amx);
  typedef int (PLUGIN_CALL *AmxUnload_t)(AMX *amx);
  typedef void (PLUGIN_CALL *ProcessTick_t)();

  Plugin();
  explicit Plugin(const std::string &filename);
  ~Plugin();

  PluginError Load(void **ppData);
  PluginError Load(const std::string &filename, void **ppData);
  void Unload();

  void *GetSymbol(const std::string &name) const;

  bool IsLoaded() const { return is_loaded_; }
  operator bool() const { return IsLoaded(); }

  const std::string GetPath() const {
    return path_;
  }
  unsigned int GetSupportsFlags() const {
    return supports_flags_;
  }
  std::string GetFailMessage() const {
    return failmsg_;
  }

  int AmxLoad(AMX *amx) const;
  int AmxUnload(AMX *amx) const;
  void ProcessTick() const;

 private:
  std::string path_;
  void *handle_;
  unsigned int supports_flags_;
  bool is_loaded_;
  std::string failmsg_;
  AmxLoad_t AmxLoad_;
  AmxUnload_t AmxUnload_;
  ProcessTick_t ProcessTick_;

 private:
  Plugin(const Plugin &other);
  Plugin &operator=(const Plugin &other);
};

#endif // !PLUGIN_H
