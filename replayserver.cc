/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <memory>
#include <csignal>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <net/route.h>
#include <iostream>
#include <vector>
#include <dirent.h>
#include <set>

#include "util.hh"
#include "get_address.hh"
#include "address.hh"
#include "signalfd.hh"
#include "netdevice.hh"
#include "web_server.hh"
#include "system_runner.hh"
#include "config.h"
#include "socket.hh"
#include "config.h"
#include "poller.hh"
#include "http_record.pb.h"

using namespace std;

/*
Find request in environment variables
for each request, compare to the stored requests until we find a match
(can have a function which takes two requests and compares them while ignoring the 
time-sensitive headers and stuff after the ?)
Then, once we find a match, we will write the response (potentially without the time
sensitive headers).
*/

void list_files( const string & dir, vector< string > & files )
{
    DIR *dp;
    struct dirent *dirp;

    if( ( dp  = opendir( dir.c_str() ) ) == NULL ) {
        throw Exception( "opendir" );
    }

    while ( ( dirp = readdir( dp ) ) != NULL ) {
        if ( string( dirp->d_name ) != "." and string( dirp->d_name ) != ".." ) {
            files.push_back( dir + string( dirp->d_name ) );
        }
    }
    SystemCall( "closedir", closedir( dp ) );
}

bool compare_requests( const string & saved_req, const string & new_req )
{
    //cout << "ORIGINAL REQUEST: " << saved_req << "<br />\n" << endl;
    cout << "NEW REQUEST IN FUNCTION: " << new_req << "<br />\n" << endl;

    if ( new_req == saved_req.substr( 0, saved_req.find( "\r\n" ) + 4 ) ) {
        return true;
    } 
    return false;
}

int main()
{
    try {
        extern char **environ;
        cout << "Content-type: text/html\r\n\r\n";
        for(char **current = environ; *current; current++) {
            cout << *current << endl << "<br />\n";
        }
        string request;
        cout << "THIS IS THE REQUEST_URI: " << getenv( "REQUEST_URI" ) << "<br />\n" << endl;
        string new_req = string( getenv( "REQUEST_METHOD" ) ) + " " + string( getenv( "SCRIPT_NAME" ) ) + " " + string ( getenv( "SERVER_PROTOCOL" ) ) + "\r\n";
        cout << "NEW REQUEST: " << new_req << "<br />\n" << endl;

        vector< string > files;
        list_files( "/home/ravi/mahimahi/blah/", files );
        for ( unsigned int i = 0; i < files.size(); i++ ) { // iterate through files and call method which compares requests
            int fd = SystemCall( "open", open( files[i].c_str(), O_RDONLY ) );
            HTTP_Record::reqrespair current_record;
            current_record.ParseFromFileDescriptor( fd );
            HTTP_Record::http_message saved_req = current_record.req();
            if ( compare_requests( saved_req.first_line(), new_req ) ) { // requests match
                cout << current_record.res().body() << endl;
            }
            cout << "First Line: " << saved_req.first_line() << "DONE" << endl;
            cout << "HEADER: " << saved_req.headers(1) << "DONE" << endl;
            cout << "BODY: " << saved_req.body() << "DONE" << endl;
            SystemCall( "close", close( fd ) );
        }
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
