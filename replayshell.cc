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
using namespace PollerShortNames;

int eventloop( ChildProcess & child_process )
{
    /* set up signal file descriptor */
    SignalMask signals_to_listen_for = { SIGCHLD, SIGCONT, SIGHUP, SIGTERM };
    signals_to_listen_for.block(); /* don't let them interrupt us */

    SignalFD signal_fd( signals_to_listen_for );

    Poller poller;

    /* we get signal -> main screen turn on -> handle signal */
    poller.add_action( Poller::Action( signal_fd.fd(), Direction::In,
                                       [&] () {
                                           return handle_signal( signal_fd.read_signal(),
                                                                 child_process );
                                       } ) );

    while ( true ) {
        auto poll_result = poller.poll( 60000 );
        if ( poll_result.result == Poller::Result::Type::Exit ) {
            return poll_result.exit_status;
        }
    }
}

void add_dummy_interface( const string & name, const Address & addr )
{

    run( { IP, "link", "add", name.c_str(), "type", "dummy" } );

    interface_ioctl( Socket( UDP ).fd(), SIOCSIFFLAGS, name.c_str(),
             [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );
    interface_ioctl( Socket( UDP ).fd(), SIOCSIFADDR, name.c_str(),
         [&] ( ifreq &ifr )
         { ifr.ifr_addr = addr.raw_sockaddr(); } );
}

void list_files( const string & dir, vector< string > & files )
{
    DIR *dp;
    struct dirent *dirp;

    if( ( dp  = opendir( dir.c_str() ) ) == NULL ) {
        throw Exception( "opendir" );
    }

    while ( ( dirp = readdir( dp ) ) != NULL ) {
        if ( string( dirp->d_name ) != "." and string( dirp->d_name ) != ".." ) {
            files.push_back( "blah/" + string( dirp->d_name ) );
        }
    }
    SystemCall( "closedir", closedir( dp ) );
}

/* make webserver for unique ip/port and dummy interface for each new ip */
void handle_addr( const Address & current_addr, vector< Address > & unique_addrs, unsigned int & interface_counter )
{
    for ( unsigned int j = 0; j < unique_addrs.size(); j++ ) {
        if ( current_addr == unique_addrs[ j ] ) { return; }
        j++;
    }

    /* Address (ip/port pair) does not exist */
    for ( unsigned int j = 0; j < unique_addrs.size(); j++ ) {
        if ( current_addr.ip() == unique_addrs[ j ].ip() ) { /* same ip, check port */
            assert( current_addr.port() != unique_addrs[ j ].port() );
            WebServer( current_addr );
            return;
        }
        j++;
    }

    /* ip does not exist */
    add_dummy_interface( "dumb" + to_string( interface_counter ), current_addr );
    interface_counter++;
    WebServer ( current_addr );
}

int main( int argc, char *argv[] )
{
    try {
        /* clear environment */
        char **user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        SystemCall( "unshare", unshare( CLONE_NEWNET ) );

        /* bring up localhost */
        interface_ioctl( Socket( UDP ).fd(), SIOCSIFFLAGS, "lo",
                         [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );

        unsigned int interface_counter = 0;
        vector< string > files;
        vector< Address > unique_addrs;
        list_files( "blah", files );
        for ( unsigned int i = 0; i < files.size(); i++ ) {
            int fd = SystemCall( "open", open( files[i].c_str(), O_RDONLY ) );
            HTTP_Record::reqrespair current_pair;
            current_pair.ParseFromFileDescriptor( fd );
            Address current_addr( current_pair.ip(), current_pair.port() );
            handle_addr( current_addr, unique_addrs, interface_counter );
        }

        srandom( time( NULL ) );

        ChildProcess bash_process( [&]() {
                drop_privileges();

                /* restore environment and tweak bash prompt */
                environ = user_environment;
                prepend_shell_prefix( "[replayshell] " );
                const string shell = shell_path();
                SystemCall( "execl", execl( shell.c_str(), shell.c_str(), static_cast<char *>( nullptr ) ) );
                return EXIT_FAILURE;
        } );
        return eventloop( bash_process );
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
