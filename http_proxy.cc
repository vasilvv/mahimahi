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

#include "address.hh"
#include "socket.hh"
#include "timestamp.hh"
#include "system_runner.hh"
#include "config.h"
#include "signalfd.hh"
#include "http_proxy.hh"
#include "poller.hh"
#include "bytestream_queue.hh"
#include "http_request_parser.hh"
#include "http_response_parser.hh"
#include "file_descriptor.hh"

using namespace std;
using namespace PollerShortNames;

HTTPProxy::HTTPProxy( const Address & listener_addr )
    : listener_socket_( TCP ),
      stored_pairs_()
{
    listener_socket_.bind( listener_addr );
    listener_socket_.listen();

    /* set starting seed for random number file names */
    srandom( time( NULL ) );

    /* SSL initialization: Needs to be done exactly once */
    /* load algorithms/ciphers */
    SSL_library_init();
    OpenSSL_add_all_algorithms();

    /* load error messages */
    SSL_load_error_strings();
}

void HTTPProxy::handle_tcp( Archive & archive )
{
    thread newthread( [&] ( Socket client ) {
            try {
                /* get original destination for connection request */
                Address original_destaddr = client.original_dest();
                cout << "connection intended for: " << original_destaddr.ip() << endl;

                /* create socket and connect to original destination and send original request */
                Socket server( TCP );
                server.connect( original_destaddr );

                Poller poller;

                HTTPRequestParser request_parser;

                HTTPResponseParser response_parser;

                HTTP_Record::reqrespair current_pair;

                auto dst_port = original_destaddr.port();

                /* Set destination ip, port and protocol in current request/response pair */
                current_pair.set_ip( original_destaddr.ip() );
                current_pair.set_port( dst_port );
                ( dst_port == 443 ) ? current_pair.set_protocol( "HTTPS" ) : current_pair.set_protocol( "HTTP" );

                /* Create Read/Write Interfaces for server and client */
                std::unique_ptr<ReadWriteInterface> server_rw  = (dst_port == 443) ?
                                                                 static_cast<decltype( server_rw )>( new SecureSocket( move( server ), CLIENT ) ) :
                                                                 static_cast<decltype( server_rw )>( new Socket( move( server ) ) );
                std::unique_ptr<ReadWriteInterface> client_rw  = (dst_port == 443) ?
                                                                 static_cast<decltype( client_rw )>( new SecureSocket( move( client ), SERVER ) ) :
                                                                 static_cast<decltype( client_rw )>( new Socket( move( client ) ) );
                /* Make bytestream_queue for browser->server and server->browser */
                ByteStreamQueue from_destination( ezio::read_chunk_size );

                /* poll on original connect socket and new connection socket to ferry packets */
                /* responses from server go to response parser and bytestreamqueue */
                poller.add_action( Poller::Action( server_rw->fd(), Direction::In,
                                                   [&] () {
                                                       if ( dst_port == 443 ) { /* SSL_read decrypts when full record -> if ssl, only read if we have full record size available to push */
                                                           if ( from_destination.contiguous_space_to_push() < 16384 ) { return ResultType::Continue; }
                                                       }
                                                       string buffer = server_rw->read_amount( from_destination.contiguous_space_to_push() );
                                                       //from_destination.push_string( buffer );
                                                       response_parser.parse( buffer, archive );
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () { return ( not client_rw->fd().eof() and from_destination.space_available() ); } ) );

                /* requests from client go to request parser */
                poller.add_action( Poller::Action( client_rw->fd(), Direction::In,
                                                   [&] () {
                                                       string buffer = client_rw->read();
                                                       request_parser.parse( buffer );
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () { return not server_rw->fd().eof(); } ) );

                /* completed requests from client are serialized and sent to server */
                poller.add_action( Poller::Action( server_rw->fd(), Direction::Out,
                                                   [&] () {
                                                       /* check if request is stored: if pending->wait, if response present->send to client, if neither->send request to server */
                                                       HTTP_Record::http_message complete_request = request_parser.front().toprotobuf();
                                                       if ( archive.request_pending( complete_request ) ) {
                                                           cout << "WE ARE HANDLING REQUEST WITH ARCHIVE" << endl;
                                                           while ( archive.request_pending( complete_request ) ) { /* wait until we have the response filled in */
                                                               sleep( 1 );
                                                           }
                                                           if ( from_destination.contiguous_space_to_push() >= archive.corresponding_response( complete_request ).size() ) { /* we have space to add response */
                                                               from_destination.push_string( request_parser.front().str() );
                                                           } else {
                                                               return ResultType::Continue;
                                                           }

                                                       } else if ( archive.have_response( complete_request ) ) { /* corresponding response already stored- send to client */
                                                           if ( from_destination.contiguous_space_to_push() >= archive.corresponding_response( complete_request ).size() ) { /* we have space to add response */
                                                               cout << "USING OUR RESPONSE" << endl;
                                                               from_destination.push_string( archive.corresponding_response( complete_request ) );
                                                           } else {
                                                               return ResultType::Continue;
                                                           }
                                                       } else { /* request not listed in archive- send request to server */
                                                           server_rw->write( request_parser.front().str() );
                                                           response_parser.new_request_arrived( request_parser.front() );
                                                           /* add request to current request/response pair */
                                                           current_pair.mutable_req()->CopyFrom( request_parser.front().toprotobuf() );
                                                       }

                                                       request_parser.pop();
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () { return not request_parser.empty(); } ) );

                /* completed responses from server are serialized and bytestreamqueue contents are sent to client */
                poller.add_action( Poller::Action( client_rw->fd(), Direction::Out,
                                                   [&] () {
                                                       if ( dst_port == 443 ) {
                                                           from_destination.pop_ssl( move( client_rw ) );
                                                       } else {
                                                           from_destination.pop( client_rw->fd() );
                                                       }
                                                       if ( not response_parser.empty() ) {
                                                           reqres_to_protobuf( current_pair, response_parser.front() );
                                                           response_parser.pop();
                                                       }
                                                       return ResultType::Continue;
                                                   },
                                                   [&] () { return from_destination.non_empty(); } ) );

                while( true ) {
                    auto poll_result = poller.poll( 60000 );
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

void HTTPProxy::reqres_to_protobuf( HTTP_Record::reqrespair & current_pair, const HTTPResponse & response )
{
    /* if request is present in current request/response pair, add response and add to vector */
    if ( current_pair.has_req() ) {
        current_pair.mutable_res()->CopyFrom( response.toprotobuf() );
        stored_pairs_.emplace_back( current_pair );
    } else {
        throw Exception( "Protobuf", "Response ready without Request" );
    }

    /* clear current request/response pair */
    current_pair.clear_req();
    current_pair.clear_res();
}
