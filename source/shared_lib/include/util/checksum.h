// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_UTIL_CHECKSUM_H_
#define _SHARED_UTIL_CHECKSUM_H_

#include <string>
#include <map>
#include "data_types.h"
#include "thread.h"
#include "leak_dumper.h"

using std::string;
using namespace Shared::Platform;

namespace Shared{ namespace Util{

// =====================================================
//	class Checksum
// =====================================================

class Checksum {
private:
	int32	sum;
	int32	r;
    int32	c1;
    int32	c2;
	std::map<string,int32> fileList;

	static Mutex fileListCacheSynchAccessor;
	static std::map<string,int32> fileListCache;

	void addSum(int32 value);
	bool addFileToSum(const string &path);

public:
	Checksum();

	int32 getSum();
	int32 getFinalFileListSum();
	int32 getFileCount();

	int32 addByte(int8 value);
	void addString(const string &value);
	int32 addInt(const int32 &value);
	void addFile(const string &path);

	static void removeFileFromCache(const string file);
	static void clearFileCache();
};

}}//end namespace

#endif
