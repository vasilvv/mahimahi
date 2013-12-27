/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <memory>
#include <csignal>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <net/route.h>
#include <iostream>

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

/* TESTING TWO DUMMY INTERFACES: dumb1 dumb2 with ips of egress and ingress addrs selected */

using namespace std;
using namespace PollerShortNames;

int pause( void ) {
    while(1) { } return 1;
}

int main( int argc, char *argv[] )
{
    try {
        /* clear environment */
        char **user_environment = environ;
        environ = nullptr;

        check_requirements( argc, argv );

        /* set egress and ingress ip addresses */
        Interfaces interfaces;

        auto egress_octet = interfaces.first_unassigned_address( 1 );
        auto ingress_octet = interfaces.first_unassigned_address( egress_octet.second + 1 );
        Address egress_addr = egress_octet.first, ingress_addr = ingress_octet.first;

        /* make pair of devices */
        //string egress_name = "veth-" + to_string( getpid() ), ingress_name = "veth-i" + to_string( getpid() );
        //VirtualEthernetPair veth_devices( egress_name, ingress_name );
        /* bring up egress */
        //assign_address( egress_name, egress_addr, ingress_addr );
        /* Fork */
        ChildProcess container_process( [&]() {
                /* bring up localhost */
                interface_ioctl( Socket( UDP ).fd(), SIOCSIFFLAGS, "lo",
                                 [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );

                run( { IP, "link", "add", "dumb0", "type", "dummy" } );
                run( { IP, "link", "add", "dumb1", "type", "dummy" } );
                interface_ioctl( Socket( UDP ).fd(), SIOCSIFFLAGS, "dumb0",
                                 [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );
                interface_ioctl( Socket( UDP ).fd(), SIOCSIFFLAGS, "dumb1",
                                 [] ( ifreq &ifr ) { ifr.ifr_flags = IFF_UP; } );
                interface_ioctl( Socket( UDP ).fd(), SIOCSIFADDR, "dumb0",
                     [&] ( ifreq &ifr )
                     { ifr.ifr_addr = egress_addr.raw_sockaddr(); } );
                interface_ioctl( Socket( UDP ).fd(), SIOCSIFADDR, "dumb1",
                     [&] ( ifreq &ifr )
                     { ifr.ifr_addr = ingress_addr.raw_sockaddr(); } );

                /* Fork again after dropping root privileges */
                drop_privileges();
                WebServer apache1( "Listen 100.64.0.1:80", 443);
                ChildProcess bash_process( [&]() {
                        /* restore environment and tweak bash prompt */
                        environ = user_environment;
                        prepend_shell_prefix( "[aliasing] " );

                        const string shell = shell_path();
                        SystemCall( "execl", execl( shell.c_str(), shell.c_str(), static_cast<char *>( nullptr ) ) );
                        return EXIT_FAILURE;
                    } );

                return pause();
            }, true ); /* new network namespace */

        /* give ingress to container */
        //run( { IP, "link", "set", "dev", ingress_name, "netns", to_string( container_process.pid() ) } );

        /* bring up ingress */
        in_network_namespace( container_process.pid(), [&] () {
                /* bring up veth device */
                //assign_address( ingress_name, ingress_addr, egress_addr );
                //SystemCall( "ioctl SIOCADDRT", ioctl( Socket( UDP ).fd().num(), SIOCADDRT, &route ) );
            } );

        return pause();
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
