// Copyright (c) 2012-2018, The CryptoNote developers, The Bytecoin developers.
// Licensed under the GNU Lesser General Public License. See LICENSING.md for details.

#pragma once

// defines are for Windows resource compiler
#define BYTECOIN_VERSION_WINDOWS_COMMA 0, 18, 3, 18
#define BYTECOIN_VERSION_WINDOWS_STRING "0.18.3.18 (BETA)"

#ifdef __cplusplus

namespace bytecoin {
inline const char *app_version() { return BYTECOIN_VERSION_WINDOWS_STRING; }
}

#endif
