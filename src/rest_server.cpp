#include "rest_server.hpp"
#include "json_timeseries.hpp"

namespace live::rest
{
  void server::setup()
  {
    router_.http_post(R"(/timeseries)", []( auto req, auto params ){
      strand_post_.post([](){
        if( json_timeseries(req.body()).post(database_) )
        {
          req.create_response(restinio::status_no_content()).done();

        }
        req.create_response(restinio::status_internal_server_error()).done();
      });
    });
  }
}