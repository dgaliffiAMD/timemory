// MIT License
//
// Copyright (c) 2020, The Regents of the University of California,
// through Lawrence Berkeley National Laboratory (subject to receipt of any
// required approvals from the U.S. Dept. of Energy).  All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "timemory/hash/macros.hpp"
#include "timemory/hash/types.hpp"
#include "timemory/utility/macros.hpp"

#include <atomic>
#include <memory>
#include <utility>

namespace tim
{
//
//--------------------------------------------------------------------------------------//
//
template <typename Tp, typename Tag, typename PtrT, typename PairT>
std::unique_ptr<PairT>&
get_shared_ptr_pair()
{
    static std::atomic<int64_t> _count{ 0 };
    static auto                 _main = std::make_shared<Tp>();
    static thread_local auto    _n    = _count++;
    static thread_local auto    _local =
        std::make_unique<PairT>(_main, (_n == 0) ? _main : std::make_shared<Tp>());
    return _local;
}
//
//--------------------------------------------------------------------------------------//
//
template <typename Tp, typename Tag, typename PtrT, typename PairT>
PtrT
get_shared_ptr_pair_instance()
{
    static thread_local auto& _v = get_shared_ptr_pair<Tp, Tag>();
    return (_v) ? _v->second : PtrT{};
}
//
//--------------------------------------------------------------------------------------//
//
template <typename Tp, typename Tag, typename PtrT, typename PairT>
PtrT
get_shared_ptr_pair_main_instance()
{
    return get_shared_ptr_pair<Tp, Tag>() ? get_shared_ptr_pair<Tp, Tag>()->first
                                          : PtrT{};
}
//
//--------------------------------------------------------------------------------------//
//
template <typename Tp, typename Tag, typename PtrT>
PtrT
get_shared_ptr_lone_instance()
{
    static auto _v = std::make_shared<Tp>();
    return _v;
}
//
//--------------------------------------------------------------------------------------//
//
}  // namespace tim
