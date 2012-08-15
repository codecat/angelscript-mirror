/*
   AngelCode Scripting Library
   Copyright (c) 2012 Andreas Jonsson

   This software is provided 'as-is', without any express or implied 
   warranty. In no event will the authors be held liable for any 
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any 
   purpose, including commercial applications, and to alter it and 
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you 
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product 
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and 
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source 
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jonsson
   andreas@angelcode.com
*/


//
// as_symboltable.h
//
//  Created on: Jun 19, 2012
//      Author: mlengerx
//
// This class is used for fast symbol lookups while parsing or loading bytecode
//

#ifndef AS_SYMBOLTABLE_H
#define AS_SYMBOLTABLE_H

// due to cygwin bug
#undef min
#undef max

#include <map>

#include "as_config.h"
#include "as_memory.h"
#include "as_string.h"
#include "as_map.h"
#include "as_datatype.h"


BEGIN_AS_NAMESPACE

// Forward declaration
struct asSNameSpace;

// Interface to avoid nested templates which is not well supported by older compilers, e.g. MSVC6
struct asIFilter
{
	virtual bool operator()(const void*) const = 0;
};

/**
 *  Map type used internally for mapping namespace::name to symbols
 *  Currently this is a std::multimap. Later it will be replaced by
 *  a STL independent class.
 */
typedef std::multimap<asCString, unsigned> _asSSymMap;

// forward declaration
template<class ENTRY_TYPE>
class asCSymbolTable;

/**
 *  Iterator that allows iterating in index order
 */
template<class ENTRY_TYPE, class VALUE_TYPE = ENTRY_TYPE>
class asCSymbolTableIterator
{
public:
    VALUE_TYPE* operator*() const;
    VALUE_TYPE* operator->() const;
    asCSymbolTableIterator<ENTRY_TYPE, VALUE_TYPE>& operator++(int);
    asCSymbolTableIterator<ENTRY_TYPE, VALUE_TYPE>& operator--(int);
    operator bool() const;
    int GetIndex() const { return _idx; }

private:
    friend class asCSymbolTable<ENTRY_TYPE>;
    asCSymbolTableIterator<ENTRY_TYPE, VALUE_TYPE>(asCSymbolTable<ENTRY_TYPE> *p_table);
    void Next();
    void Previous();
    asCSymbolTable<ENTRY_TYPE>* _p_table;
    unsigned _idx;
};



/**
 *  Filter class that accepts all entries
 */
template <class ENTRY_TYPE>
struct asCAcceptAll
{
    bool operator()(const ENTRY_TYPE*)
    {
        return true;
    }
};

/**
 * Iterator for iterating over a list of entries with same namespace and name
 * matching certain criteria defined by a filter-object
 */
template<class ENTRY_TYPE, class VALUE_TYPE = ENTRY_TYPE, class FILTER= asCAcceptAll<ENTRY_TYPE> >
class asCSymbolTableRangeIterator
{
public:

    VALUE_TYPE* operator*() const;
    VALUE_TYPE* operator->() const;
    asCSymbolTableRangeIterator<ENTRY_TYPE, VALUE_TYPE, FILTER>& operator++(int);
    operator bool() const;
    int GetIndex() const;

private:
    friend class asCSymbolTable<ENTRY_TYPE>;
    asCSymbolTableRangeIterator(
            const asCSymbolTable<ENTRY_TYPE> * p_table,
            const _asSSymMap::const_iterator &begin,
            const _asSSymMap::const_iterator &end,
            const FILTER &comp);

    const asCSymbolTable<ENTRY_TYPE> *mp_table;
    _asSSymMap::const_iterator m_current;
    _asSSymMap::const_iterator m_end;
    FILTER m_comp;
};

/**
 * Symbol table mapping namespace + name to symbols
 */
