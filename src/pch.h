#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <F4SE/F4SE.h>
#include <RE/C/ControlMap.h>
#include <Windows.h>
#include <Psapi.h>
#undef ERROR // Windows' ERROR macro conflicts with REX::ERROR.
#include <array>
#include <vector>
