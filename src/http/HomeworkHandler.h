#pragma once

#include "http/HttpRequest.hh"
#include "http/HttpResponse.hh"

#include <string>

class HomeworkHandler {
public:
    static void handle(const HttpRequest& req, HttpResponse* resp);
    
private:
    static void handlePost(const HttpRequest& req, HttpResponse* resp);
    static void handleGet(const HttpRequest& req, HttpResponse* resp);
    static std::string getFileType(const std::string& filePath);
};