template<class ENTRY_TYPE>
class asCSymbolTable
{
public:
    typedef asCSymbolTableIterator<ENTRY_TYPE, ENTRY_TYPE> iterator;
    typedef asCSymbolTableIterator<ENTRY_TYPE, const ENTRY_TYPE> const_iterator;
    typedef asCSymbolTableRangeIterator<ENTRY_TYPE, const ENTRY_TYPE> const_range_iterator;
    friend class asCSymbolTableIterator<ENTRY_TYPE, ENTRY_TYPE>;
    friend class asCSymbolTableIterator<ENTRY_TYPE, const ENTRY_TYPE>;

	asCSymbolTable(unsigned initial_capacity = 0);

	int GetIndex(const asSNameSpace *ns, const asCString &name, const asIFilter &comparator) const;

	int GetFirstIndex(const asSNameSpace *ns, const asCString &name) const;
	int GetIndex(const ENTRY_TYPE*) const;
	int GetLastId() const;
	ENTRY_TYPE* GetFirst(const asSNameSpace *ns, const asCString &name);

	ENTRY_TYPE* Get(const asSNameSpace *ns, const asCString &name, const asIFilter &comparator) const;

	ENTRY_TYPE* Get(unsigned index);
	ENTRY_TYPE* GetLast();
	const ENTRY_TYPE* GetFirst(const asSNameSpace *ns, const asCString &name) const;
    const ENTRY_TYPE* Get(unsigned index) const;
    const ENTRY_TYPE* GetLast() const;

    const_range_iterator GetRange(
            const asSNameSpace *ns, const asCString &name) const;

	int Put(ENTRY_TYPE* entry);
	void Set(unsigned idx, ENTRY_TYPE* p_entry);
	unsigned GetSize() const;
	asCSymbolTable<ENTRY_TYPE>& operator=(const asCSymbolTable<ENTRY_TYPE> &other);
	/*
	ENTRY_TYPE* operator[](unsigned idx);
	const ENTRY_TYPE* operator[](unsigned idx) const;
	*/
	void Clear();
	bool Erase(unsigned idx);
	void Allocate(unsigned elem_cnt, bool keep_data);

	iterator List();
	const_iterator List() const;

private:

	void GetKey(const ENTRY_TYPE *entry, asCString &key) const;
	void BuildKey(const asSNameSpace *ns, const asCString &name, asCString &key) const;
	bool CheckIdx(unsigned idx) const;

	/*
	 * severe problems with asCMap (Segmentation Fault).
	 * guess this is due to the missing deep-copy implementation.
	 * before implementing a deep-copy one should check if a simple
	 * move (moving the RB-Tree from one asCMap-object to the other
	 * while invaldiating the first one) would be sufficient.
	 *
	 * UPDATE: Multimap needed due to entries that only differ in
	 * type.
	 */

	//asCMap<asCString, unsigned> _map;
	typedef std::pair<_asSSymMap::iterator, _asSSymMap::iterator> symmap_range_t;
	typedef std::pair<_asSSymMap::const_iterator, _asSSymMap::const_iterator> symmap_const_range_t;

	_asSSymMap             _map;
	asCArray<ENTRY_TYPE*>  _entries;
};


/**
 * Assignment operator
 */
template<class ENTRY_TYPE>
asCSymbolTable<ENTRY_TYPE>& asCSymbolTable<ENTRY_TYPE>::operator=(const asCSymbolTable<ENTRY_TYPE> &other)
{
    _map = other._map;
    _entries = other._entries;
    return *this;
}

/**
 * Constructor
 * @param initial_capacity Number of entries to allocate in advance
 */
template<class ENTRY_TYPE>
asCSymbolTable<ENTRY_TYPE>::asCSymbolTable(unsigned initial_capacity) : _entries(initial_capacity)
{
}


/**
 * Find the index of the first symbol that matches all criteria
 *
 * @param ns symbols namespace
 * @param symbols name
 * @param filter Filter to apply to objects
 *
 * @return -1 or index of first fitting symbol
 */
