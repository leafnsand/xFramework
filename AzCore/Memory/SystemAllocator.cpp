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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/AllocatorManager.h>

#include <AzCore/Memory/OSAllocator.h>

#include <AzCore/std/functional.h>

#define AZCORE_SYS_ALLOCATOR_HPPA  // If you disable this make sure you start building the heapschema.cpp

#ifdef AZCORE_SYS_ALLOCATOR_HPPA
#   include <AzCore/Memory/HphaSchema.h>
#else
#   include <AzCore/Memory/heapschema.h>
#endif

using namespace AZ;

//////////////////////////////////////////////////////////////////////////
// Globals - we use global storage for the first memory schema, since we can't use dynamic memory!
static bool g_isSystemSchemaUsed = false;
#ifdef AZCORE_SYS_ALLOCATOR_HPPA
static AZStd::aligned_storage<sizeof(HphaSchema), AZStd::alignment_of<HphaSchema>::value>::type g_systemSchema;
#else
static AZStd::aligned_storage<sizeof(HeapSchema), AZStd::alignment_of<HeapSchema>::value>::type g_systemSchema;
#endif

//////////////////////////////////////////////////////////////////////////

//=========================================================================
// SystemAllocator
// [9/2/2009]
//=========================================================================
SystemAllocator::SystemAllocator()
    : m_isCustom(false)
    , m_allocator(nullptr)
{
}

//=========================================================================
// ~SystemAllocator
//=========================================================================
SystemAllocator::~SystemAllocator()
{
    if (IsReady())
    {
        Destroy();
    }
}

//=========================================================================
// ~Create
// [9/2/2009]
//=========================================================================
bool
SystemAllocator::Create(const Descriptor& desc)
{
    AZ_Assert(IsReady() == false, "System allocator was already created!");
    if (IsReady())
    {
        return false;
    }

    if (!AllocatorInstance<OSAllocator>::IsReady())
    {
        AllocatorInstance<OSAllocator>::Create();  // debug allocator is such that there is no much point to free it
    }
    bool isReady = false;
    if (desc.m_custom)
    {
        m_isCustom = true;
        m_allocator = desc.m_custom;
        isReady = true;
    }
    else
    {
        m_isCustom = false;
#ifdef AZCORE_SYS_ALLOCATOR_HPPA
        HphaSchema::Descriptor      heapDesc;
        heapDesc.m_pageSize     = desc.m_heap.m_pageSize;
        heapDesc.m_poolPageSize = desc.m_heap.m_poolPageSize;
        AZ_Assert(desc.m_heap.m_numMemoryBlocks <= 1, "We support max1 memory block at the moment!");
        if (desc.m_heap.m_numMemoryBlocks > 0)
        {
            heapDesc.m_memoryBlock = desc.m_heap.m_memoryBlocks[0];
            heapDesc.m_memoryBlockByteSize = desc.m_heap.m_memoryBlocksByteSize[0];
        }
        heapDesc.m_subAllocator = desc.m_heap.m_subAllocator;
        heapDesc.m_isPoolAllocations = desc.m_heap.m_isPoolAllocations;
#else
        HeapSchema::Descriptor      heapDesc;
        memcpy(heapDesc.m_memoryBlocks, desc.m_heap.m_memoryBlocks, sizeof(heapDesc.m_memoryBlocks));
        memcpy(heapDesc.m_memoryBlocksByteSize, desc.m_heap.m_memoryBlocksByteSize, sizeof(heapDesc.m_memoryBlocksByteSize));
        heapDesc.m_numMemoryBlocks = desc.m_heap.m_numMemoryBlocks;
#endif
        if (&AllocatorInstance<SystemAllocator>::Get() == (void*)this) // if we are the system allocator
        {
            AZ_Assert(!g_isSystemSchemaUsed, "AZ::SystemAllocator MUST be created first! It's the source of all allocations!");
#ifdef AZCORE_SYS_ALLOCATOR_HPPA
            m_allocator = new(&g_systemSchema)HphaSchema(heapDesc);
#else
            m_allocator = new(&g_systemSchema)HeapSchema(heapDesc);
#endif
            g_isSystemSchemaUsed = true;
            isReady = true;
        }
        else
        {
            // this class should be inheriting from SystemAllocator
            AZ_Assert(AllocatorInstance<SystemAllocator>::IsReady(), "System allocator must be created before any other allocator! They allocate from it.");
#ifdef AZCORE_SYS_ALLOCATOR_HPPA
            m_allocator = azcreate(HphaSchema, (heapDesc), SystemAllocator);
#else
            m_allocator = azcreate(HeapSchema, heapDesc, SystemAllocator);
#endif
            if (m_allocator == NULL)
            {
                isReady = false;
            }
            else
            {
                isReady = true;
            }
        }
    }

    if (isReady)
    {
        OnCreated();
    }

    return IsReady();
}

