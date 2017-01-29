/*
    This file is part of Kismet

    Kismet is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Kismet is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Kismet; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "config.h"

#include <stdio.h>
#include <time.h>
#include <list>
#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <pthread.h>
#include <microhttpd.h>

#include "globalregistry.h"
#include "trackedelement.h"

#ifndef __KIS_NET_MICROHTTPD__
#define __KIS_NET_MICROHTTPD__

class Kis_Net_Httpd;
class Kis_Net_Httpd_Session;
class Kis_Net_Httpd_Connection;

class EntryTracker;

// Basic request handler from MHD
class Kis_Net_Httpd_Handler {
public:
    Kis_Net_Httpd_Handler() { }
    Kis_Net_Httpd_Handler(GlobalRegistry *in_globalreg);
    virtual ~Kis_Net_Httpd_Handler();

    // Bind a http server if we need to do that later in the instantiation
    void Bind_Httpd_Server(GlobalRegistry *in_globalreg);

    // Handle a request
    virtual int Httpd_HandleRequest(Kis_Net_Httpd *httpd,
            struct MHD_Connection *connection,
            const char *url, const char *method, const char *upload_data,
            size_t *upload_data_size) = 0;

    // Can this handler process this request?
    virtual bool Httpd_VerifyPath(const char *path, const char *method) = 0;

    // Shortcut to checking if the serializer can handle this, since most
    // endpoints will be implementing serialization
    virtual bool Httpd_CanSerialize(string path);

    // Shortcuts for getting path info
    virtual string Httpd_GetSuffix(string path);
    virtual string Httpd_StripSuffix(string path);

    // Post handler.  By default does nothing and bails on the post data.
    // Override this to do useful post interpreting.  This implements parsing
    // iterative data and will be called multiple times; if you are implementing
    // a post system which takes multiple values you will need to index the state
    // via the connection info and parse them all as you are called from the
    // microhttpd handler.
    //
    // A simpler method is to take a single parameter encoding a JSON string 
    // or a msgpack container, with all the options contained therein.
    virtual int Httpd_PostIterator(void *coninfo_cls, enum MHD_ValueKind kind, 
            const char *key, const char *filename, const char *content_type,
            const char *transfer_encoding, const char *data, 
            uint64_t off, size_t size) {
#if 0
        // Example implementation 
        Kis_Net_Httpd_Connection *concls = (Kis_Net_Httpd_Connection *) coninfo_cls;

        if (strcmp(key, "msgpack") == 0) {
            // Handle data
        }

        concls->response_stream << "OK";

        return MHD_YES;

#endif
        return MHD_NO;
    }

protected:
    GlobalRegistry *http_globalreg;

    shared_ptr<Kis_Net_Httpd> httpd;
    shared_ptr<EntryTracker> entrytracker;

};

// Take a C++ stringstream and use it as a response
class Kis_Net_Httpd_Stream_Handler : public Kis_Net_Httpd_Handler {
public:
    Kis_Net_Httpd_Stream_Handler() { }
    Kis_Net_Httpd_Stream_Handler(GlobalRegistry *in_globalreg) :
        Kis_Net_Httpd_Handler(in_globalreg) { };
    virtual ~Kis_Net_Httpd_Stream_Handler() { };

    virtual bool Httpd_VerifyPath(const char *path, const char *method) = 0;

    virtual void Httpd_CreateStreamResponse(Kis_Net_Httpd *httpd,
            struct MHD_Connection *connection,
            const char *url, const char *method, const char *upload_data,
            size_t *upload_data_size, std::stringstream &stream) = 0;

    virtual int Httpd_HandleRequest(Kis_Net_Httpd *httpd, 
            struct MHD_Connection *connection,
            const char *url, const char *method, const char *upload_data,
            size_t *upload_data_size);

    // Shortcuts to the entry tracker and serializer since most endpoints will
    // need to serialize
    virtual bool Httpd_Serialize(string path, std::stringstream &stream,
            SharedTrackerElement e);
};

// Fallback handler to report that we can't serve static files
class Kis_Net_Httpd_No_Files_Handler : public Kis_Net_Httpd_Stream_Handler {
public:
    virtual bool Httpd_VerifyPath(const char *path, const char *method);

    virtual void Httpd_CreateStreamResponse(Kis_Net_Httpd *httpd,
            struct MHD_Connection *connection,
            const char *url, const char *method, const char *upload_data,
            size_t *upload_data_size, std::stringstream &stream);
};

#define KIS_SESSION_COOKIE      "KISMET"
#define KIS_HTTPD_POSTBUFFERSZ  (1024 * 32)

// Connection data, used for processing POST requests
class Kis_Net_Httpd_Connection {
public:
    const static int CONNECTION_GET = 0;
    const static int CONNECTION_POST = 1;

    // response generated by post
    std::stringstream response_stream;

    // HTTP code of response
    int httpcode;

    // URL
    string url;

    // Post processor struct
    struct MHD_PostProcessor *postprocessor;
    // Type of request/connection
    int connection_type;

    // httpd parent
    Kis_Net_Httpd *httpd;    

    // Handler
    Kis_Net_Httpd_Handler *httpdhandler;    

    // Session
    Kis_Net_Httpd_Session *session;
};

class Kis_Net_Httpd_Session {
public:
    // Session ID
    string sessionid;

    // Time session was created
    time_t session_created;

    // Last time the session was seen active
    time_t session_seen;

    // Amount of time session is valid for after last active
    time_t session_lifetime;
};

class Kis_Net_Httpd : public LifetimeGlobal {
public:
    static shared_ptr<Kis_Net_Httpd> create_httpd(GlobalRegistry *in_globalreg) {
        shared_ptr<Kis_Net_Httpd> mon(new Kis_Net_Httpd(in_globalreg));
        in_globalreg->httpd_server = mon.get();
        in_globalreg->RegisterLifetimeGlobal(mon);
        in_globalreg->InsertGlobal("HTTPD_SERVER", mon);
        return mon;
    }

private:
    Kis_Net_Httpd(GlobalRegistry *in_globalreg);

public:
    virtual ~Kis_Net_Httpd();

    int StartHttpd();
    int StopHttpd();

    bool HttpdRunning() { return running; }
    unsigned int FetchPort() { return http_port; };
    bool FetchUsingSSL() { return use_ssl; };

    void RegisterHandler(Kis_Net_Httpd_Handler *in_handler);
    void RemoveHandler(Kis_Net_Httpd_Handler *in_handler);

    static string GetSuffix(string url);
    static string StripSuffix(string url);

    void RegisterMimeType(string suffix, string mimetype);
    string GetMimeType(string suffix);

    bool HasValidSession(struct MHD_Connection *connection);
    bool HasValidSession(Kis_Net_Httpd_Connection *connection);
    void CreateSession(struct MHD_Response *response, time_t in_lifetime);

    // Generic response sender
    static int SendHttpResponse(Kis_Net_Httpd *httpd,
            struct MHD_Connection *connection, 
            const char *url, int httpcode, string responsestr);

    // Catch MHD panics and try to close more elegantly
    static void MHD_Panic(void *cls, const char *file, unsigned int line,
            const char *reason);

protected:
    GlobalRegistry *globalreg;

    unsigned int http_port;
    string http_data_dir, http_aux_data_dir;

    bool http_serve_files, http_serve_user_files;

    struct MHD_Daemon *microhttpd;
    std::vector<Kis_Net_Httpd_Handler *> handler_vec;

    bool use_ssl;
    char *cert_pem, *cert_key;
    string pem_path, key_path;

    bool running;

    std::map<string, string> mime_type_map;

    pthread_mutex_t controller_mutex;

    // Handle the requests and dispatch to controllers
    static int http_request_handler(void *cls, struct MHD_Connection *connection,
            const char *url, const char *method, const char *version,
            const char *upload_data, size_t *upload_data_size, void **ptr);

    static void http_request_completed(void *cls, struct MHD_Connection *connection,
            void **con_cls, enum MHD_RequestTerminationCode toe);

    static int handle_static_file(void *cls, struct MHD_Connection *connection,
            const char *url, const char *method);

    static int http_post_handler(void *coninfo_cls, enum MHD_ValueKind kind, 
            const char *key, const char *filename, const char *content_type,
            const char *transfer_encoding, const char *data, 
            uint64_t off, size_t size);

    char *read_ssl_file(string in_fname);

    void AddSession(Kis_Net_Httpd_Session *in_session);
    void DelSession(string in_key);
    void DelSession(map<string, Kis_Net_Httpd_Session *>::iterator in_itr);
    void WriteSessions();

    map<string, Kis_Net_Httpd_Session *> session_map;

    bool store_sessions;
    string sessiondb_file;
    ConfigFile *session_db;

};

#endif

