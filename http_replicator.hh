/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef HTTP_REPLICATOR_HH
#define HTTP_REPLICATOR_HH

#include "http_proxy.hh"
#include "replayserver.hh"

class HTTPReplicator : public HTTPProxy
{
protected:
    ReplayServer replayer;
public:
    HTTPReplicator( const Address & listener_addr, const std::string & record_folder );
    virtual void handle_tcp( void );
};

#endif /* HTTP_REPLICATOR_HH */
