/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <thread>
#include <string>
#include <iostream>
#include <utility>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/netfilter_ipv4.h>
#include <signal.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <time.h>

#include "address.hh"
#include "socket.hh"
#include "timestamp.hh"
#include "system_runner.hh"
#include "config.h"
#include "signalfd.hh"
#include "http_replicator.hh"
#include "poller.hh"
#include "bytestream_queue.hh"
#include "http_request_parser.hh"
#include "http_response_parser.hh"
#include "file_descriptor.hh"

using namespace std;
using namespace PollerShortNames;

HTTPReplicator::HTTPReplicator( const Address & listener_addr, const std::string & record_folder ) : HTTPProxy( listener_addr, record_folder ), replayer( record_folder )
{
}

void HTTPReplicator::handle_tcp( void )
{
    thread newthread( [&] ( Socket client ) {
            try {
                /* get original destination for connection request */
                Address original_destaddr = client.original_dest();
                cout << "connection intended for: " << original_destaddr.ip() << endl;

                Poller poller;

                HTTPRequestParser request_parser;

                auto dst_port = original_destaddr.port();

                /* Create Read/Write Interface for client */
                std::unique_ptr<ReadWriteInterface> client_rw  = (dst_port == 443) ?
                                                                 static_cast<decltype( client_rw )>( new SecureSocket( move( client ), SERVER ) ) :
                                                                 static_cast<decltype( client_rw )>( new Socket( move( client ) ) );

                /* poll on original connect socket and new connection socket to ferry packets */
                /* requests from client go to request parser */
                poller.add_action( Poller::Action( client_rw->fd(), Direction::In,
                                                   [&] () {
                                                       string buffer = client_rw->read();
                                                       request_parser.parse( buffer );
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () { return true; } ) );

                /* completed requests from client are serialized and sent to server */
                poller.add_action( Poller::Action( client_rw->fd(), Direction::Out,
                                                   [&] () {
                                                       client_rw->write( replayer.replay( request_parser.front() ) );
                                                       request_parser.pop();
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () { return not request_parser.empty(); } ) );

                timespec sleep_time = { 0, 1000 * 50 };
                while( true ) {
                    auto poll_result = poller.poll( 60000 );
                    nanosleep( &sleep_time, nullptr ); // Prevent CPU overload under high load
                    if ( poll_result.result == Poller::Result::Type::Exit ) {
                        return;
                    }
                }
            } catch ( const Exception & e ) {
                e.perror();
                return;
            }
            return;
        }, listener_socket_.accept() );

    /* don't wait around for the reply */
    newthread.detach();
}