template<class ENTRY_TYPE>
int asCSymbolTable<ENTRY_TYPE>::GetIndex(
        const asSNameSpace *ns,
        const asCString &name,
        const asIFilter &filter) const
{
	asCString key;
	BuildKey(ns, name, key);
	symmap_const_range_t range = _map.equal_range(key);
	for( _asSSymMap::const_iterator it = range.first; it != range.second; it++ )
	{
		ENTRY_TYPE *entry = _entries[it->second];
		if( entry && filter(entry) )
			return it->second;
	}
	return -1;
}


/**
 *
 *
 * @return symbol or 0
 */
template<class ENTRY_TYPE>
ENTRY_TYPE* asCSymbolTable<ENTRY_TYPE>::Get(const asSNameSpace *ns, const asCString &name, const asIFilter &comp) const
{
    int idx = GetIndex(ns, name, comp);
    if (idx != -1) return _entries[idx];
    return 0;
}



/**
 * Find the first symbol that matches all criteria
 *
 * @param ns symbols namespace
 * @param symbols name
 * @return -1 or index of first fitting symbol
 */
template<class ENTRY_TYPE>
int asCSymbolTable<ENTRY_TYPE>::GetFirstIndex(const asSNameSpace *ns, const asCString &name) const
{
    asCString key;
    BuildKey(ns, name, key);
    if (_map.count(key))
    {
        return _map.find(key)->second;
    }
    return -1;
}


/**
 * Find the index of a certain symbol
 * ATTENTION: this function has linear runtime complexity O(n)!!
 *
 * @param p_entry Pointer to symbol
 * @return Index of specified pointer or -1 if not found
 */
template<class ENTRY_TYPE>
int asCSymbolTable<ENTRY_TYPE>::GetIndex(const ENTRY_TYPE* p_entry) const
{
    asCSymbolTableIterator<ENTRY_TYPE, const ENTRY_TYPE> it = List();
    while (it)
    {
        if (*it == p_entry)
        {
            return it.GetIndex();
        }
        it++;
    }
    return -1;
}

/**
 * Get symbol with specified id
 * @return Pointer to symbol or 0
 */
template<class ENTRY_TYPE>
ENTRY_TYPE* asCSymbolTable<ENTRY_TYPE>::Get(unsigned idx)
{
    if (!CheckIdx(idx))
    {
        return 0;
    }
    return _entries[idx];
}

/**
 * Get first symbol that matches namespace + name
 * @return symbol or 0
 */
template<class ENTRY_TYPE>
ENTRY_TYPE* asCSymbolTable<ENTRY_TYPE>::GetFirst(const asSNameSpace *ns, const asCString &name)
{
    int idx = GetFirstIndex(ns, name);
    return Get(idx);
}

/**
 * Get the Last symbol in the list
 * @return Pointer to last added symbol or 0 if there is none or the symbol was removed
 */
template<class ENTRY_TYPE>
ENTRY_TYPE* asCSymbolTable<ENTRY_TYPE>::GetLast()
{
    return Get(GetLastId());
}

/**
 * @return Pointer to symbol at position idx
 */
template<class ENTRY_TYPE>
const ENTRY_TYPE* asCSymbolTable<ENTRY_TYPE>::Get(unsigned idx) const
{
    return const_cast< asCSymbolTable<ENTRY_TYPE>* >(this)->Get(idx);
}


/**
 * Get first symbol that matches namespace + name
 * @return symbol or 0
 */
template<class ENTRY_TYPE>
const ENTRY_TYPE* asCSymbolTable<ENTRY_TYPE>::GetFirst(const asSNameSpace *ns, const asCString &name) const
{
    return const_cast< asCSymbolTable<ENTRY_TYPE>* >(this)->GetFirst(ns, name);
}

/**
 * Get the Last symbol in the list
 * @return Pointer to last added symbol or 0 if there is none or the symbol was removed
 */
