#ifndef MACAPILOCK_HPP
#define MACAPILOCK_HPP

#pragma once

#include "Definitions.hpp"
#include "ReentrantLock.hpp"

namespace GS {
	GSROOT_DLL_EXPORT ReentrantLock& GetMacAPILock (void);
}
#endif
