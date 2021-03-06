//
// Created by kamlesh on 9/5/16.
//

#include <ctime>
#include <chrono>
#include <sstream>
#include <iterator>
#include <zlib.h>
#include "net/HttpUtil.h"

using  namespace pigeon::net;


struct logtype {
    int log_id;
    const char *log_type;
} logtypes[] =
        {
                {1, "[WARNING]"},
                {2, "[INFORMATION]"},
                {3, "[ERROR]"},
                {0, 0}
        };

struct severity {
    int severity_id;
    const char *severity_type;
} severities[] =
        {
                {1, "[LOW]"},
                {2, "[MEDIUM]"},
                {3, "[HIGH]"},
                {4, "[CRITICAL]"},
                {0, 0}
        };

struct header {
    int header_id;
    const char *header_field;
} headers[] =
        {
                {1, "Content-Type: "},
                {2, "Content-Length: "},
                {3, "Last-Modified: "},
                {4, "ETag: "},
                {5, "Content-Encoding: deflate\r\n"},
                {6, "Set-Cookie: "},
                {7, "Authorization:"},
                {8, "Access-Control-Allow-Origin:"},
                {9, "Access-Control-Allow-Methods:"},
                {10, "Access-Control-Allow-Headers:"},
                {11, "Access-Control-Max-Age:"},
                {12, "Access-Control-Allow-Credentials:"},
                {13, "Access-Control-Expose-Headers:"},
                {14, "Vary"},
                {15, "Keep-Alive"},
                {16, "Connection"},
                {0, 0}
        };

struct statusphrase {
    int status_code;
    const char *status_phrase;
} statusphrases[] =
        {
                {100, "HTTP/1.1 100 Continue"},
                {101, "HTTP/1.1 101 Switching Protocols"},
                {103, "HTTP/1.1 103 Checkpoint"},
                {200, "HTTP/1.1 200 OK"},
                {201, "HTTP/1.1 201 Created"},
                {202, "HTTP/1.1 202 Accepted"},
                {203, "HTTP/1.1 203 Non-Authoritative-Information"},
                {204, "HTTP/1.1 204 No Content"},
                {205, "HTTP/1.1 205 Reset Content"},
                {206, "HTTP/1.1 206 Partial Content"},
                {300, "HTTP/1.1 300 Multiple Choices"},
                {301, "HTTP/1.1 301 Moved Permanently"},
                {302, "HTTP/1.1 302 Found"},
                {303, "HTTP/1.1 303 See Other"},
                {304, "HTTP/1.1 304 Not Modified"},
                {306, "HTTP/1.1 306 Switch Proxy"},
                {307, "HTTP/1.1 307 Temporary Redirect"},
                {308, "HTTP/1.1 308 Resume Incomplete"},
                {400, "HTTP/1.1 400 Bad Request"},
                {401, "HTTP/1.1 401 Unauthorized"},
                {402, "HTTP/1.1 402 Payment Required"},
                {403, "HTTP/1.1 403 Forbidden"},
                {404, "HTTP/1.1 404 Not Found"},
                {405, "HTTP/1.1 405 Method Not Allowed"},
                {406, "HTTP/1.1 406 Not Acceptable"},
                {407, "HTTP/1.1 407 Proxy Authentication Required"},
                {408, "HTTP/1.1 408 Request Timeout"},
                {409, "HTTP/1.1 409 Conflict"},
                {410, "HTTP/1.1 410 Gone"},
                {411, "HTTP/1.1 411 Length Required"},
                {412, "HTTP/1.1 412 Precondition Failed"},
                {413, "HTTP/1.1 413 Request Entity Too Large"},
                {414, "HTTP/1.1 414 Request URI Too Large"},
                {415, "HTTP/1.1 415 Unsupported Media Type"},
                {416, "HTTP/1.1 416 Requested Range Not Satisfiable"},
                {417, "HTTP/1.1 417 Expectation Failled"},
                {500, "HTTP/1.1 500 Internal Server Error"},
                {501, "HTTP/1.1 501 Not Implemented"},
                {502, "HTTP/1.1 502 Bad Gateway"},
                {503, "HTTP/1.1 503 Service Unavailable"},
                {504, "HTTP/1.1 504 Gateway Timeout"},
                {505, "HTTP/1.1 505 HTTP Version Not Supported"},
                {511, "HTTP/1.1 511 Network Authentication Required"},
                {0,   0}
        };

