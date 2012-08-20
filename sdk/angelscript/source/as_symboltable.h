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
//      Author: Markus Lenger, a.k.a. mlengerx
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

// TODO: cleanup: This should be in its own header. It is only here because it is 
//                needed for the template and cannot be resolved with a forward declaration
struct asSNameSpace
{
	asCString name;

	// TODO: namespace: A namespace should have access masks. The application should be 
	//                  able to restrict specific namespaces from access to specific modules
};




// Interface to avoid nested templates which is not well supported by older compilers, e.g. MSVC6
struct asIFilter
{
	virtual bool operator()(const void*) const = 0;
};




// Filter class that accepts all entries
struct asCAcceptAll : public asIFilter
{
    bool operator()(const void*) const
    {
        return true;
    }
};



/**
 *  Map type used internally for mapping namespace::name to symbols
 *  Currently this is a std::multimap. Later it will be replaced by
 *  a STL independent class.
 */
typedef std::multimap<asCString, unsigned> _asSSymMap;


// forward declaration
template<class T>
class asCSymbolTable;




// Iterator that allows iterating in index order
template<class T, class T2 = T>
class asCSymbolTableIterator
{
public:
    T2* operator*() const;
    T2* operator->() const;
    asCSymbolTableIterator<T, T2>& operator++(int);
    asCSymbolTableIterator<T, T2>& operator--(int);
    operator bool() const;
    int GetIndex() const { return m_idx; }

private:
    friend class asCSymbolTable<T>;
    asCSymbolTableIterator<T, T2>(asCSymbolTable<T> *table);

    void Next();
    void Previous();

    asCSymbolTable<T>* m_table;
    unsigned int       m_idx;
};




// Iterator for iterating over a list of entries with same namespace and name
template<class T, class T2 = T>
class asCSymbolTableRangeIterator
{
public:
    T2* operator*() const;
    T2* operator->() const;
    asCSymbolTableRangeIterator<T, T2>& operator++(int);
    operator bool() const;
    int GetIndex() const;

private:
    friend class asCSymbolTable<T>;
    asCSymbolTableRangeIterator(
            const asCSymbolTable<T>          *table,
            const _asSSymMap::const_iterator &begin,
            const _asSSymMap::const_iterator &end);

    const asCSymbolTable<T>    *m_table;
    _asSSymMap::const_iterator  m_current;
    _asSSymMap::const_iterator  m_end;
};



// Symbol table mapping namespace + name to symbols
template<class T>
class asCSymbolTable
{
public:
    typedef asCSymbolTableIterator<T, T> iterator;
    typedef asCSymbolTableIterator<T, const T> const_iterator;
    typedef asCSymbolTableRangeIterator<T, const T> const_range_iterator;

	asCSymbolTable(unsigned initial_capacity = 0);

	int GetIndex(const asSNameSpace *ns, const asCString &name, const asIFilter &comparator) const;

	int GetFirstIndex(const asSNameSpace *ns, const asCString &name) const;
	int GetIndex(const T*) const;
	int GetLastId() const;
	T* GetFirst(const asSNameSpace *ns, const asCString &name);

	T* Get(const asSNameSpace *ns, const asCString &name, const asIFilter &comparator) const;

	T* Get(unsigned index);
	T* GetLast();
	const T* GetFirst(const asSNameSpace *ns, const asCString &name) const;
    const T* Get(unsigned index) const;
    const T* GetLast() const;

    const_range_iterator GetRange(
            const asSNameSpace *ns, const asCString &name) const;

	int Put(T* entry);
	void Set(unsigned idx, T* p_entry);
	unsigned GetSize() const;
	asCSymbolTable<T>& operator=(const asCSymbolTable<T> &other);

	void SwapWith(asCSymbolTable<T> &other);

	void Clear();
	bool Erase(unsigned idx);
	void Allocate(unsigned elem_cnt, bool keep_data);

	iterator List();
	const_iterator List() const;

private:
    friend class asCSymbolTableIterator<T, T>;
    friend class asCSymbolTableIterator<T, const T>;
	
	void GetKey(const T *entry, asCString &key) const;
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

	_asSSymMap    _map;
	asCArray<T*>  _entries;
};



// Assignment operator
template<class T>
asCSymbolTable<T>& asCSymbolTable<T>::operator=(const asCSymbolTable<T> &other)
{
    _map = other._map;
    _entries = other._entries;
    return *this;
}


template<class T>
void asCSymbolTable<T>::SwapWith(asCSymbolTable<T> &other)
{
	_map.swap(other._map);
	_entries.SwapWith(other._entries);
}


