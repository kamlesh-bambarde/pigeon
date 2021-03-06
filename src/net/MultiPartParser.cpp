//
// Created by kamlesh on 25/3/16.
//


#include <iterator>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <regex>
#include "net/MultiPartParser.h"

using namespace pigeon::net;

enum state {
    header_line_start,
    header_name,
    space_before_header_value,
    header_value,
    expecting_newline_2,
    expecting_newline_3,
    body_start
} state_;

enum param_state {
    param_start,
    param_name,
    param_value
} param_state_;

//parse the multipart data and separate the file contents and the headers
Form MultiPartParser::ParsePart(string data, string &boundary) {
    state_ = header_line_start;
    Form form_data;
    string temp;
    string val;
    char c;

    string::iterator begin = data.begin();
    string::iterator end = data.end();

    while (begin != end) {

        if (state_ != body_start) {
            c = *begin++;
        }

        switch (state_) {

            case header_line_start:
                if (c == '\r') {
                    state_ = expecting_newline_3;
                }
                else {
                    temp.push_back(c);
                    state_ = header_name;
                }
                break;

            case header_name:
                if (c == ':') {
                    state_ = space_before_header_value;
                }
                else {
                    temp.push_back(c);
                }
                break;

            case space_before_header_value:
                if (c == ' ') {
                    state_ = header_value;
                }
                break;
            case header_value:
                if (c == '\r') {
                    form_data.headers.emplace(std::pair<string, string>(temp, val));
                    temp.clear();
                    val.clear();
                    state_ = expecting_newline_2;
                }
                else {
                    val.push_back(c);
                }
                break;
            case expecting_newline_2:
                if (c == '\n') {
                    state_ = header_line_start;
                }
                break;
            case expecting_newline_3:
                if (c == '\n') {
                    state_ = body_start;
                }

                break;
            case body_start:
                string body_with_boundary = string(begin, end);
                unsigned long pos = body_with_boundary.find(boundary);
                for (unsigned long actualPos = pos; actualPos <= pos; --actualPos) {
                    if (body_with_boundary[actualPos] == '\r') {
                        pos = actualPos;
                        break;
                    }
                }
                string body_without_boundary = body_with_boundary.substr(0, pos);
                form_data.file_data = body_without_boundary;

                begin = end;
                break;

        }
    }

	//get the Content-Disposition header to get attributes like filename etc.
    string cont_disp = form_data.headers["Content-Disposition"];
	
	//parse all attributes of Content-Disposition and add them to the
	//parameters map, which can be used while handling the writting of
	//file on the disk
    if (cont_disp.size() > 0) {
        param_state_ = param_start;
        string key, val;

        for (auto &c:cont_disp) {

            switch (param_state_) {

                case param_start:
                    if (c == ';') {
                        param_state_ = param_name;
                    }
                    break;
                case param_name:
                    if (c == '=') {
                        param_state_ = param_value;
                    } else {
                        if (c != ' ') {
                            key.push_back(c);
                        }
                    }
                    break;
                case param_value:
                    if (c == ';') {
                        param_state_ = param_name;
                        form_data.parameters.emplace(std::pair<string, string>(key, val));
                        key.clear();
                        val.clear();
                    } else {
                        if (c != '"') {
                            val.push_back(c);
                        }
                    }
                    break;

            }
        }
        //adding last key value, since it cannot be captured in the loop
        if(key.size() > 0 && val.size() > 0){
            form_data.parameters.emplace(std::pair<string, string>(key, val));
            key.clear(); val.clear();
        }
    }

    return form_data;
}

void MultiPartParser::Parse(HttpContext *context, string _boundary) {

    boundary = _boundary;

    string parts(context->Request->content.begin(), context->Request->content.end());

	//split all the files that are uploaded into separate strings
	//and add them to a vector, so that each can be parsed individually
    unsigned long start_pos = parts.find(boundary);
    while (start_pos != string::npos) {
        unsigned long end_pos = parts.find(boundary, start_pos + boundary.size());
        if (end_pos != string::npos) {
            unsigned long start_from = start_pos + boundary.size() + 2;
            string part = parts.substr(start_from, end_pos - 2);
            file_contents.push_back(part);
        }
        start_pos = parts.find(boundary, end_pos);
    }

    for (auto &str: file_contents) {
        Form fr = ParsePart(str, boundary);
        //browser also posting the submit button as multipart/form-data, 
        //the Content-Disposition header though, didn't contain the filename attribute
        //hence added the below check is filename doesn't exists then don't add that part 
        //in forms collection
        if(fr.parameters["filename"].size() != 0) {
            context->Request->forms.push_back(fr);
        }
    }


}


