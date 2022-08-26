//-------------------------------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
// A simple template that implements a dynamically sized
// array using memory from the heap.
//
//-------------------------------------------------------------------------------------------------

#pragma once

template<class TYPE>
class DynamicArray
{
public:
    NEW_CTOR_SAFE()

    // Make sure we're zeroed.
    DynamicArray()
    {
        memset(this, 0, sizeof(*this));
    }

    // Makes sure that this goes away.
    ~DynamicArray()
    {
        Destroy();
    }

    // Set the initial size of the array.
    void SizeArray(_In_ unsigned long ulSlots)
    {
        Destroy();

        m_array = VBAllocator::AllocateArray<TYPE>(ulSlots);
        m_cEntries = 0;
        m_cSlots   = ulSlots;
    }

    // Copy the contents of one array into another.
    void TransferFromArgument(DynamicArray<TYPE> *pda)
    {
        Destroy();

        m_cSlots = pda->m_cSlots;
        m_cEntries = pda->m_cEntries;
        m_array = pda->m_array;

        pda->m_cSlots = 0;
        pda->m_cEntries = 0;
        pda->m_array = NULL;
    }

    // Destroys the array.
    void Destroy()
    {
        // Free the contents.
        for (unsigned i = 0; i < m_cEntries; i++)
        {
            // This will cause the contents of our thinggy to get
            // destroyed.  Neato, eh?
            //
            // Note (Microsoft): The destructor will not get called if the object you are storing
            // in the array is a pointer, so you will have to remove every element yourself
            // and call delete on it.

            // 


            m_array[i].~TYPE();
        }

        // Free the array.
        if (m_array)
        {
            VBFree(m_array);
        }

        // Reset the members.
        memset(this, 0, sizeof(DynamicArray<TYPE>));
    }

    // Resets the array without destroying any of its elements
    // (good to use if the elements stored in the array are pointers).
    void Collapse()
    {
        // Free the array.
        if (m_array)
        {
            VBFree(m_array);
        }

        // Reset the members.
        memset(this, 0, sizeof(DynamicArray<TYPE>));
    }

    // Resets the array so it has no size.
    void Reset()
    {
        Shrink(m_cEntries);
    }

    //
    // Manipulation methods.
    //

    // Get the number of entries in the array.
    ULONG Count()
    {
        return m_cEntries;
    }

    // Get the array.
    TYPE *Array()
    {
        return m_array;
    }

    // Get an element of an array.
    TYPE &Element(unsigned iElement)
    {
        VSASSERT(iElement < m_cEntries, "Out of range.");

        return m_array[iElement];
    }

    // Grow the size of the array.  Returns the first element that
    // was added.
    //
    TYPE &Grow(ULONG cGrow = 1)
    {
        VSASSERT(cGrow > 0, "Must be greater than 0");

        if (m_cEntries + cGrow > m_cSlots)
        {

            // Expand the array.
            ULONG cExpand = m_cSlots > cGrow ? m_cSlots : cGrow;

            TYPE *rgt = (TYPE *)VBRealloc(
                m_array,
                VBMath::Multiply(
                    VBMath::Add(m_cSlots, cExpand), 
                    sizeof(TYPE))); 

            // Zero the expanded array.
            memset(rgt + m_cSlots, 0, cExpand * sizeof(TYPE));

            // Set the new memory.
            m_cSlots += cExpand;
            m_array = rgt;
        }

        // Get the element that was added.
        unsigned cFirst = m_cEntries;

        // Grow the number of slots used.
        m_cEntries += cGrow;

        // Return the first element that was added.
        return m_array[cFirst];
    }

    // Add an element to the end of the array.
    //
    void AddElement( const TYPE& ElementToAdd )
    {
        TYPE& NewElement = Grow();
        NewElement = ElementToAdd;
    }

    // Shrink the array.
    void Shrink(ULONG cShrink = 1)
    {
        VSASSERT(m_cEntries >= cShrink, "Underflow.");

        for (unsigned i = m_cEntries; i > m_cEntries - cShrink; )
        {

            i--;

            // This will cause the contents of our thinggy to get
            // destroyed.  Neato, eh?
            //

            // 



            m_array[i].~TYPE();
        }

        memset(m_array + m_cEntries - cShrink, 0, sizeof(TYPE) * cShrink);
        m_cEntries -= cShrink;
    }

    // Add an element to the array.
    TYPE &Add(ULONG cAdd = 1)
    {
        return Grow(cAdd);
    }

    // Add element to the array and invoke the constructor properly.
    TYPE &CreateNew(ULONG cAdd = 1)
    {
        TYPE &rFirst = Grow(cAdd);
        for (ULONG i = 0; i < cAdd; ++i)
        {
            new((void*)(&rFirst + i)) TYPE();
        }
        return rFirst;
    }

    // Remove an element from the array, even from the middle.
    void Remove(ULONG iRemove)
    {
        VSASSERT(iRemove < m_cEntries, "Index out of range.");

        // Destroy the element.
        {
            m_array[iRemove].~TYPE();
        }

        // Copy the entries that appear after this one.
        memmove(m_array + iRemove,
            m_array + iRemove + 1,
            (m_cEntries - iRemove - 1) * sizeof(TYPE));

        // Manually remove the last element.
        m_cEntries--;

        memset(m_array + m_cEntries, 0, sizeof(TYPE));
    }

    //. Insert an element into the array, even into the middle.
    TYPE &Insert(ULONG index)
    {
        VSASSERT(index <= m_cEntries, "Index out of range.");
        Grow(); // grow by 1 slot.

        // Move all the entries that appear after this one back one slot.
        // If some regions of the source area and the destination overlap, memmove ensures that the original source bytes in the overlapping region are copied before being overwritten.
        memmove(m_array + index + 1,
            m_array + index,
            (m_cEntries - index - 1) * sizeof(TYPE));

        // Clean the slot
        memset(m_array + index, 0, sizeof(TYPE));

        // Return the element.
        return m_array[index];
    }

    // Copies the array element in fromIndex over the array element in toIndex and then
    // removes the element at fromIndex.
    void CopyOver(ULONG toIndex, ULONG fromIndex)
    {
        VSASSERT(toIndex <= m_cEntries && fromIndex <= m_cEntries, "Index out of range.");

        // Nothing to do if we are copying from and to the same index.
        if (toIndex == fromIndex)
        {
            return;
        }

        memcpy(m_array + toIndex, m_array + fromIndex, sizeof(TYPE));
        Remove(fromIndex);
    }

    // Return the last slot in the array.
    TYPE &End()
    {
        return m_array[m_cEntries - 1];
    }

private:

    ULONG m_cSlots;
    ULONG m_cEntries;

    TYPE *m_array;
};

