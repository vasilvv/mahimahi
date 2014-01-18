/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <memory>
#include <csignal>
#include <algorithm>
#include <iostream>
#include <vector>

#include "util.hh"
#include "system_runner.hh"
#include "http_record.pb.h"
#include "exception.hh"
#include "file_descriptor.hh"

using namespace std;

int main( int argc, char *argv[] )
{
    try {
        if ( argc != 2 ) {
            throw Exception( "Usage", string( argv[0] ) + " folder_with_recorded_content" );
        }
        string directory( argv[ 1 ] );
        if( directory.back() != '/' ) {
             directory.append( "/" );
        }
        vector< string > files;
        list_files( directory.c_str(), files );
        FileDescriptor bulkreply = SystemCall( "open", open( "bulkreply.proto", O_WRONLY | O_CREAT, 00700 ) );
        uint32_t num_files = files.size();
        bulkreply.write( "HTTP/1.1 200 OK\r\nContent-Type: application/x-bulkreply\r\n\r\n");
        SystemCall( "write", write( bulkreply.num(), &num_files, 4 ) );
        unsigned int i;
        for ( i = 0; i < num_files; i++ ) { /* iterate through recorded files and for each request, append size and request to bulkreply */
            FileDescriptor fd = SystemCall( "open", open( files[i].c_str(), O_RDONLY ) );
            HTTP_Record::reqrespair current_record;
            current_record.ParseFromFileDescriptor( fd.num() );

            /* serialize request to string and obtain size */
            string current_req;
            current_record.req().SerializeToString( &current_req );
            uint32_t req_size = current_req.size();
            SystemCall( "write", write( bulkreply.num(), &req_size, 4 ) );
            bulkreply.write( current_req );
        }
        for ( i = 0; i < num_files; i++ ) { /* iterate through recorded files and for each response, append size and response to bulkreply */
            FileDescriptor fd = SystemCall( "open", open( files[i].c_str(), O_RDONLY ) );
            HTTP_Record::reqrespair current_record;
            current_record.ParseFromFileDescriptor( fd.num() );

            /* serialize response to string and obtain size */
            string current_res;
            current_record.res().SerializeToString( &current_res );
            uint32_t res_size = current_res.size();
            SystemCall( "write", write( bulkreply.num(), &res_size, 4 ) );
            bulkreply.write( current_res );
        }
    } catch ( const Exception & e ) {
        e.perror();
        return EXIT_FAILURE;
    }
}
