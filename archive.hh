/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <string>
#include <vector>
#include <utility>

#include "http_record.pb.h"

#ifndef ARCHIVE_HH
#define ARCHIVE_HH

static const std::string default_filename_template = "apache_config";

class Archive
{
private:
    std::vector< std::pair< HTTP_Record::http_message, std::string > > pending_ {};

    std::string get_corresponding_response( const HTTP_Record::http_message & new_req );

public:
    Archive() {};

    /* Add a request */
    void add_request( const HTTP_Record::http_message & request );

    /* Add a response */
    void add_response( const std::string & response, const size_t position );

    /* Do we have a matching request that is pending? */
    bool request_pending( const HTTP_Record::http_message & new_req );

    /* Do we have a stored response for this request? */
    bool have_response( const HTTP_Record::http_message & new_req );

    /* Return the corresponding response to the stored request (caller should first call have_response) */ 
    std::string corresponding_response( const HTTP_Record::http_message & new_req );

    size_t num_of_requests( void ) { return pending_.size(); }
};

#endif
