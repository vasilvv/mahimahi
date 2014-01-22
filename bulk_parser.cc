/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <sstream>
#include <fstream>
#include <assert.h>
#include "ezio.hh"
#include "bulk_parser.hh"

using namespace std;

string::size_type BulkBodyParser::read( const std::string & input_buffer, Archive & archive )
{
    last_size_ = parser_buffer_.size();
    int used_now;
    parser_buffer_ += input_buffer;

    while ( !parser_buffer_.empty() ) {
        switch (state_) {
        case RESPONSE_SIZE: {
            if ( parser_buffer_.size() >= 4) { /* have enough for bulk response size */
                /* set request/response left to total (first 4 bytes) */
                std::istringstream ifs( parser_buffer_.substr(0,4));
                uint32_t total_size;
                ifs.read(reinterpret_cast<char*>(&total_size), sizeof total_size);
                cout << "SIZE: " << total_size << endl;
                requests_left_ = total_size;
                responses_left_ = total_size;

                /* Transition appropriately */
                state_ = MESSAGE_HDR;

                /* shrink parser_buffer_ */
                parser_buffer_ = parser_buffer_.substr( 4 );
                acked_so_far_ = acked_so_far_ + 4;
                used_now = used_now + 4;

                break;
            } else {
                /* Haven't seen enough bytes so far, do nothing */
                return string::npos;
            }
        }

        case MESSAGE_HDR: {
            if ( parser_buffer_.size() >= 4 ) { /* have enough for current message size */
                cout << "PARSER BUFFER SIZE BEFORE SIZE: " << parser_buffer_.size() << endl;
                std::istringstream its( parser_buffer_.substr(0,4));
                uint32_t message_size;
                its.read(reinterpret_cast<char*>(&message_size), sizeof message_size);
                current_message_size_ = message_size;
                cout << "MESSAGE SIZE: " << current_message_size_ << endl;

               /* Transition to next state */
                state_ = MESSAGE;

                /* shrink parser_buffer_ */
                parser_buffer_ = parser_buffer_.substr( 4 );
                cout << "PARSER BUFFER SIZE AFTER SIZE: " << parser_buffer_.size() << endl;
                acked_so_far_ = acked_so_far_ + 4;
                used_now = used_now + 4;
                break;
            } else {
                /* Haven't seen enough bytes so far, do nothing */
                return string::npos;
            }
        }

        case MESSAGE: {
            if ( parser_buffer_.size() >= current_message_size_ ) { /* we have the entire message */
                if ( requests_left_ > 0 ) { /* this is a request so store protobuf in pending */
                    fstream requests_fd_("logbulk.req", std::ios::binary|std::ios::out);
                    requests_fd_.write( parser_buffer_.substr(0, current_message_size_).c_str(), current_message_size_);
                    HTTP_Record::http_message request;
                    request.ParseFromString( parser_buffer_.substr( 0, current_message_size_) );
                    cout << "ADDING REQUEST" << endl;
                    archive.add_request( request );
                    requests_left_ = requests_left_ - 1;
                    cout << "PENDING SIZE AFTER ADDING: " << archive.num_of_requests() << endl;
                } else { /* this is a response so store string in pending_ */
                    cout << "ADDING RESPONSE" << endl;
                    HTTP_Record::http_message response;
                    response.ParseFromString( parser_buffer_.substr( 0, current_message_size_) );
                    string tot_response;
                    tot_response.append( response.first_line() );
                    for ( int j = 0; j < response.headers_size(); j++ ) {
                        tot_response.append( response.headers( j ) );
                    }
                    tot_response.append( response.body() ); 
                    size_t pos = archive.num_of_requests() - responses_left_;
                    //archive.add_response( parser_buffer_.substr( 0, current_message_size_ ), pos );
                    archive.add_response( tot_response, pos );
                    responses_left_ = responses_left_ - 1;
                }
                acked_so_far_ = acked_so_far_ + current_message_size_;
                used_now = used_now + current_message_size_;

                /* Transition back to begin next message */
                state_ = MESSAGE_HDR;

                /* shrink parser_buffer_ */ 
                parser_buffer_ = parser_buffer_.substr( current_message_size_ );

                if ( responses_left_ == 0 ) { /* entire bulk response is complete */
                    cout << "FINISHED WITH: " << parser_buffer_.size() << endl;
                    //return acked_so_far_;
                    return ( used_now - last_size_ );
                }
                break;
            } else {
                /* Haven't seen enough bytes so far for message, do nothing */
                return string::npos;
            }
        }

        default: {
            assert( false );
            return false;
        }
        }
    }
    return string::npos;
}
