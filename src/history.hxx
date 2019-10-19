#ifndef REPLXX_HISTORY_HXX_INCLUDED
#define REPLXX_HISTORY_HXX_INCLUDED 1

#include <vector>

#include "unicodestring.hxx"
#include "conversion.hxx"

namespace replxx {

class History {
public:
	typedef std::vector<UnicodeString> lines_t;
private:
	lines_t _data;
	int _maxSize;
	int _index;
	/*
	 * _previousIndex and _recallMostRecent are used to allow
	 * HISTORY_NEXT action (a down-arrow key) to have a special meaning
	 * if invoked after a line from history was accepted without
	 * any modification.
	 * Special meaning is: a down arrow shall jump to the line one
	 * after previously accepted from history.
	 */
	int _previousIndex;
	bool _recallMostRecent;
	bool _unique;
public:
	History( void );
	void add( UnicodeString const& line );
	void save( std::string const& filename );
	void load( std::string const& filename );
	void clear( void );
	void set_max_size( int len );
	void set_unique( bool unique_ ) {
		_unique = unique_;
	}
	void reset_pos( int = -1 );
	UnicodeString const& operator[] ( int ) const;
	void set_recall_most_recent( void ) {
		_recallMostRecent = true;
	}
	void reset_recall_most_recent( void ) {
		_recallMostRecent = false;
	}
	void drop_last( void ) {
		_data.pop_back();
	}
	void commit_index( void ) {
		_previousIndex = _recallMostRecent ? _index : -2;
	}
	int current_pos( void ) const {
		return ( _index );
	}
	bool is_last( void ) const {
		return ( _index == ( size() - 1 ) );
	}
	bool is_empty( void ) const {
		return ( _data.empty() );
	}
	void update_last( UnicodeString const& line_ ) {
		_data.back() = line_;
	}
	bool move( bool );
	UnicodeString const& current( void ) const {
		return ( _data[_index] );
	}
	void jump( bool );
	bool common_prefix_search( UnicodeString const&, int, bool );
	int size( void ) const {
		return ( static_cast<int>( _data.size() ) );
	}
private:
	History( History const& ) = delete;
	History& operator = ( History const& ) = delete;
};

}

#endif

