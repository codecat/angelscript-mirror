/*
   AngelCode Scripting Library
   Copyright (c) 2003-2007 Andreas Jönsson

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

   Andreas Jönsson
   andreas@angelcode.com
*/

#ifndef AS_OBJECTARRAY_H
#define AS_OBJECTARRAY_H

#if !defined(AS_NO_MEMORY_H)
#include <memory.h>
#endif
#include <string.h> // some compilers declare memcpy() here
#include <assert.h>

BEGIN_AS_NAMESPACE

template <class T> class asCObjectArray
{
public:
	asCObjectArray();
	asCObjectArray(const asCObjectArray<T> &);
	~asCObjectArray();

	void   Allocate(size_t numElements, bool keepData);
	size_t GetCapacity() const;

	void PushLast(const T &element);
	T    PopLast();

	void   SetLength(size_t numElements);
	size_t GetLength() const;

	void Copy(const T*, size_t count);
	asCObjectArray<T> &operator =(const asCObjectArray<T> &);

	const T &operator [](size_t index) const;
	T &operator [](size_t index);
	T *AddressOf();

	void Concatenate(const asCObjectArray<T> &);

	T* Find(const T &element);

	bool operator==(const asCObjectArray<T> &) const;
	bool operator!=(const asCObjectArray<T> &) const;

protected:
	T   *array;
	size_t  length;
	size_t  maxLength;
};

// Implementation

template <class T>
T *asCObjectArray<T>::AddressOf()
{
	return array;
}

template <class T>
asCObjectArray<T>::asCObjectArray(void)
{
	array     = 0;
	length    = 0;
	maxLength = 0;
}

template <class T>
asCObjectArray<T>::asCObjectArray(const asCObjectArray<T> &copy)
{
	array     = 0;
	length    = 0;
	maxLength = 0;

	*this = copy;
}

template <class T>
asCObjectArray<T>::~asCObjectArray(void)
{
	if( array )
	{
		// Call the destructor for all elements
		for( size_t n = 0; n < length; n++ )
			array[n].~T();
		DELETEOBJARRAY(array);
		array = 0;
	}
}

template <class T>
size_t asCObjectArray<T>::GetLength() const
{
	return length;
}

template <class T>
const T &asCObjectArray<T>::operator [](size_t index) const
{
	assert(index < length);

	return array[index];
}

template <class T>
T &asCObjectArray<T>::operator [](size_t index)
{
	assert(index < length);

	return array[index];
}

template <class T>
void asCObjectArray<T>::PushLast(const T &element)
{
	if( length == maxLength )
		Allocate(maxLength + maxLength/2 + 1, true);

	array[length++] = element;
}

template <class T>
T asCObjectArray<T>::PopLast()
{
	assert(length > 0);

	return array[--length];
}

template <class T>
void asCObjectArray<T>::Allocate(size_t numElements, bool keepData)
{
	T *tmp = numElements ? NEWOBJARRAY(T,numElements) : 0;

	if( array )
	{
		size_t oldLength = length;

		if( keepData )
		{
			if( length > numElements )
				length = numElements;

			for( size_t n = 0; n < length; n++ )
				tmp[n] = array[n];
		}
		else
			length = 0;

		// Call the destructor for all elements
		for( size_t n = 0; n < oldLength; n++ )
			array[n].~T();
		DELETEOBJARRAY(array);
	}

	array = tmp;
	maxLength = numElements;
}

template <class T>
size_t asCObjectArray<T>::GetCapacity() const
{
	return maxLength;
}

template <class T>
void asCObjectArray<T>::SetLength(size_t numElements)
{
	if( numElements > maxLength )
		Allocate(numElements, true);

	length = numElements;
}

template <class T>
void asCObjectArray<T>::Copy(const T *data, size_t count)
{
	if( maxLength < count )
		Allocate(count, false);

	for( size_t n = 0; n < count; n++ )
		array[n] = data[n];

	length = count;
}

template <class T>
asCObjectArray<T> &asCObjectArray<T>::operator =(const asCObjectArray<T> &copy)
{
	Copy(copy.array, copy.length);

	return *this;
}

template <class T>
bool asCObjectArray<T>::operator ==(const asCObjectArray<T> &other) const
{
	if( length != other.length ) return false;

	for( asUINT n = 0; n < length; n++ )
		if( array[n] != other.array[n] )
			return false;

	return true;
}

template <class T>
bool asCObjectArray<T>::operator !=(const asCObjectArray<T> &other) const
{
	return !(*this == other);
}

template <class T>
void asCObjectArray<T>::Concatenate(const asCObjectArray<T> &other)
{
	if( maxLength < length + other.length )
		Allocate(length + other.length, true);

	for( size_t n = 0; n < other.length; n++ )
		array[length+n] = other.array[n];

	length += other.length;
}

template <class T>
T *asCObjectArray<T>::Find(const T &e)
{
	for( size_t n = 0; n < length; n++ )
		if( array[n] == e ) return &array[n];

	return 0;
}

END_AS_NAMESPACE

#endif