template<class ENTRY_TYPE>
const ENTRY_TYPE* asCSymbolTable<ENTRY_TYPE>::GetLast() const
{
    return const_cast< asCSymbolTable<ENTRY_TYPE>* >(this)->GetLast();
}

/**
 * Get range iterator over symbols that match the name
 */
template<class ENTRY_TYPE>
asCSymbolTableRangeIterator<ENTRY_TYPE, const ENTRY_TYPE> asCSymbolTable<ENTRY_TYPE>::GetRange(
            const asSNameSpace *ns,
            const asCString &name
) const
{
    asCString key;
    BuildKey(ns, name, key);
    symmap_const_range_t range = _map.equal_range(key);
    return asCSymbolTableRangeIterator<ENTRY_TYPE, const ENTRY_TYPE>(this, range.first, range.second, asCAcceptAll<ENTRY_TYPE>());
}

/*
template<class ENTRY_TYPE>
ENTRY_TYPE* asCSymbolTable<ENTRY_TYPE>::operator[](unsigned idx)
{
    return _entries[idx];
}

template<class ENTRY_TYPE>
const ENTRY_TYPE* asCSymbolTable<ENTRY_TYPE>::operator[](unsigned idx) const
{
    return _entries[idx];
}
*/

/**
 * Clear the symbol table
 * ATTENTION: The contained symbols are not rleased. This is up to the client
 */
template<class ENTRY_TYPE>
void asCSymbolTable<ENTRY_TYPE>::Clear()
{
    _entries.SetLength(0);
    //_map.EraseAll();
    _map.clear();
}

/**
 * Pre-allocate slots for elemCnt entries
 * @param elemCnt Number of elements
 * @param keepData If the existing entries shall be removed
 */
template<class ENTRY_TYPE>
void asCSymbolTable<ENTRY_TYPE>::Allocate(unsigned elemCnt, bool keepData)
{
    _entries.Allocate(elemCnt, keepData);
    if (!keepData)
    {
        //_map.EraseAll();
        _map.clear();
    }
}

/**
 * Remove a symbol from the list
 */
template<class ENTRY_TYPE>
bool asCSymbolTable<ENTRY_TYPE>::Erase(unsigned idx)
{
    if (!CheckIdx(idx))
    {
        asASSERT(false);
        return false;
    }
    ENTRY_TYPE * p_entry = _entries[idx];
    asASSERT(p_entry);
    if (!p_entry) return false;
    asCString key;
    GetKey(p_entry, key);
    symmap_range_t range = _map.equal_range(key);
    for (_asSSymMap::iterator it = range.first; it != range.second; it++)
    {
        if (it->second == idx)
        {
            _entries[idx] = 0;
            _map.erase(it);
            return true;
        }
    }
    asASSERT(false);
    return false;
}

/**
 * Put a symobl in the symbol table
 * @return Symbols id or -1 if not successful
 */
template<class ENTRY_TYPE>
int asCSymbolTable<ENTRY_TYPE>::Put(ENTRY_TYPE *entry)
{
	unsigned idx = _entries.GetLength();
	asCString key;
	GetKey(entry, key);
	_map.insert(std::pair<asCString, unsigned>(key, idx));
	_entries.PushLast(entry);
	return idx;
}

/**
 * Sets the entry for a specific id. If the id is associated with another id
 * this association will be removed
 */
template<class ENTRY_TYPE>
void asCSymbolTable<ENTRY_TYPE>::Set(unsigned idx, ENTRY_TYPE* p_entry)
{
    if (idx >= _entries.GetLength())
    {
        _entries.SetLength(idx + 1);
    }
    else
    {
        // remove any existing entry
        if (_entries[idx])
        {
            Erase(idx);
        }
    }

    asCString key;
    GetKey(p_entry, key);
    _map.insert(std::pair<asCString, unsigned>(key, idx));
    _entries[idx] = p_entry;
}


/**
 * Build a key for the lookup map
 */