// Constructor
// initial_capacity gives the number of entries to allocate in advance
template<class T>
asCSymbolTable<T>::asCSymbolTable(unsigned initial_capacity) : _entries(initial_capacity)
{
}



template<class T>
int asCSymbolTable<T>::GetIndex(
        const asSNameSpace *ns,
        const asCString &name,
        const asIFilter &filter) const
{
	asCString key;
	BuildKey(ns, name, key);
	symmap_const_range_t range = _map.equal_range(key);
	for( _asSSymMap::const_iterator it = range.first; it != range.second; it++ )
	{
		T *entry = _entries[it->second];
		if( entry && filter(entry) )
			return it->second;
	}
	return -1;
}




template<class T>
T* asCSymbolTable<T>::Get(const asSNameSpace *ns, const asCString &name, const asIFilter &comp) const
{
    int idx = GetIndex(ns, name, comp);
    if (idx != -1) return _entries[idx];
    return 0;
}




template<class T>
int asCSymbolTable<T>::GetFirstIndex(const asSNameSpace *ns, const asCString &name) const
{
    asCString key;
    BuildKey(ns, name, key);
    if (_map.count(key))
    {
        return _map.find(key)->second;
    }
    return -1;
}


// Find the index of a certain symbol
// ATTENTION: this function has linear runtime complexity O(n)!!
template<class T>
int asCSymbolTable<T>::GetIndex(const T* p_entry) const
{
    asCSymbolTableIterator<T, const T> it = List();
    while( it )
	{
        if( *it == p_entry )
            return it.GetIndex();
        it++;
	}
    return -1;
}




template<class T>
T* asCSymbolTable<T>::Get(unsigned idx)
{
    if( !CheckIdx(idx) )
        return 0;

    return _entries[idx];
}


template<class T>
T* asCSymbolTable<T>::GetFirst(const asSNameSpace *ns, const asCString &name)
{
    int idx = GetFirstIndex(ns, name);
    return Get(idx);
}


template<class T>
T* asCSymbolTable<T>::GetLast()
{
    return Get(GetLastId());
}



template<class T>
const T* asCSymbolTable<T>::Get(unsigned idx) const
{
    return const_cast< asCSymbolTable<T>* >(this)->Get(idx);
}



template<class T>
const T* asCSymbolTable<T>::GetFirst(const asSNameSpace *ns, const asCString &name) const
{
    return const_cast< asCSymbolTable<T>* >(this)->GetFirst(ns, name);
}



template<class T>
const T* asCSymbolTable<T>::GetLast() const
{
    return const_cast< asCSymbolTable<T>* >(this)->GetLast();
}




template<class T>
asCSymbolTableRangeIterator<T, const T> asCSymbolTable<T>::GetRange(
            const asSNameSpace *ns,
            const asCString &name) const
{
    asCString key;
    BuildKey(ns, name, key);
    symmap_const_range_t range = _map.equal_range(key);
    return asCSymbolTableRangeIterator<T, const T>(this, range.first, range.second);
}



// Clear the symbol table
// ATTENTION: The contained symbols are not rleased. This is up to the client
template<class T>
void asCSymbolTable<T>::Clear()
{
    _entries.SetLength(0);
    //_map.EraseAll();
    _map.clear();
}




// Pre-allocate slots for elemCnt entries
template<class T>
void asCSymbolTable<T>::Allocate(unsigned elemCnt, bool keepData)
{
    _entries.Allocate(elemCnt, keepData);
    if (!keepData)
        _map.clear();
}



