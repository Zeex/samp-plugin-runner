// Copyright (c) 2019 Zeex
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

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <list>
#include <new>
#include <string>
#include <thread>
#include <vector>
#include "plugin.h"
#include "plugincommon.h"
#include "amx/amx.h"
#include "amx/amxaux.h"

extern "C" {
  int AMXEXPORT amx_ConsoleInit(AMX *amx);
  int AMXEXPORT amx_ConsoleCleanup(AMX *amx);
  int AMXEXPORT amx_CoreInit(AMX *amx);
  int AMXEXPORT amx_CoreCleanup(AMX *amx);
  int AMXEXPORT amx_FileInit(AMX *amx);
  int AMXEXPORT amx_FileCleanup(AMX *amx);
  int AMXEXPORT amx_FloatInit(AMX *amx);
  int AMXEXPORT amx_FloatCleanup(AMX *amx);
  int AMXEXPORT amx_StringInit(AMX *amx);
  int AMXEXPORT amx_StringCleanup(AMX *amx);
  int AMXEXPORT amx_TimeInit(AMX *amx);
  int AMXEXPORT amx_TimeCleanup(AMX *amx);
}

namespace {

void logprintf(const char *format, ...);

const void *amx_functions[] = {
  (void *)amx_Align16,
  (void *)amx_Align32,
  nullptr,
  (void *)amx_Allot,
  (void *)amx_Callback,
	(void *)amx_Cleanup,
	(void *)amx_Clone,
  (void *)amx_Exec,
  (void *)amx_FindNative,
  (void *)amx_FindPublic,
  (void *)amx_FindPubVar,
  (void *)amx_FindTagId,
  (void *)amx_Flags,
  (void *)amx_GetAddr,
  (void *)amx_GetNative,
  (void *)amx_GetPublic,
  (void *)amx_GetPubVar,
  (void *)amx_GetString,
  (void *)amx_GetTag,
  (void *)amx_GetUserData,
  (void *)amx_Init,
  (void *)amx_InitJIT,
  (void *)amx_MemInfo,
  (void *)amx_NameLength,
  (void *)amx_NativeInfo,
  (void *)amx_NumNatives,
  (void *)amx_NumPublics,
  (void *)amx_NumPubVars,
  (void *)amx_NumTags,
  (void *)amx_Push,
  (void *)amx_PushArray,
  (void *)amx_PushString,
  (void *)amx_RaiseError,
  (void *)amx_Register,
  (void *)amx_Release,
  (void *)amx_SetCallback,
  (void *)amx_SetDebugHook,
  (void *)amx_SetString,
  (void *)amx_SetUserData,
  (void *)amx_StrLen,
  (void *)amx_UTF8Check,
  (void *)amx_UTF8Get,
  (void *)amx_UTF8Len,
  (void *)amx_UTF8Put
};

void *plugin_data[] {
  (void *)logprintf,    // PLUGIN_DATA_LOGPRINTF
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  nullptr,
  (void *)amx_functions // PLUGIN_DATA_AMX_EXPORTS
};

const char AMX_FILE_EXT[] = ".amx";
#ifdef _WIN32
  const char PLUGIN_EXT[] = ".dll";
#else
  const char PLUGIN_EXT[] = ".so";
#endif

std::atomic<bool> process_ticks{false};

void logprintf(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  printf("\n");
  va_end(args);
}

bool EndsWith(const std::string &s1, const std::string &s2) {
  if (s1.length() >= s2.length()) {
    return (s1.compare(s1.length() - s2.length(), s2.length(), s2) == 0);
  }
  return false;
}

Plugin *LoadPlugin(std::string path) {
  auto plugin = new(std::nothrow) Plugin;
  if (plugin == nullptr) {
    return nullptr;
  }

  if (!EndsWith(path, PLUGIN_EXT)) {
    path.append(PLUGIN_EXT);
  }

  auto error = plugin->Load(path, plugin_data);
  if (plugin->IsLoaded()) {
    std::printf("Loaded plugin: %s\n", path.c_str());
    return plugin;
  }

  std::printf("Could not load plugin: %s: ", path.c_str());
  switch (error) {
    case PLUGIN_ERROR_FAILED:
      std::printf("%s\n", plugin->GetFailMessage().c_str());
      break;
    case PLUGIN_ERROR_VERSION:
      std::printf("Unsupported version\n");
      break;
    case PLUGIN_ERROR_API:
      std::printf("Plugin does not conform to acrhitecture\n");
      break;
  }

  delete plugin;
  return nullptr;
}

cell AMX_NATIVE_CALL n_ExitProcess(AMX *amx, const cell *params) {
  std::exit(params[1]);
  return 0;
}

cell AMX_NATIVE_CALL n_CallLocalFunction(AMX *amx, const cell *params) {
  char *function_name;
  amx_StrParam(amx, params[1], function_name);

  int function_index;
  if (amx_FindPublic(amx, function_name, &function_index) != AMX_ERR_NONE) {
    return 0;
  }

  char *format;
  amx_StrParam(amx, params[2], format);

  cell num_args = params[0] / sizeof(cell);
  std::vector<cell> pushed_strings;

  if (format != nullptr) {
    std::size_t format_length = std::strlen(format);
    cell arg_index = 2;
    for (std::size_t i = 0;
         i < format_length && arg_index < num_args;
         i++, arg_index++) {
      cell value = params[arg_index + 1];
      switch (format[i]) {
        case 'd':
        case 'i':
        case 'f':
          amx_Push(amx, value);
          break;
        case 's': {
          char *s;
          amx_StrParam(amx, value, s);
          cell amx_addr;
          cell *phys_addr;
          if (amx_PushString(amx,
                             &amx_addr,
                             &phys_addr,
                             s,
                             false,
                             false) == AMX_ERR_NONE) {
            pushed_strings.push_back(amx_addr);
          }
          break;
        }
      }
    }
  }

  cell retval = 0;
  amx_Exec(amx, &retval, function_index);

  for (std::size_t i = pushed_strings.size(); i > 0; i--) {
    amx_Release(amx, pushed_strings[i - 1]);
  }

  return retval;
}

bool CheckAmxNatives(AMX *amx) {
  bool result = true;
  AMX_HEADER *hdr = reinterpret_cast<AMX_HEADER*>(amx->base);
  AMX_FUNCSTUBNT *natives =
    reinterpret_cast<AMX_FUNCSTUBNT*>(hdr->natives +
      reinterpret_cast<unsigned int>(amx->base));
  AMX_FUNCSTUBNT *libraries =
    reinterpret_cast<AMX_FUNCSTUBNT*>(hdr->libraries +
      reinterpret_cast<unsigned int>(amx->base));
  for (AMX_FUNCSTUBNT *n = natives; n < libraries; n++) {
    if (n->address == 0) {
      char *name = reinterpret_cast<char*>(n->nameofs +
          reinterpret_cast<unsigned int>(hdr));
      std::printf("Native function is not registered: %s\n", name);
      result = false;
    }
  }
  return result;
}

bool LoadScript(AMX *amx, std::string amx_path) {
  auto amx_error = aux_LoadProgram(amx, amx_path.c_str(), nullptr);
  if (amx_error != AMX_ERR_NONE) {
    std::printf("Could not load script: %s: %s\n",
                amx_path.c_str(), aux_StrError(amx_error));
    return false;
  }

  std::printf("Loaded script: %s\n", amx_path.c_str());
  amx_CoreInit(amx);
  amx_ConsoleInit(amx);
  amx_FloatInit(amx);
  amx_StringInit(amx);
  amx_FileInit(amx);

  static const AMX_NATIVE_INFO natives[] = {
    "ExitProcess", n_ExitProcess,
    "CallLocalFunction", n_CallLocalFunction
  };
  int num_natives = static_cast<int>(sizeof(natives) / sizeof(natives[0]));
  amx_Register(amx, natives, num_natives);

  return true;
}

int RunScriptMain(AMX *amx) {
  cell retval = 0;
  int amx_error = amx_Exec(amx, &retval, AMX_EXEC_MAIN);
  if (amx_error != AMX_ERR_NONE) {
    std::printf("Error while executing main: %s (%d)\n",
                aux_StrError(amx_error), amx_error);
    return -1;
  }
  return retval;
}

void UnloadScript(AMX *amx) {
  amx_CoreCleanup(amx);
  amx_ConsoleCleanup(amx);
  amx_FloatCleanup(amx);
  amx_StringCleanup(amx);
  amx_FileCleanup(amx);
}

} // anonymous namespace

