/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef YOUTUBE_SERVER_HH
#define YOUTUBE_SERVER_HH

#include <unordered_map>
#include <sys/types.h>

#include "replayserver.hh"
#include "youtube_util.hh"

class YouTubeServer : public ReplayServer
{
private:
    std::unordered_map<off_t, std::string> videofiles{};

protected:
    virtual const std::string handle_special_request( const HTTPRequest & request ) override;

public:
    YouTubeServer( const std::string & record_folder, const std::string & videodir );
};

#endif /* YOUTUBE_SERVER_HH */
