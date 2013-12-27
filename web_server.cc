/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>
#include <iostream>

#include "web_server.hh"
#include "apache_configuration.hh"
#include "system_runner.hh"
#include "config.h"

using namespace std;

WebServer::WebServer( const string & listen_line, int port )
    : config_file_name()
{
    TempFile config_file( apache_main_config );
    TempFile lock_file( "" );

    config_file_name = "/home/ravi/mahimahi/" + config_file.name();

    if ( port == 443 ) { /* ssl */
        config_file.append( apache_ssl_config );
    }

    config_file.append( "PidFile /home/ravi/mahimahi/" + lock_file.name() + "\n");

    config_file.append( listen_line );

    run( { APACHE2, "-f", config_file_name, "-k", "start" } );
}

WebServer::~WebServer()
{
    run( { APACHE2, "-f", config_file_name, "-k", "stop" } );
}
