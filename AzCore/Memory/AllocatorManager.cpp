/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#ifndef AZ_UNITY_BUILD

#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/Memory.h>

#include <AzCore/Memory/OSAllocator.h>

#include <AzCore/std/parallel/lock.h>

using namespace AZ;

//////////////////////////////////////////////////////////////////////////
// The only allocator manager instance.
AZ::AllocatorManager AZ::g_allocMgr;
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// AllocatorManager
// [9/11/2009]
//=========================================================================
AllocatorManager::AllocatorManager()
{
    m_numAllocators = 0;
    m_isAllocatorLeaking = false;
}

//=========================================================================
// ~AllocatorManager
// [9/11/2009]
//=========================================================================
AllocatorManager::~AllocatorManager()
{
    // one exception is the debug allocator we close it automatically if the user has not.
    if (m_numAllocators == 1 && AllocatorInstance<OSAllocator>::IsReady())
    {
        AllocatorInstance<OSAllocator>::Destroy();
    }
    if (!m_isAllocatorLeaking)
    {
        AZ_Assert(m_numAllocators == 0, "There are still %d registered allocators!", m_numAllocators);
    }
}

//=========================================================================
// RegisterAllocator
// [9/17/2009]
//=========================================================================
void
AllocatorManager::RegisterAllocator(class IAllocator* alloc)
{
    AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
    AZ_Assert(m_numAllocators < m_maxNumAllocators, "Too many allocators %d! Max is %d", m_numAllocators, m_maxNumAllocators);
    m_allocators[m_numAllocators++] = alloc;
}

//=========================================================================
// UnRegisterAllocator
// [9/17/2009]
//=========================================================================
void
AllocatorManager::UnRegisterAllocator(class IAllocator* alloc)
{
    AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);
    for (int i = 0; i < m_numAllocators; ++i)
    {
        if (m_allocators[i] == alloc)
        {
            --m_numAllocators;
            m_allocators[i] = m_allocators[m_numAllocators];
        }
    }
}

//=========================================================================
// GarbageCollect
// [9/11/2009]
//=========================================================================
void
AllocatorManager::GarbageCollect()
{
    AZStd::lock_guard<AZStd::mutex> lock(m_allocatorListMutex);

    for (int i = 0; i < m_numAllocators; ++i)
    {
        m_allocators[i]->GarbageCollect();
    }
}

//=========================================================================
// AddOutOfMemoryListener
// [12/2/2010]
//=========================================================================
bool
AllocatorManager::AddOutOfMemoryListener(OutOfMemoryCBType& cb)
{
    AZ_Warning("Memory", !m_outOfMemoryListener, "Out of memory listener was already installed!");
    if (!m_outOfMemoryListener)
    {
        m_outOfMemoryListener = cb;
        return true;
    }

    return false;
}

//=========================================================================
// RemoveOutOfMemoryListener
// [12/2/2010]
//=========================================================================
void
AllocatorManager::RemoveOutOfMemoryListener()
{
    m_outOfMemoryListener = 0;
}

//=========================================================================
// MemoryBreak
// [2/24/2011]
//=========================================================================
AllocatorManager::MemoryBreak::MemoryBreak()
{
    addressStart = NULL;
    addressEnd = NULL;
    byteSize = 0;
    alignment = static_cast<size_t>(0xffffffff);
    name = NULL;

    fileName = NULL;
    lineNum = -1;
}

//=========================================================================
// SetMemoryBreak
// [2/24/2011]
//=========================================================================
void
AllocatorManager::SetMemoryBreak(int slot, const MemoryBreak& mb)
{
    AZ_Assert(slot < MaxNumMemoryBreaks, "Invalid slot index");
    m_activeBreaks |= 1 << slot;
    m_memoryBreak[slot] = mb;
}

//=========================================================================
// ResetMemoryBreak
// [2/24/2011]
//=========================================================================
void
AllocatorManager::ResetMemoryBreak(int slot)
{
    if (slot == -1)
    {
        m_activeBreaks = 0;
    }
    else
    {
        AZ_Assert(slot < MaxNumMemoryBreaks, "Invalid slot index");
        m_activeBreaks &= ~(1 << slot);
    }
}
#endif // #ifndef AZ_UNITY_BUILD