struct statusmsg {
    int status_code;
    const char *status_msg;
} statusmsgs[] =
        {
                {100, "Continue"},
                {101, "Switching Protocols"},
                {103, "Checkpoint"},
                {200, "OK"},
                {201, "Created"},
                {202, "Accepted"},
                {203, "Non-Authoritative-Information"},
                {204, "No Content"},
                {205, "Reset Content"},
                {206, "Partial Content"},
                {300, "Multiple Choices"},
                {301, "Moved Permanently"},
                {302, "Found"},
                {303, "See Other"},
                {304, "Not Modified"},
                {306, "Switch Proxy"},
                {307, "Temporary Redirect"},
                {308, "Resume Incomplete"},
                {400, "Bad Request"},
                {401, "Unauthorized"},
                {402, "Payment Required"},
                {403, "Forbidden"},
                {404, "Not Found"},
                {405, "Method Not Allowed"},
                {406, "Not Acceptable"},
                {407, "Proxy Authentication Required"},
                {408, "Request Timeout"},
                {409, "Conflict"},
                {410, "Gone"},
                {411, "Length Required"},
                {412, "Precondition Failed"},
                {413, "Request Entity Too Large"},
                {414, "Request URI Too Large"},
                {415, "Unsupported Media Type"},
                {416, "Requested Range Not Satisfiable"},
                {417, "Expectation Failled"},
                {500, "Internal Server Error"},
                {501, "Not Implemented"},
                {502, "Bad Gateway"},
                {503, "Service Unavailable"},
                {504, "Gateway Timeout"},
                {505, "HTTP Version Not Supported"},
                {511, "Network Authentication Required"},
                {0,   0}
        };

struct mapping {
    const char *extension;
    const char *mime_type;
} mappings[] =
        {
                {"html",  "text/html; charset=UTF-8"},
                {"htm",   "text/html; charset=UTF-8"},
                {"htmls", "text/html; charset=UTF-8"},
                {"jpe",   "image/jpeg"},
                {"jpeg",  "image/jpeg"},
                {"jpg",   "image/jpeg"},
                {"js",    "application/javascript; charset=UTF-8"},
                {"jsonp", "application/javascript; charset=UTF-8"},
                {"json",  "application/json; charset=UTF-8"},
                {"map",   "application/json; charset=UTF-8"},
                {"gif",   "image/gif"},
                {"css",   "text/css; charset=UTF-8"},
                {"gz",    "application/x-gzip"},
                {"gzip",  "multipart/x-gzip"},
                {"ico",   "image/x-icon"},
                {"png",   "image/png"},
                {0,       0}
        };


const char* cached_date_response = "\r\nDate: ";
const char* err_cached_response = "\r\nConnection: keep-alive\r\nServer: pigeon\r\nAccept_Range: bytes\r\nContent-Type: text/html; charset=UTF-8\r\n";
const char* api_cached_response = "\r\nConnection: keep-alive\r\nServer: pigeon\r\nAccept_Range: bytes\r\nContent-Type: application/json\r\n";

const char *err_msg1 = "<!DOCTYPE html><html><head lang='en'><meta charset='UTF-8'><title>Status</title></head><body><p/><p/><p/><p/><p/><p/><p/><table align=\"center\" style=\"font-family: monospace;font-size: large;background-color: lemonchiffon;border-left-color: green;border-color: red;\"><tr style=\"background: burlywood;\"><th>Status Code</th><th>Message</th></tr><tr><td>";
const char *err_msg3 = "</td><td>";
const char *err_msg5 = "</td></tr><tr><td>Description: </td><td>";
const char *err_msg7 = "</td></tr></table></body></html>";

const char* api_err_msg1 = "{ \"Status\":";
const char* api_err_msg2 = ", \"StatusDescription:\"";
const char* api_err_msg3 = ", \"ErrorDescription\":";
const char* api_err_msg4 = "}";

char *pigeon::net::Now() {

    time_t now = time(0);
    char *dt;

    tm *gmtm = gmtime(&now);
    dt = asctime(gmtm);
    dt[strlen(dt) - 1] = '\0';

    return dt;
}

auto pigeon::net::GetCachedResponse(bool is_api, StringBuilder *sb) -> void {

    sb->Append((char *) cached_date_response);
    char* ts = Now();
    sb->Append(ts);

    if (is_api) {
        sb->Append((char *) api_cached_response);
    }
}

auto pigeon::net::GetHeaderField(HttpHeader hdr) -> const char* {

    for (header *m = headers; m->header_id; ++m) {
        if (m->header_id == static_cast<int>(hdr)) {
            return m->header_field;
        }
    }

    return  "unknown header";
}

