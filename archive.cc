/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <stdlib.h>
#include <unistd.h>

#include "archive.hh"
#include "exception.hh"
#include "util.hh"

using namespace std;

void Archive::add_request( const HTTP_Record::http_message & request )
{
    pending_.emplace_back( request ); 
}

void Archive::add_response( const string & respose, const int position )
{
    pending_.at( position ) = response;
}
