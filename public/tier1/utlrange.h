//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Strongly-typed index ranges.
//
//			TF2's shared code includes this tier1 header, but it is not part of
//			the public Source SDK drop, so it is reconstructed here from the way
//			the TF2 sources use it (see CTFLobbyShared::MatchPlayers_t).
//
//			The point of the type is to stop indices belonging to different
//			collections from being mixed up: the range is parameterised on the
//			deriving class, so CUtlIndexRange< int, A >::index and
//			CUtlIndexRange< int, B >::index are unrelated types even though both
//			wrap an int.
//
//=============================================================================//

#ifndef UTLRANGE_H
#define UTLRANGE_H

#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------
// Purpose: A half-open range [begin, end) of strongly-typed indices.
//
//			Derive with the deriving class as the second parameter:
//				class MyRange : public CUtlIndexRange< int, MyRange >
//
//			Iterate with range-based for, which yields index objects:
//				for ( auto idx : MyRange() ) { ... idx.value ... }
//-----------------------------------------------------------------------------
template < typename T, typename TDerived >
class CUtlIndexRange
{
public:
	typedef T ValueType_t;

	// The index doubles as the iterator: dereferencing yields the index itself,
	// so a range-based for loop hands out indices rather than raw values.
	// Deliberately an aggregate so that index{ n } stays valid.
	struct index
	{
		T value;

		index &operator++()					{ ++value; return *this; }
		index operator++( int )				{ index prev = *this; ++value; return prev; }
		const index &operator*() const		{ return *this; }

		bool operator==( const index &rhs ) const	{ return value == rhs.value; }
		bool operator!=( const index &rhs ) const	{ return value != rhs.value; }
		bool operator<( const index &rhs ) const	{ return value < rhs.value; }
		bool operator<=( const index &rhs ) const	{ return value <= rhs.value; }
		bool operator>( const index &rhs ) const	{ return value > rhs.value; }
		bool operator>=( const index &rhs ) const	{ return value >= rhs.value; }
	};

	CUtlIndexRange( T beginIndex, T endIndex )
		: m_beginIndex( beginIndex ), m_endIndex( endIndex ) {}

	index begin() const		{ index i = { m_beginIndex }; return i; }
	index end() const		{ index i = { m_endIndex }; return i; }

	// An index that was never found compares equal to end(), so validity is
	// simply "inside the half-open range".
	bool BValidIdx( index idx ) const
	{
		return idx.value >= m_beginIndex && idx.value < m_endIndex;
	}

private:
	T m_beginIndex;
	T m_endIndex;
};

#endif // UTLRANGE_H