int main(int argc, char **argv) {
  // Find the start of config options (`--`).
  int opts = 0;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--") == 0) {
      opts = i + 1;
      if (opts == argc) {
        opts = 0; // `--` with no following parameters.
      }
      argc = i; // Remainder of the code stops before `--`.
      break;
    }
  }

  if (argc < 2) {
    std::fprintf(stderr,
                 "Usage: plugin-runner [plugin1 [plugin2 [...]]] amx_file [-- opt1 [opt2 [...]]]\n");
    return EXIT_FAILURE;
  }

  std::list<Plugin *> plugins;
  if (argc >= 3) {
    for (int i = 1; i < argc - 1; i++) {
      auto plugin = LoadPlugin(argv[i]);
      if (plugin != nullptr) {
        plugins.push_back(plugin);
      }
    }
  }

  std::string amx_path = argv[argc - 1];
  if (!EndsWith(amx_path, AMX_FILE_EXT)) {
    amx_path.append(AMX_FILE_EXT);
  }

  AMX amx = {0};
  int exit_status = EXIT_SUCCESS;

  if (LoadScript(&amx, amx_path)) {
    for (auto &plugin : plugins) {
      if (plugin->GetSupportsFlags() & SUPPORTS_AMX_NATIVES) {
        plugin->AmxLoad(&amx);
      }
      if (plugin->GetSupportsFlags() & SUPPORTS_PROCESS_TICK) {
        process_ticks = true;
      }
    }
    if (CheckAmxNatives(&amx)) {
      exit_status = RunScriptMain(&amx);
    }
    UnloadScript(&amx);
  }

  if (process_ticks) {
    std::printf("Running indefinitely because ProcessTick() was requested\n");
    while (process_ticks) {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      for (auto &plugin : plugins) {
        if (plugin->IsLoaded()
            && plugin->GetSupportsFlags() & SUPPORTS_PROCESS_TICK) {
          plugin->ProcessTick();
        }
      }
    }
  }

  for (auto &plugin : plugins) {
    if (!plugin->IsLoaded()) {
      continue;
    }
    if (amx.base != nullptr
        && plugin->GetSupportsFlags() & SUPPORTS_AMX_NATIVES) {
      plugin->AmxUnload(&amx);
    }
    plugin->Unload();
  }
  plugins.erase(plugins.begin(), plugins.end());

  return exit_status;
}
