/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef REPLAYSERVER_HH
#define REPLAYSERVER_HH

#include <vector>
#include <string>
#include <sstream>
#include <memory>

#include "http_request.hh"
#include "http_record.pb.h"

class ReplayServer {
private:
    std::vector< HTTP_Record::reqrespair > all_responses{};

    bool check_headers( const HTTPRequest & request, const std::string & stored_header, HTTP_Record::http_message & saved_req ) const;
    bool compare_requests( const HTTPRequest & request, const HTTP_Record::reqrespair & saved_record, std::vector< HTTP_Record::reqrespair > & possible_matches ) const;
    static int longest_substr( const std::string & str1, const std::string & str2 );
    int closest_match( const std::vector< HTTP_Record::reqrespair > & possible, std::string && req ) const;
    void return_message( const HTTP_Record::reqrespair & record, std::stringstream & out ) const;
public:
    ReplayServer( const std::string & record_folder );

    const std::string replay( const HTTPRequest & request ) const;
};

#endif /* REPLAYSERVER_HH */
