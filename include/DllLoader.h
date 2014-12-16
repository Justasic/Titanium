#pragma once
#include <unistd.h>
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <string>
#include <dlfcn.h>
#include <cassert>

class DynamicLibrary
{
protected:
	void *handle;
	std::string name;
public:
	DynamicLibrary(const std::string &str) : handle(nullptr), name(str)
	{
		assert(!str.empty());

		this->handle = dlopen(str.c_str(), RTLD_LAZY);
		if (!this->handle)
		{
			fprintf(stderr, "Failed to open module %s: %s\n", str.c_str(), dlerror());
			return;
		}
	}

	~DynamicLibrary()
	{
		if (this->handle)
			dlclose(this->handle);
	}

	template<typename T> T ResolveSymbol(const std::string &str)
	{
		// Union-cast to get around C++ warnings.
		union {
			T func;
			void *ptr;
		} fn;

		fn.ptr = dlsym(this->handle, str.c_str());
		if (!fn.ptr)
		{
			fprintf(stderr, "Failed to resolve symbol %s: %s\n", str.c_str(), dlerror());
			return T();
		}

		return fn.func;
	}

	std::string GetName() const
	{
		return this->name;
	}
};
