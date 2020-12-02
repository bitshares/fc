The following license only applies to code that was contributed to this repository in the context of BitShares worker proposals and/or BitShares-1 delegate workers. Other code may be licensed indirectly by our "sister" repositories https://github.com/EOSIO/fc and https://github.com/bytemaster/fc/tree/phoenix .

All subsequent contributions to this repository are licensed by their respective authors under this same license as well, unless explicitly stated otherwise.

Furthermore, this repository includes code from third party authors. See the list at the end of this file.

----

Copyright (c) 2018-2019 BitShares Blockchain Foundation and contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

----

The following terms apply to third-party code included here. Most have to be published along with binaries of this software.
Some of the files are only used for building binaries of this software, without becoming an actual part of those binaries. Their respective licenses also do not apply to the binaries.

include/bloom_filter.hpp is copyright Arash Partow and covered by the MIT license. See http://www.opensource.org/licenses/MIT .

src/utf8 is copyright Nemanja Trifunovic and covered by the MIT license (Thrift variant). See licenses/MIT_Thrift_LICENSE.md.

src/crypto/base58.cpp is copyright Satoshi Nakamoto and The Bitcoin Developers and is covered by the MIT/X11 license. See licenses/MIT_X11_LICENSE.md.

src/crypto/base64.cpp is copyright René Nyffenegger and covered by the zlib/libpng license. See licenses/ZLIB_LICENSE.md.

src/crypto/city.cpp and libraries/fc/include/fc/crypto/city.hpp are copyright Google, Inc. and covered by the MIT/X11 license. See licenses/MIT_X11_LICENSE.md.

CMakeModules/CheckLibcxxAtomic.cmake was taken from the LLVM repository and is licensed under the Apache-2.0 license. See licenses/Apache.txt .

GitVersionGen/GetGitRevisionDescription.cmake is copyright Iowa State University and covered by the Boost Software License. See licenses/Boost_License-1.0.txt.

vendor/editline is copyright Simmule Turner and Rich Salz and covered by a BSD-like license. See vendor/editline/LICENSE.

vendor/secp256k1-zkp is copyright Pieter Wuille and covered by the MIT/X11 license. See vendor/secp256k1-zkp/COPYING.

vendor/websocketpp is copyright Peter Thorson and covered by a 3-clause BSD-license. It also includes code from 4th-party authors with various copyrights, see vendor/websocketpp/COPYING.

.github/workflows/build-and-test-macos.yml: the code in the step "Use XCode 12.2 for Big Sur" was taken from https://github.com/d12frosted/homebrew-emacs-plus/commit/315f55c7a36ba13e533a8a9bb7111b489398e86a , under the MIT License, Copyright (c) 2016-2020 Boris Buliga. See licenses/LICENSE-MIT.d12frosted.txt .
