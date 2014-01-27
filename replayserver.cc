/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <memory>
#include <csignal>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>

#include "util.hh"
#include "system_runner.hh"
#include "http_record.pb.h"
#include "http_header.hh"
#include "exception.hh"
#include "http_message.hh"

#include "replayserver.hh"

using namespace std;

bool ReplayServer::check_headers( const HTTPRequest & request, const string & stored_header, HTTP_Record::http_message & saved_req ) const
{
    if ( not request.has_header( stored_header ) ) {
        return true;
    }

    auto req_value = request.get_header_value( stored_header );
    for ( int i = 0; i < saved_req.headers_size(); i++ ) {
        HTTPHeader current_header( saved_req.headers(i) );
        if ( HTTPMessage::equivalent_strings( current_header.key(), stored_header ) ) { /* compare to environment variable */
            return current_header.value().substr( 0, current_header.value().find( "\r\n" ) ) == req_value;
        }
    }
    
    /* request header not in stored header */
    return true;
}

/* compare request_line and certain headers of incoming request and stored request */
bool ReplayServer::compare_requests( const HTTPRequest & request, const HTTP_Record::reqrespair & saved_record, vector< HTTP_Record::reqrespair > & possible_matches ) const
{
    HTTP_Record::http_message saved_req = saved_record.req();

    /* request line */
    string new_req = request.get_first_line() + "\r\n";

    uint64_t query_loc = saved_req.first_line().find( "?" );
    if ( query_loc == std::string::npos ) { /* no query string: compare object name */
        if ( not ( new_req == saved_req.first_line() ) ) {
            return false;
        }
    } else { /* query string present: compare and if not match but object names matches, add to possibilities */
        if ( not ( new_req == saved_req.first_line() ) ) {
            uint64_t query_loc_request = new_req.find( "?" );
            string no_query;
            if( query_loc_request == std::string::npos ) {
                no_query = new_req.substr( 0, new_req.rfind( " " ) );
            } else {
                no_query = new_req.substr( 0, query_loc_request );
            }
            if ( no_query == saved_req.first_line().substr( 0, query_loc ) ) { /* request w/o query string matches */
                possible_matches.emplace_back( saved_record );
            }
            return false;
        }
    }

    /* compare existing environment variables for request to stored header values */
    if ( not check_headers( request, "Accept-Encoding", saved_req ) ) { return false; }
    if ( not check_headers( request, "Accept-Language", saved_req ) ) { return false; }
    //if ( not check_headers( "HTTP_CONNECTION", "Connection", saved_req ) ) { return false; }
    //if ( not check_headers( "HTTP_COOKIE", "Cookie", saved_req ) ) { return false; }
    if ( not check_headers( request, "Host", saved_req ) ) { return false; }
    //if ( not check_headers( "HTTP_REFERER", "Referer", saved_req ) ) { return false; }
    //if ( not check_headers( "HTTP_USER_AGENT", "User-Agent", saved_req ) ) { return false; }

    /* all compared fields match */
    return true;
}

/* return size of longest substring match between two strings */
int ReplayServer::longest_substr( const string & str1, const string & str2 )
{
    /* http://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Longest_common_substring#C.2B.2B */

    if ( str1.empty() or str2.empty() ) {
        return 0;
    }

    vector< int > curr (str2.size() );
    vector< int > prev (str2.size() );
    vector< int > swap;
    int maxSubstr = 0;

    for ( unsigned int i = 0; i < str1.size(); i++ ) {
        for ( unsigned int j = 0; j < str2.size(); j++) {
            if ( str1[ i ] != str2[ j ] ) {
                curr.at( j ) = 0;
            } else {
                if ( i == 0 || j == 0 ) {
                    curr.at( j ) = 1;
                } else {
                    curr.at( j ) = 1 + prev.at( j-1 );
                }
                if( maxSubstr < curr.at( j ) ) {
                    maxSubstr = curr.at( j );
                }
            }
        }
        swap=curr;
        curr=prev;
        prev=swap;
    }
    return maxSubstr;
}

/* return index of stored request_line with longest matching substring to current request */
int ReplayServer::closest_match( const vector< HTTP_Record::reqrespair > & possible, string && req ) const
{
    vector< int > longest_str;
    for ( unsigned int i = 0; i < possible.size(); i++ ) {
        string current_req = possible.at( i ).req().first_line();
        longest_str.emplace_back( longest_substr( req, current_req ) );
    }
    vector< int >::iterator result = max_element( begin( longest_str ), end( longest_str ) );
    return distance( begin( longest_str ), result );
}

/* write response to stdout (using no-parsed headers for apache ) */
void ReplayServer::return_message( const HTTP_Record::reqrespair & record, stringstream & out ) const
{
    out << record.res().first_line();
    for ( int j = 0; j < record.res().headers_size(); j++ ) {
        out << record.res().headers( j );
    }
    out << record.res().body() << endl;
}

const std::string ReplayServer::replay( const HTTPRequest & request ) const
{
    vector< HTTP_Record::reqrespair > possible_matches;
    possible_matches.reserve( all_responses.size() );

    stringstream response;

    unsigned int i;
    for ( i = 0; i < all_responses.size(); i++ ) { /* iterate through recorded files and compare requests to incoming req */
        const HTTP_Record::reqrespair & current_record = all_responses[i];
        if ( compare_requests( request, current_record, possible_matches ) ) { /* requests match */
            return_message( current_record, response );
            break;
        }
    }

    if ( i == all_responses.size() ) { /* no exact matches for request */
        if ( possible_matches.size() == 0 ) { /* no potential matches */
            response << "HTTP/1.1 200 OK\r\n";
            response << "Content-Type: Text/html\r\n";
            response << "Connection: close\r\n";
            response << "Cache-Control: no-cache, no-store, must-revalidate\r\n";
            response << "Pragma: no-cache\r\n";
            response << "Expires: 0\r\n";
            response << "Content-Length: 24\r\n\r\nCOULD NOT FIND AN OBJECT";
        } else { /* return possible match with largest shared substring */
            return_message( possible_matches.at( closest_match( possible_matches, request.get_first_line() + "\r\n" ) ), response );
        }
    }

    return response.str();
}

ReplayServer::ReplayServer( const std::string & record_folder ) {
    vector< string > files;
    list_files( record_folder, files );
    for ( unsigned int i = 0; i < files.size(); i++ ) {
        int fd = SystemCall( "open", open( files[i].c_str(), O_RDONLY ) );
        HTTP_Record::reqrespair current_record;
        current_record.ParseFromFileDescriptor( fd );
        all_responses.push_back( move( current_record ) );
        SystemCall( "close", close( fd ) );
    }
}