template<class ENTRY_TYPE>
void asCSymbolTable<ENTRY_TYPE>::BuildKey(const asSNameSpace *ns, const asCString &name, asCString &key) const
{
	// TODO: The key shouldn't be just an asCString. It should keep the 
	//       namespace as a pointer, so it can be compared as pointer.
    key = ns->name + "::" + name;
}

/**
 * @return key for specified symbol (namespace and name are used to generate the key)
 */
template<class ENTRY_TYPE>
void asCSymbolTable<ENTRY_TYPE>::GetKey(const ENTRY_TYPE *entry, asCString &key) const
{
	// TODO: Reduce memory usage by sharing the string objects
	BuildKey(entry->nameSpace, entry->name, key);
}

/**
 * @return Number of symbols
 */
template<class ENTRY_TYPE>
unsigned asCSymbolTable<ENTRY_TYPE>::GetSize() const
{
    return _map.size();
}

/**
 * @return An iterator that allows iterating over the symbols in order of their id
 */
template<class ENTRY_TYPE>
asCSymbolTableIterator<ENTRY_TYPE, ENTRY_TYPE> asCSymbolTable<ENTRY_TYPE>::List()
{
    return asCSymbolTableIterator<ENTRY_TYPE, ENTRY_TYPE>(this);
}

/**
 * @return An iterator that allows iterating over the symbols in order of their id
 */
template<class ENTRY_TYPE>
typename asCSymbolTable<ENTRY_TYPE>::const_iterator asCSymbolTable<ENTRY_TYPE>::List() const
{
    return asCSymbolTableIterator<ENTRY_TYPE, const ENTRY_TYPE>(const_cast< asCSymbolTable<ENTRY_TYPE> *>(this));
}

/**
 * Check if an index is valid
 */
template<class ENTRY_TYPE>
bool asCSymbolTable<ENTRY_TYPE>::CheckIdx(unsigned idx) const
{
    return (idx >= 0 && idx < _entries.GetLength());
}

/**
 * @return Id of element last added of -1
 *
 */
template<class ENTRY_TYPE>
int asCSymbolTable<ENTRY_TYPE>::GetLastId() const
{
    unsigned idx = _entries.GetLength() - 1;
    if (!CheckIdx(idx))
    {
        return -1;
    }
    return idx;
}

// Iterator
template<class ENTRY_TYPE, class VALUE_TYPE>
VALUE_TYPE* asCSymbolTableIterator<ENTRY_TYPE, VALUE_TYPE>::operator*() const
{
    asASSERT(_p_table->CheckIdx(_idx));
    return _p_table->_entries[_idx];
}

template<class ENTRY_TYPE, class VALUE_TYPE>
VALUE_TYPE* asCSymbolTableIterator<ENTRY_TYPE, VALUE_TYPE>::operator->() const
{
    asASSERT(_p_table->CheckIdx(_idx));
    return _p_table->_entries[_idx];
}

template<class ENTRY_TYPE, class VALUE_TYPE>
asCSymbolTableIterator<ENTRY_TYPE, VALUE_TYPE>& asCSymbolTableIterator<ENTRY_TYPE, VALUE_TYPE>::operator++(int)
{
    Next();
    return *this;
}

/*template<class ENTRY_TYPE, class VALUE_TYPE>
void asCSymbolTableIterator<ENTRY_TYPE, VALUE_TYPE>::operator+(unsigned cnt)
{
    for (unsigned i = 0; i < cnt; i++)
    {
        Next();
    }
}*/

template<class ENTRY_TYPE, class VALUE_TYPE>
asCSymbolTableIterator<ENTRY_TYPE, VALUE_TYPE>::asCSymbolTableIterator(asCSymbolTable<ENTRY_TYPE> *p_table) : _p_table(p_table), _idx(0)
{
    unsigned sz = _p_table->_entries.GetLength();
    while (_idx < sz && _p_table->_entries[_idx] == 0)
    {
        _idx++;
    }
}


