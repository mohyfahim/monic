#pragma once

#include "database.hpp"
#include "httplib.h"

namespace monic {

typedef struct {
  bool connect;
  monic_storage_t *storage;
  httplib::Client *http_client_ptr;
} state_t;

} // namespace monic

//   httplib::Result res = cli.Get("/");
//   if (!res) {
//     const httplib::Error err = res.error();

//     switch (err) {
//     case httplib::Error::SSLConnection:
//       std::cout << "SSL connection failed, SSL error: " << res.ssl_error()
//                 << std::endl;
//       break;

//     case httplib::Error::SSLLoadingCerts:
//       std::cout << "SSL cert loading failed, OpenSSL error: " << std::hex
//                 << res.ssl_openssl_error() << std::endl;
//       break;

//     case httplib::Error::SSLServerVerification:
//       std::cout << "SSL verification failed, X509 error: "
//                 << res.ssl_openssl_error() << std::endl;
//       break;

//     case httplib::Error::SSLServerHostnameVerification:
//       std::cout << "SSL hostname verification failed, X509 error: "
//                 << res.ssl_openssl_error() << std::endl;
//       break;

//     default:
//       std::cout << "HTTP error: " << httplib::to_string(err) << std::endl;
//     }
//   } else {
//     std::cout << "Result: " << res->status << "," << res->body << std::endl;
//   }
