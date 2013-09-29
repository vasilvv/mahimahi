/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <poll.h>

#include <thread>
#include <string>
#include <iostream>
#include <utility>

#include "address.hh"
#include "socket.hh"
#include "timestamp.hh"

using namespace std;

void service_request( const Socket & server_socket )
{
    try {
        /* open socket to 172.30.100.100 and an assigned port */
        Socket outgoing_socket;
        outgoing_socket.connect( Address( "172.30.100.100", "0" ) );

        struct pollfd pollfds[ 2 ];
        pollfds[ 0 ].fd = outgoing_socket.raw_fd();
        pollfds[ 0 ].events = POLLIN;
          
        pollfds[ 1 ].fd = server_socket.raw_fd();
        pollfds[ 1 ].events = POLLIN;

        /* process requests until either socket is idle for 20 seconds */
        while( true ) {
             if ( poll( &pollfds[ 1 ], 1, 20000 ) < 0 ) {
                throw Exception( "poll" );
            }
            if ( pollfds[ 1 ].revents & POLLIN ) {
                /* read request, then send to local dns server */
                outgoing_socket.write( server_socket.read() );
            }
            if ( poll( &pollfds[ 0 ], 1, 20000 ) < 0 ) {
                throw Exception( "poll" );
            }
           /* if response comes from local dns server, write back to source of request */
           if ( pollfds[ 0 ].revents & POLLIN ) {
                /* read response, then send back to client */
                server_socket.write( outgoing_socket.read() );
            }
        }
    
    } catch ( const Exception & e ) {
        cerr.flush();
        e.perror();
        return;
    }        

    cerr.flush();
    return;
}

int main( void )
{
    /* make listener socket for dns requests */
    try {
        Socket listener_socket;
        /* bind to 127.0.0.1 and port 53 for DNS requests */
        listener_socket.bind( Address( "localhost", "domain" ) );

        cout << "hostname: " << listener_socket.local_addr().hostname() << endl;
        cout << "port: " << listener_socket.local_addr().port() << endl;

        /* listen on listener socket */
        listener_socket.listen();

        Socket listen_connect = listener_socket.accept();

        while ( true ) {  /* service requests from this source  */
            thread newthread( [&listen_connect] () -> void {
                    service_request( listen_connect ); } );
            newthread.detach();
        }
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