/**
 * @return if more elements are following
 *
 * Beware: When deleting the object currently pointed to by this iterator this
 * method returns false even though there might be more elements in the list
 */
template<class ENTRY_TYPE, class VALUE_TYPE>
asCSymbolTableIterator<ENTRY_TYPE, VALUE_TYPE>::operator bool() const
{
    return _idx < _p_table->_entries.GetLength() && _p_table->_entries[_idx] != 0;
}

template<class ENTRY_TYPE, class VALUE_TYPE>
void asCSymbolTableIterator<ENTRY_TYPE, VALUE_TYPE>::Next()
{
    unsigned sz = _p_table->_entries.GetLength();
    _idx++;
    while (_idx < sz && _p_table->_entries[_idx] == 0)
    {
        _idx++;
    }
}

template<class ENTRY_TYPE, class VALUE_TYPE>
void asCSymbolTableIterator<ENTRY_TYPE, VALUE_TYPE>::Previous()
{
    // overflow on stepping over first element
    unsigned sz = _p_table->_entries.GetLength();
    _idx--;
    while (_idx < sz && _p_table->_entries[_idx] == 0)
    {
        _idx--;
    }
}

template<class ENTRY_TYPE, class VALUE_TYPE>
asCSymbolTableIterator<ENTRY_TYPE, VALUE_TYPE>& asCSymbolTableIterator<ENTRY_TYPE, VALUE_TYPE>::operator--(int)
{
    Previous();
    return *this;
}

//
// Implementation of asCSymbolTableRangeIterator
//

/**
 * Iterator for iterating over a list of entries with same namespace and name matching
 *  certain criterias defined a FILTER-object
 */

template <class ENTRY_TYPE, class VALUE_TYPE, class FILTER>
VALUE_TYPE* asCSymbolTableRangeIterator<ENTRY_TYPE, VALUE_TYPE, FILTER>::operator*() const
{
    return mp_table->Get(m_current->second);
}

template <class ENTRY_TYPE, class VALUE_TYPE, class FILTER>
VALUE_TYPE* asCSymbolTableRangeIterator<ENTRY_TYPE, VALUE_TYPE, FILTER>::operator->() const
{
    return m_current->second;
}


template <class ENTRY_TYPE, class VALUE_TYPE, class FILTER>
asCSymbolTableRangeIterator<ENTRY_TYPE, VALUE_TYPE, FILTER>& asCSymbolTableRangeIterator<ENTRY_TYPE, VALUE_TYPE, FILTER>::operator++(int)
{
    m_current++;
    while (m_current != m_end && !m_comp(mp_table->Get(m_current->second)))
    {
        m_current++;
    }
    return *this;
}


template <class ENTRY_TYPE, class VALUE_TYPE, class FILTER>
asCSymbolTableRangeIterator<ENTRY_TYPE, VALUE_TYPE, FILTER>::operator bool() const
{
    return m_current != m_end;
}

template <class ENTRY_TYPE, class VALUE_TYPE, class FILTER>
int asCSymbolTableRangeIterator<ENTRY_TYPE, VALUE_TYPE, FILTER>::GetIndex() const
{
    return m_current->second;
}

template <class ENTRY_TYPE, class VALUE_TYPE, class FILTER>
asCSymbolTableRangeIterator<ENTRY_TYPE, VALUE_TYPE, FILTER>::asCSymbolTableRangeIterator(
        const asCSymbolTable<ENTRY_TYPE> *p_table,
        const _asSSymMap::const_iterator &begin,
        const _asSSymMap::const_iterator &end, const FILTER &comp) :
            mp_table(p_table), m_current(begin), m_end(end), m_comp(comp)
{
    while (m_current != m_end && !m_comp(mp_table->Get(m_current->second)))
    {
        m_current++;
    }
}


END_AS_NAMESPACE

#endif // AS_SYMBOLTABLE_H
