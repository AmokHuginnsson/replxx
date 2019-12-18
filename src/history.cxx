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

Replxx::HistoryScanImpl::HistoryScanImpl( History::entries_t const& entries_, Utf8String& utf8Cache_ )
	: _entries( entries_ )
	, _it( _entries.end() )
	, _utf8Cache( utf8Cache_ )
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
	_utf8Cache.assign( *_it );
	_entryCache = Replxx::HistoryEntry( "", _utf8Cache.get() );
	return ( _entryCache );
}

Replxx::HistoryScan::impl_t History::scan( Utf8String& utf8_ ) const {
	return ( Replxx::HistoryScan::impl_t( new Replxx::HistoryScanImpl( _entries, utf8_ ), delete_ReplxxHistoryScanImpl ) );
}

History::History( void )
	: _entries()
	, _maxSize( REPLXX_DEFAULT_HISTORY_MAX_LEN )
	, _index( 0 )
	, _yankIndex( -1 )
	, _previousIndex( 0 )
	, _recallMostRecent( false )
	, _unique( true ) {
}

void History::add( UnicodeString const& line ) {
	if ( ( _maxSize > 0 ) && ( _entries.empty() || ( line != _entries.back() ) ) ) {
		if ( _unique ) {
			_entries.erase( std::remove( _entries.begin(), _entries.end(), line ), _entries.end() );
		}
		if ( size() > _maxSize ) {
			_entries.erase( _entries.begin() );
			_recallMostRecent = false;
		}
		_entries.push_back( line );
	}
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
	for ( UnicodeString const& h : data ) {
		add( h );
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
	for ( UnicodeString const& h : _entries ) {
		if ( ! h.is_empty() ) {
			utf8.assign( h );
			histFile << utf8.get() << endl;
		}
	}
	return;
}

void History::load( std::string const& filename ) {
	ifstream histFile( filename );
	if ( ! histFile ) {
		return;
	}
	string line;
	while ( getline( histFile, line ).good() ) {
		string::size_type eol( line.find_first_of( "\r\n" ) );
		if ( eol != string::npos ) {
			line.erase( eol );
		}
		if ( ! line.empty() ) {
			add( UnicodeString( line ) );
		}
	}
	return;
}

void History::clear( void ) {
	_entries.clear();
	_index = 0;
	_recallMostRecent = false;
}

void History::set_max_size( int size_ ) {
	if ( size_ >= 0 ) {
		_maxSize = size_;
		int curSize( size() );
		if ( _maxSize < curSize ) {
			_entries.erase( _entries.begin(), _entries.begin() + ( curSize - _maxSize ) );
		}
	}
}

void History::reset_pos( int pos_ ) {
	if ( pos_ == -1 ) {
		_index = size() - 1;
	} else {
		_index = pos_;
	}
}

void History::reset_yank_iterator( void ) {
	_yankIndex = -1;
}

bool History::next_yank_position( void ) {
	bool resetYankSize( false );
	if ( _yankIndex == -1 ) {
		resetYankSize = true;
	} else {
		-- _yankIndex;
	}
	if ( _yankIndex < 0 ) {
		_yankIndex = _entries.size() - 2;
	}
	return ( resetYankSize );
}

bool History::move( bool up_ ) {
	if ( _recallMostRecent && ! up_ ) {
		_index = _previousIndex; // emulate Windows down-arrow
	} else {
		_index += up_ ? -1 : 1;
	}
	_recallMostRecent = false;
	if (_index < 0) {
		_index = 0;
		return ( false );
	} else if ( _index >= size() ) {
		_index = size() - 1;
		return ( false );
	}
	return ( true );
}

void History::jump( bool start_ ) {
	_index = start_ ? 0 : size() - 1;
	_recallMostRecent = false;
}

bool History::common_prefix_search( UnicodeString const& prefix_, int prefixSize_, bool back_ ) {
	int direct( size() + ( back_ ? -1 : 1 ) );
	int i( ( _index + direct ) % _entries.size() );
	while ( i != _index ) {
		if ( _entries[i].starts_with( prefix_.begin(), prefix_.begin() + prefixSize_ ) ) {
			_index = i;
			commit_index();
			return ( true );
		}
		i += direct;
		i %= _entries.size();
	}
	return ( false );
}

UnicodeString const& History::operator[] ( int idx_ ) const {
	return ( _entries[ idx_ ] );
}

}

