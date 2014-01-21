/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <assert.h>
#include "ezio.hh"
#include "bulk_parser.hh"

using namespace std;

string::size_type BulkBodyParser::read( const std::string & input_buffer, Archive & archive )
{
    parser_buffer_ += input_buffer;

    while ( !parser_buffer_.empty() ) {
        switch (state_) {
        case RESPONSE_SIZE: {
            if ( parser_buffer_.size() >= 4) { /* have enough for bulk response size */
                /* set request/response left to total (first 4 bytes) */
                requests_left_ = atoi( parser_buffer_.substr( 0, 4 ).c_str() );
                responses_left_ = atoi( parser_buffer_.substr( 0, 4 ).c_str() );
                cout << "READ SIZE: " << atoi(parser_buffer_.substr( 0, 4 ).c_str()) << endl;
                cout << "REQUESTS_LEFT AT BEGINNING: " << requests_left_ << endl;

                /* Transition appropriately */
                state_ = MESSAGE_HDR;

                /* shrink parser_buffer_ */
                parser_buffer_ = parser_buffer_.substr( 4 );
                acked_so_far_ = acked_so_far_ + 4;
                break;
            } else {
                /* Haven't seen enough bytes so far, do nothing */
                return string::npos;
            }
        }

        case MESSAGE_HDR: {
            if ( parser_buffer_.size() >= 4 ) { /* have enough for current message size */
                current_message_size_ = atoi ( parser_buffer_.substr( 0, 4 ).c_str() );

               /* Transition to next state */
                state_ = MESSAGE;

                /* shrink parser_buffer_ */
                parser_buffer_ = parser_buffer_.substr( 4 );
                acked_so_far_ = acked_so_far_ + 4;
                break;
            } else {
                /* Haven't seen enough bytes so far, do nothing */
                return string::npos;
            }
        }

        case MESSAGE: {
            if ( parser_buffer_.size() >= current_message_size_ ) { /* we have the entire message */
                if ( requests_left_ > 0 ) { /* this is a request so store protobuf in pending */
                    HTTP_Record::http_message request;
                    request.ParseFromString( parser_buffer_ );
                    cout << "ADDING REQUEST" << endl;
                    archive.add_request( request );
                    requests_left_ = requests_left_ - 1;
                } else { /* this is a response so store string in pending_ */
                    cout << "ADDING RESPONSE" << endl;
                    size_t pos = archive.num_of_requests() - responses_left_;
                    archive.add_response( parser_buffer_.substr( 0, current_message_size_ ), pos );
                    responses_left_ = responses_left_ - 1;
                }
                acked_so_far_ = acked_so_far_ + current_message_size_;

                /* Transition back to begin next message */
                state_ = MESSAGE_HDR;

                /* shrink parser_buffer_ */ 
                parser_buffer_ = parser_buffer_.substr( current_message_size_ );

                if ( responses_left_ == 0 ) { /* entire bulk response is complete */
                    return acked_so_far_;
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
