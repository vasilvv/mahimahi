/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef YOUTUBE_UTIL_HH
#define YOUTUBE_UTIL_HH

#include <string>

inline bool is_youtube_media_request( const std::string & first_line ) {
    return first_line.find( "GET /videoplayback?" ) == 0;
}

#endif /* YOUTUBE_UTIL_HH */
