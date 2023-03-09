// AUTOGENERATED COPYRIGHT HEADER START
// Copyright (C) 2020-2023 Michael Fabian 'Xaymar' Dirks <info@xaymar.com>
// Copyright (C) 2022 lainon <GermanAizek@yandex.ru>
// AUTOGENERATED COPYRIGHT HEADER END

#include "util-library.hpp"
#include "util-platform.hpp"

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__) // Windows
#define ST_WINDOWS
#else
#define ST_UNIX
#endif

#include "warning-disable.hpp"
#include <unordered_map>
#if defined(ST_WINDOWS)
#include <Windows.h>
#elif defined(ST_UNIX)
#include <dlfcn.h>
#endif
#include "warning-enable.hpp"

streamfx::util::library::library(std::filesystem::path file) : _library(nullptr), _owner(true)
{
#if defined(ST_WINDOWS)
	SetLastError(ERROR_SUCCESS);
	auto wfile = ::streamfx::util::platform::utf8_to_native(file.u8string());
	if (file.is_absolute()) {
		_library = reinterpret_cast<void*>(LoadLibraryExW(wfile.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR));
	} else {
		_library = reinterpret_cast<void*>(LoadLibraryExW(wfile.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS));
	}
	if (!_library) {
		std::string ex    = "Failed to load library.";
		DWORD       error = GetLastError();
		if (error != ERROR_PROC_NOT_FOUND) {
			PSTR message = NULL;
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
						   NULL, error, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPSTR)&message, 0, NULL);
			if (message) {
				ex = message;
				LocalFree(message);
				throw std::runtime_error(ex);
			}
		}
		throw std::runtime_error(ex);
	}
#elif defined(ST_UNIX)
	_library = dlopen(file.u8string().c_str(), RTLD_LAZY);
	if (!_library) {
		if (char* error = dlerror(); error)
			throw std::runtime_error(error);
		else
			throw std::runtime_error("Failed to load library.");
	}
#endif
}

streamfx::util::library::library(void* library) : _library(library), _owner(false) {}

streamfx::util::library::~library()
{
	if (_owner) {
#if defined(ST_WINDOWS)
		FreeLibrary(reinterpret_cast<HMODULE>(_library));
#elif defined(ST_UNIX)
		dlclose(_library);
#endif
	}
}

void* streamfx::util::library::load_symbol(std::string_view name)
{
#if defined(ST_WINDOWS)
	return reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HMODULE>(_library), name.data()));
#elif defined(ST_UNIX)
	return reinterpret_cast<void*>(dlsym(_library, name.data()));
#endif
}

static std::unordered_map<std::string, std::weak_ptr<::streamfx::util::library>> libraries;

std::shared_ptr<::streamfx::util::library> streamfx::util::library::load(std::filesystem::path file)
{
	auto kv = libraries.find(file.u8string());
	if (kv != libraries.end()) {
		if (auto ptr = kv->second.lock(); ptr)
			return ptr;
		libraries.erase(kv);
	}

	auto ptr = std::make_shared<::streamfx::util::library>(file);
	libraries.emplace(file.u8string(), ptr);

	return ptr;
}

std::shared_ptr<::streamfx::util::library> streamfx::util::library::load(std::string_view name)
{
	return load(std::filesystem::u8path(name));
}

std::shared_ptr<::streamfx::util::library> streamfx::util::library::load(obs_module_t* instance)
{
	auto path = std::filesystem::absolute({obs_get_module_binary_path(instance)});
	auto kv   = libraries.find(path.u8string());
	if (kv != libraries.end()) {
		if (auto ptr = kv->second.lock(); ptr) {
			return ptr;
		}
		libraries.erase(kv);
	}

	auto ptr = std::make_shared<::streamfx::util::library>(obs_get_module_lib(instance));
	libraries.emplace(path.u8string(), ptr);

	return ptr;
}