template<class T>
bool asCSymbolTable<T>::Erase(unsigned idx)
{
    if( !CheckIdx(idx) )
    {
        asASSERT(false);
        return false;
    }

    T *p_entry = _entries[idx];
    asASSERT(p_entry);
    if( !p_entry ) 
		return false;

    asCString key;
    GetKey(p_entry, key);
    symmap_range_t range = _map.equal_range(key);
    for( _asSSymMap::iterator it = range.first; it != range.second; it++ )
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




template<class T>
int asCSymbolTable<T>::Put(T *entry)
{
	unsigned idx = _entries.GetLength();
	asCString key;
	GetKey(entry, key);
	_map.insert(std::pair<asCString, unsigned>(key, idx));
	_entries.PushLast(entry);
	return idx;
}



// Sets the entry for a specific id. If the id is associated
// with another id this association will be removed
template<class T>
void asCSymbolTable<T>::Set(unsigned idx, T* p_entry)
{
    if( idx >= _entries.GetLength() )
        _entries.SetLength(idx + 1);
    else
    {
        // remove any existing entry
        if( _entries[idx] )
            Erase(idx);
    }

    asCString key;
    GetKey(p_entry, key);
    _map.insert(std::pair<asCString, unsigned>(key, idx));
    _entries[idx] = p_entry;
}




template<class T>
void asCSymbolTable<T>::BuildKey(const asSNameSpace *ns, const asCString &name, asCString &key) const
{
	// TODO: The key shouldn't be just an asCString. It should keep the 
	//       namespace as a pointer, so it can be compared as pointer.
    key = ns->name + "::" + name;
}


// Return key for specified symbol (namespace and name are used to generate the key)
template<class T>
void asCSymbolTable<T>::GetKey(const T *entry, asCString &key) const
{
	BuildKey(entry->nameSpace, entry->name, key);
}



template<class T>
unsigned asCSymbolTable<T>::GetSize() const
{
    return _map.size();
}




template<class T>
asCSymbolTableIterator<T, T> asCSymbolTable<T>::List()
{
    return asCSymbolTableIterator<T, T>(this);
}




template<class T>
typename asCSymbolTable<T>::const_iterator asCSymbolTable<T>::List() const
{
    return asCSymbolTableIterator<T, const T>(const_cast< asCSymbolTable<T> *>(this));
}




template<class T>
bool asCSymbolTable<T>::CheckIdx(unsigned idx) const
{
    return (idx >= 0 && idx < _entries.GetLength());
}




template<class T>
int asCSymbolTable<T>::GetLastId() const
{
    unsigned idx = _entries.GetLength() - 1;
    if (!CheckIdx(idx))
    {
        return -1;
    }
    return idx;
}



// Iterator
template<class T, class T2>
T2* asCSymbolTableIterator<T, T2>::operator*() const
{
    asASSERT(m_table->CheckIdx(m_idx));
    return m_table->_entries[m_idx];
}

template<class T, class T2>
T2* asCSymbolTableIterator<T, T2>::operator->() const
{
    asASSERT(m_table->CheckIdx(m_idx));
    return m_table->_entries[m_idx];
}

template<class T, class T2>
asCSymbolTableIterator<T, T2>& asCSymbolTableIterator<T, T2>::operator++(int)
{
    Next();
    return *this;
}

template<class T, class T2>
asCSymbolTableIterator<T, T2>::asCSymbolTableIterator(asCSymbolTable<T> *table) : m_table(table), m_idx(0)
{
    unsigned int sz = m_table->_entries.GetLength();
    while( m_idx < sz && m_table->_entries[m_idx] == 0 )
    {
        m_idx++;
    }
}


// Return true if more elements are following
// ATTENTION: When deleting the object currently pointed to by this iterator this
// method returns false even though there might be more elements in the list
template<class T, class T2>
asCSymbolTableIterator<T, T2>::operator bool() const
{
    return m_idx < m_table->_entries.GetLength() && m_table->_entries[m_idx] != 0;
}



template<class T, class T2>
void asCSymbolTableIterator<T, T2>::Next()
{
    unsigned int sz = m_table->_entries.GetLength();
    m_idx++;
    while( m_idx < sz && m_table->_entries[m_idx] == 0 )
        m_idx++;
}



template<class T, class T2>
void asCSymbolTableIterator<T, T2>::Previous()
{
    // overflow on stepping over first element
    unsigned int sz = m_table->_entries.GetLength();
    m_idx--;
    while( m_idx < sz && m_table->_entries[m_idx] == 0 )
        m_idx--;
}



template<class T, class T2>
asCSymbolTableIterator<T, T2>& asCSymbolTableIterator<T, T2>::operator--(int)
{
    Previous();
    return *this;
}



//
// Implementation of asCSymbolTableRangeIterator
//


template <class T, class T2>
T2* asCSymbolTableRangeIterator<T, T2>::operator*() const
{
    return m_table->Get(m_current->second);
}



template <class T, class T2>
T2* asCSymbolTableRangeIterator<T, T2>::operator->() const
{
    return m_current->second;
}


template <class T, class T2>
asCSymbolTableRangeIterator<T, T2>& asCSymbolTableRangeIterator<T, T2>::operator++(int)
{
    m_current++;

    return *this;
}


template <class T, class T2>
asCSymbolTableRangeIterator<T, T2>::operator bool() const
{
    return m_current != m_end;
}



template <class T, class T2>
int asCSymbolTableRangeIterator<T, T2>::GetIndex() const
{
    return m_current->second;
}



template <class T, class T2>
asCSymbolTableRangeIterator<T, T2>::asCSymbolTableRangeIterator(
        const asCSymbolTable<T> *table,
        const _asSSymMap::const_iterator &begin,
        const _asSSymMap::const_iterator &end) :
            m_table(table), m_current(begin), m_end(end)
{
}


END_AS_NAMESPACE

#endif // AS_SYMBOLTABLE_H
