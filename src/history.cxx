#include <algorithm>
#include <memory>
#include <fstream>
#include <cstring>

#ifndef _WIN32

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#endif /* _WIN32 */

#include "replxx.hxx"
#include "history.hxx"

using namespace std;

namespace replxx {

namespace {
void delete_ReplxxHistoryScanImpl( Replxx::HistoryScanImpl* impl_ ) {
	delete impl_;
}
}

static int const REPLXX_DEFAULT_HISTORY_MAX_LEN( 1000 );

Replxx::HistoryScan::HistoryScan( impl_t impl_ )
	: _impl( std::move( impl_ ) ) {
}

bool Replxx::HistoryScan::next( void ) {
	return ( _impl->next() );
}

Replxx::HistoryScanImpl::HistoryScanImpl( History::entries_t const& entries_ )
	: _entries( entries_ )
	, _it( _entries.end() )
	, _utf8Cache()
	, _entryCache( std::string(), std::string() ) {
}

Replxx::HistoryEntry const& Replxx::HistoryScan::get( void ) const {
	return ( _impl->get() );
}

bool Replxx::HistoryScanImpl::next( void ) {
	if ( _it == _entries.end() ) {
		_it = _entries.begin();
	} else {
		++ _it;
	}
	return ( _it != _entries.end() );
}

Replxx::HistoryEntry const& Replxx::HistoryScanImpl::get( void ) const {
	_utf8Cache.assign( _it->text() );
	_entryCache = Replxx::HistoryEntry( _it->timestamp(), _utf8Cache.get() );
	return ( _entryCache );
}

Replxx::HistoryScan::impl_t History::scan( void ) const {
	return ( Replxx::HistoryScan::impl_t( new Replxx::HistoryScanImpl( _entries ), delete_ReplxxHistoryScanImpl ) );
}

History::History( void )
	: _entries()
	, _maxSize( REPLXX_DEFAULT_HISTORY_MAX_LEN )
	, _current( _entries.begin() )
	, _yankPos( _entries.end() )
	, _previous( _entries.begin() )
	, _recallMostRecent( false )
	, _unique( true ) {
}

void History::add( UnicodeString const& line, std::string const& when ) {
	if ( ( _maxSize <= 0 ) || ( ! _entries.empty() && ( line == _entries.back().text() ) ) ) {
		return;
	}
	if ( _unique ) {
		entries_t::iterator it(
			std::find_if(
				_entries.begin(),
				_entries.end(),
				[&line]( Entry const& entry_ ) {
					return ( entry_.text() == line );
				}
			)
		);
		if ( it != _entries.end() ) {
			erase( it );
		}
	}
	if ( size() > _maxSize ) {
		erase( _entries.begin() );
		_recallMostRecent = false;
	}
	_entries.emplace_back( when, line );
	if ( _current == _entries.end() ) {
		_current = moved( _entries.end(), -1 );
	}
	_yankPos = _entries.end();
}

class FileLock {
	std::string _path;
	int _lockFd;
public:
	FileLock( std::string const& name_ )
		: _path( name_ + ".lock" )
		, _lockFd( ::open( _path.c_str(), O_CREAT | O_RDWR, 0600 ) ) {
		static_cast<void>( ::lockf( _lockFd, F_LOCK, 0 ) == 0 );
	}
	~FileLock( void ) {
		static_cast<void>( ::lockf( _lockFd, F_ULOCK, 0 ) == 0 );
		::close( _lockFd );
		::unlink( _path.c_str() );
		return;
	}
};

void History::save( std::string const& filename ) {
#ifndef _WIN32
	mode_t old_umask = umask( S_IXUSR | S_IRWXG | S_IRWXO );
	FileLock fileLock( filename );
#endif
	entries_t data( std::move( _entries ) );
	load( filename );
	for ( Entry const& h : data ) {
		add( h.text(), h.timestamp() );
	}
	ofstream histFile( filename );
	if ( ! histFile ) {
		return;
	}
#ifndef _WIN32
	umask( old_umask );
	chmod( filename.c_str(), S_IRUSR | S_IWUSR );
#endif
	Utf8String utf8;
	for ( Entry const& h : _entries ) {
		if ( ! h.text().is_empty() ) {
			utf8.assign( h.text() );
			histFile << "### " << h.timestamp() << "\n" << utf8.get() << endl;
		}
	}
	return;
}

namespace {

bool is_timestamp( std::string const& s ) {
	static char const TIMESTAMP_PATTERN[] = "### dddd-dd-dd dd:dd:dd.ddd";
	static int const TIMESTAMP_LENGTH( sizeof ( TIMESTAMP_PATTERN ) - 1 );
	if ( s.length() != TIMESTAMP_LENGTH ) {
		return ( false );
	}
	for ( int i( 0 ); i < TIMESTAMP_LENGTH; ++ i ) {
		if ( TIMESTAMP_PATTERN[i] == 'd' ) {
			if ( ! isdigit( s[i] ) ) {
				return ( false );
			}
		} else if ( s[i] != TIMESTAMP_PATTERN[i] ) {
			return ( false );
		}
	}
	return ( true );
}

}


void History::load( std::string const& filename ) {
	ifstream histFile( filename );
	if ( ! histFile ) {
		return;
	}
	string line;
	string when( "0000-00-00 00:00:00.000" );
	while ( getline( histFile, line ).good() ) {
		string::size_type eol( line.find_first_of( "\r\n" ) );
		if ( eol != string::npos ) {
			line.erase( eol );
		}
		if ( is_timestamp( line ) ) {
			when.assign( line, 4 );
			continue;
		}
		if ( ! line.empty() ) {
			add( UnicodeString( line ), when );
		}
	}
	return;
}

void History::clear( void ) {
	_entries.clear();
	_current = _entries.begin();
	_recallMostRecent = false;
}

void History::set_max_size( int size_ ) {
	if ( size_ >= 0 ) {
		_maxSize = size_;
		while ( size() > _maxSize ) {
			erase( _entries.begin() );
		}
	}
}

void History::reset_yank_iterator( void ) {
	_yankPos = _entries.end();
}

bool History::next_yank_position( void ) {
	bool resetYankSize( false );
	if ( _yankPos == _entries.end() ) {
		resetYankSize = true;
	}
	if ( ( _yankPos != _entries.begin() ) && ( _yankPos != _entries.end() ) ) {
		-- _yankPos;
	} else {
		_yankPos = moved( _entries.end(), -2 );
	}
	return ( resetYankSize );
}

bool History::move( bool up_ ) {
	bool doRecall( _recallMostRecent && ! up_ );
	if ( doRecall ) {
		_current = _previous; // emulate Windows down-arrow
	}
	_recallMostRecent = false;
	return ( doRecall || move( _current, up_ ? -1 : 1 ) );
}

void History::jump( bool start_, bool reset_ ) {
	if ( start_ ) {
		_current = _entries.begin();
	} else {
		_current = moved( _entries.end(), -1 );
	}
	if ( reset_ ) {
		_recallMostRecent = false;
	}
}

void History::save_pos( void ) {
	_previous = _current;
}

void History::restore_pos( void ) {
	_current = _previous;
}

bool History::common_prefix_search( UnicodeString const& prefix_, int prefixSize_, bool back_ ) {
	int step( back_ ? -1 : 1 );
	entries_t::const_iterator it( moved( _current, step, true ) );
	while ( it != _current ) {
		if ( it->text().starts_with( prefix_.begin(), prefix_.begin() + prefixSize_ ) ) {
			_current = it;
			commit_index();
			return ( true );
		}
		move( it, step, true );
	}
	return ( false );
}

bool History::move( entries_t::const_iterator& it_, int by_, bool wrapped_ ) const {
	if ( by_ > 0 ) {
		for ( int i( 0 ); i < by_; ++ i ) {
			++ it_;
			if ( it_ != _entries.end() ) {
				break;
			} else if ( wrapped_ ) {
				it_ = _entries.begin();
			} else {
				-- it_;
				return ( false );
			}
		}
	} else {
		for ( int i( 0 ); i > by_; -- i ) {
			if ( it_ != _entries.begin() ) {
				-- it_;
			} else if ( wrapped_ ) {
				it_ = moved( _entries.end(), -1 );
			} else {
				return ( false );
			}
		}
	}
	return ( true );
}

History::entries_t::const_iterator History::moved( entries_t::const_iterator it_, int by_, bool wrapped_ ) const {
	move( it_, by_, wrapped_ );
	return ( it_ );
}

void History::erase( entries_t::iterator it_ ) {
	bool invalidated( it_ == _current );
	it_ = _entries.erase( it_ );
	if ( invalidated ) {
		_current = it_;
	}
	if ( ( _current == _entries.end() ) && ! _entries.empty() ) {
		-- _current;
	}
	_yankPos = _entries.end();
	_previous = _current;
}

}