//=========================================================================
// Allocate
// [9/2/2009]
//=========================================================================
void
SystemAllocator::Destroy()
{
    OnDestroy();

    if (g_isSystemSchemaUsed)
    {
        int dummy;
        (void)dummy;
    }

    if (!m_isCustom)
    {
        if ((void*)m_allocator == (void*)&g_systemSchema)
        {
#ifdef AZCORE_SYS_ALLOCATOR_HPPA
            static_cast<HphaSchema*>(m_allocator)->~HphaSchema();
#else
            static_cast<HeapSchema*>(m_allocator)->~HeapSchema();
#endif
            g_isSystemSchemaUsed = false;
        }
        else
        {
            azdestroy(m_allocator);
        }
    }
}

//=========================================================================
// Allocate
// [9/2/2009]
//=========================================================================
SystemAllocator::pointer_type
SystemAllocator::Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord)
{
    AZ_Assert(byteSize > 0, "You can not allocate 0 bytes!");
    AZ_Assert((alignment & (alignment - 1)) == 0, "Alignment must be power of 2!");

    SystemAllocator::pointer_type address = m_allocator->Allocate(byteSize, alignment, flags, name, fileName, lineNum, suppressStackRecord + 1);

    if (address == 0)
    {
        // Free all memory we can and try again!
        g_allocMgr.GarbageCollect();

        address = m_allocator->Allocate(byteSize, alignment, flags, name, fileName, lineNum, suppressStackRecord + 1);
    }

    AZ_Assert(address != 0, "SystemAllocator: Failed to allocate %d bytes aligned on %d (flags: 0x%08x) %s : %s (%d)!", byteSize, alignment, flags, name ? name : "(no name)", fileName ? fileName : "(no file name)", lineNum);

    return address;
}

//=========================================================================
// DeAllocate
// [9/2/2009]
//=========================================================================
void
SystemAllocator::DeAllocate(pointer_type ptr, size_type byteSize, size_type alignment)
{
    m_allocator->DeAllocate(ptr, byteSize, alignment);
}

//=========================================================================
// ReAllocate
// [9/13/2011]
//=========================================================================
SystemAllocator::pointer_type
SystemAllocator::ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment)
{
    pointer_type newAddress = m_allocator->ReAllocate(ptr, newSize, newAlignment);
    return newAddress;
}

//=========================================================================
// Resize
// [8/12/2011]
//=========================================================================
SystemAllocator::size_type
SystemAllocator::Resize(pointer_type ptr, size_type newSize)
{
    size_type resizedSize = m_allocator->Resize(ptr, newSize);
    return resizedSize;
}

//=========================================================================
//
// [8/12/2011]
//=========================================================================
SystemAllocator::size_type
SystemAllocator::AllocationSize(pointer_type ptr)
{
    size_type allocSize = m_allocator->AllocationSize(ptr);
    return allocSize;
}

#endif // #ifndef AZ_UNITY_BUILD