auto pigeon::net::GetHeaderField(HttpHeader hdr, string &data) -> void {
    for (header *m = headers; m->header_id; ++m) {
        if (m->header_id == static_cast<int>(hdr)) {
            data += m->header_field;
        }
    }
}

auto pigeon::net::GetStatusPhrase(HttpStatus status, StringBuilder *sb) -> void {

    for (statusphrase *m = statusphrases; m->status_code; ++m) {
        if (m->status_code == static_cast<int>(status)) {
            sb->Append((char *) m->status_phrase);
        }
    }
}

auto pigeon::net::GetStatusMessage(HttpStatus status) -> const char* {

    for (statusmsg *m = statusmsgs; m->status_code; ++m) {
        if (m->status_code == static_cast<int>(status)) {
            return m->status_msg;
        }
    }
    return "Unknown";
}

auto pigeon::net::GetErrorMessage(const char *msg, bool is_api, HttpStatus status, StringBuilder *sb) -> void {

    string headers;
    string message;

    sb->Append((char *) err_cached_response);
    if(is_api){
        message.append(err_msg1);
        char* stat = (char*)std::to_string((int) status).c_str();
        message.append(stat);
        message.append(err_msg3);
        char* stat_msg = (char*) GetStatusMessage(status);
        message.append(stat_msg);
        message.append(err_msg5);
        message.append(msg);
        message.append(err_msg7);
    } else {
        message.append(api_err_msg1);
        char* stat = (char*)std::to_string((int) status).c_str();
        message.append(stat);
        message.append(api_err_msg2);
        char* stat_msg = (char*) GetStatusMessage(status);
        message.append(stat_msg);
        message.append(api_err_msg3);
        message.append(msg);
        message.append(api_err_msg4);
    }

    headers += GetHeaderField(HttpHeader::Content_Length);
    size_t length = message.size();
    sb->Append((char *) headers.c_str(), headers.size());
    sb->Append((char *) std::to_string(length).c_str());
    sb->Append((char *) "\r\n\r\n");
    sb->Append((char *) message.c_str(), message.size());

}

auto pigeon::net::GetMimeType(string &extension) -> const string {

    for (mapping *m = mappings; m->extension; ++m) {
        if (m->extension == extension) {
            return m->mime_type;
        }
    }

    return string("text/plain");

}

auto pigeon::net::GetLogType(LogType type) -> const string {

    for (logtype *m = logtypes; m->log_id; ++m) {
        if (m->log_id == static_cast<int>(type)) {
            return m->log_type;
        }
    }

    return "unknown log type";
}

auto pigeon::net::GetSeverity(Severity severe) -> const string {
    for (severity *m = severities; m->severity_id; ++m) {
        if (m->severity_id == static_cast<int>(severe)) {
            return m->severity_type;
        }
    }

    return "unknown severity type";
}

auto pigeon::net::UrlDecode(const string &in, string &out) -> bool {

    out.clear();
    out.reserve(in.size());
    for (std::size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '%') {
            if (i + 3 <= in.size()) {
                int value = 0;
                std::istringstream is(in.substr(i + 1, 2));
                if (is >> std::hex >> value) {
                    out += static_cast<char>(value);
                    i += 2;
                }
                else {
                    return false;
                }
            }
            else {
                return false;
            }
        }
        else if (in[i] == '+') {
            out += ' ';
        }
        else {
            out += in[i];
        }
    }
    return true;

}

auto pigeon::net::DeflateString(string &in, string &out) -> unsigned long {

    z_stream zs;                        // z_stream is zlib's control structure
    memset(&zs, 0, sizeof(zs));
    int compressionlevel = Z_BEST_COMPRESSION;
    if (deflateInit(&zs, compressionlevel) != Z_OK)
        throw (std::runtime_error("deflateInit failed while compressing."));

    zs.next_in = (Bytef *) in.data();
    zs.avail_in = static_cast<unsigned int>(in.size());           // set the z_stream's input

    int ret;
    char outbuffer[32768];
    std::string outstring;

    // retrieve the compressed bytes blockwise
    do {
        zs.next_out = reinterpret_cast<Bytef *>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = deflate(&zs, Z_FINISH);

        if (outstring.size() < zs.total_out) {
            // append the block to the output string
            outstring.append(outbuffer, zs.total_out - outstring.size());
        }
    } while (ret == Z_OK);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
        std::ostringstream oss;
        oss << "Exception during zlib compression: (" << ret << ") " << zs.msg;
        throw (std::runtime_error(oss.str()));
    }

    zs.avail_out = zs.total_out;// outstring.size();
    out = outstring;

    return outstring.size();
}