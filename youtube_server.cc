/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include "youtube_server.hh"

#include <cstdlib>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

#include "util.hh"

using namespace std;

YouTubeServer::YouTubeServer( const std::string & record_folder, const std::string & videodir ) : ReplayServer( record_folder )
{
    vector<string> files;
    list_files( videodir, files );
    for( auto & file : files ) {
        struct stat fileinfo;
        SystemCall( "stat", stat( file.c_str(), &fileinfo ) );
        videofiles[fileinfo.st_size] = file;
    }
}

const string YouTubeServer::handle_special_request( const HTTPRequest & request )
{
    const string & first_line = request.get_first_line();

    if( !is_youtube_media_request( first_line ) ) {
        return "";
    }

    // Extract clen
    size_t clen_start = first_line.find( "&clen=" );
    if( clen_start == string::npos ) {
        throw Exception( "YouTube", "no clen found" );
    }
    string clen_str = first_line.substr( clen_start + 6,
            first_line.find( '&', clen_start + 6 ) - (clen_start + 6) );
    off_t clen = atoll( clen_str.c_str() );

    if( !videofiles.count(clen) ) {
        cerr << "Warning: video with clen " << clen << " request but not found" << endl;
        return "";
    }

    // Extract range info
    size_t range_start = first_line.find( "&range=" );
    size_t range_mid   = first_line.find( '-', range_start );
    size_t range_end   = first_line.find( '&', range_mid );
    if( range_start == string::npos || range_mid == string::npos ) {
        throw Exception( "YouTube", "no valid range found" );
    }

    off_t chunk_offset = atoll( first_line.substr( range_start + 7, range_mid - (range_start + 7) ).c_str() );
    off_t chunk_last   = atoll( first_line.substr( range_mid   + 1, range_end - (range_mid   + 1) ).c_str() );
    off_t chunk_len = chunk_last - chunk_offset + 1;

    /*
    cout << first_line << endl;
    cout.flush();
    */

    string file_name = videofiles[clen];
    string mime = "video/mp4";
    if( file_name.find( ".aac" ) != string::npos ) {
        mime = "audio/mp4";
    }

    stringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Cache-Control: private, max-age=20930\r\n";
    response << "Connection: keep-alive\r\n";
    response << "Last-Modified: Sat, 10 Aug 2013 01:47:48 GMT\r\n";
    response << "Server: gvs 1.0\r\n";
    response << "Accept-Ranges: bytes\r\n";
    response << "Access-Control-Allow-Credentials: true\r\n";
    response << "Access-Control-Allow-Origin: http://www.youtube.com\r\n";
    response << "Timing-Allow-Origin: http://www.youtube.com\r\n";
    response << "Content-Type: " << mime << "\r\n";
    response << "X-Content-Type-Options: nosniff\r\n";
    response << "Content-Length: " << chunk_len << "\r\n";
    response << "\r\n";

    ifstream file{ file_name, ifstream::in | ifstream::binary };
    file.seekg( chunk_offset, ifstream::beg );

    char * buf = new char[chunk_len];
    file.read( buf, chunk_len );
    response.write( buf, chunk_len );
    delete[] buf;

    return response.str();
}
