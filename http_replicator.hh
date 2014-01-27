/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_REPLICATOR_HH
#define HTTP_REPLICATOR_HH

#include "http_proxy.hh"
#include "replayserver.hh"

class HTTPReplicator : public HTTPProxy
{
protected:
    std::unique_ptr<ReplayServer> replayer_;
public:
    HTTPReplicator( const Address & listener_addr, const std::string & record_folder, ReplayServer* replayer );
    virtual void handle_tcp( void );
};

#endif /* HTTP_REPLICATOR_HH */
