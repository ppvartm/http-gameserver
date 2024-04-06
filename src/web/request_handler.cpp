#include "request_handler.h"




namespace http_handler {
    

    json::value MapNotFound() {
        json::value val = {
                    {"code", "mapNotFound"},
                    {"message","Map not found"} };
        return val;
    }

    json::value BadRequest() {
        json::value val = {
                   {"code", "badRequest"},
                   {"message","Bad request"} };
        return val;
    }

    json::value EmptyNickname() {
        json::value val = {
                         {"code", "invalidArgument"},
                         {"message","Invalid Name"} };
        return val;
    }

    json::value JsonParseError() {
        json::value val = {
                        {"code", "invalidArgument"},
                        {"message","Join game request parse error"} };
        return val;
    }

    json::value NotPostRequest() {
        json::value val = {
                               {"code", "invalidMethod"},
                               {"message","Only POST method is expected"} };
        return val;
    }

    json::value AuthorizationMissing() {
        json::value val = {
            {"code", "invalidToken"},
            {"message","Authorization header is missing"}};
        return val;
    }

    json::value PlayerNotFound() {
        json::value val = {
           {"code", "unknownToken"},
           {"message","Player token has not been found"} };
        return val;
    }

    json::value InvalidMethod() {
        json::value val = {
           {"code", "invalidMethod"},
           {"message","Invalid method"} };
        return val;
    }

    json::value InvalidContentType() {
        json::value val = {
                   {"code", "invalidArgument"},
                   {"message","Invalid content type"} };
        return val;
    }

    json::value ErrorParseAction() {
        json::value val = {
           {"code", "invalidArgument"},
           {"message","Failed to parse action"} };
        return val;
    }

    json::value ErrorParseTick(){
        json::value val = {
          {"code", "invalidArgument"},
          {"message","Failed to parse tick request JSON"} };
        return val;
    }

    std::string GetMaps(const model::Game& game) {
        std::vector<MapInfo> maps_info;
        auto maps = game.GetMaps();
        for (int i = 0; i < maps.size(); ++i)
            maps_info.emplace_back(maps[i]);
        std::string answ = json::serialize(json::value_from(maps_info));
        return answ;
    }
   
    std::string GetFileType(std::string file_name) {
        std::string answ;
        auto it = file_name.rbegin();
        while (*it != '.' && it !=  file_name.rend()) {
            *it = std::tolower(*it);
            answ = *(it++) + answ;
        }
        if (answ == "htm" || answ == "html")
            return "text/html";
        if (answ == "css")
            return "text/css";
        if (answ == "txt")
            return "text/plain";
        if (answ == "js")
            return "text/javascript";
        if (answ == "json")
            return "application/json";
        if (answ == "xml")
            return "application/xml";
        if (answ == "png")
            return "image/png";
        if (answ == "jpg" || answ == "jpeg" || answ == "jpe")
            return "image/jpeg";
        if (answ == "gif")
            return "image/gif";
        if (answ == "bmp")
            return "image/bmp";
        if (answ == "ico")
            return "image/vnd.microsoft.icon";
        if (answ == "tiff" || answ == "tif")
            return "image/tiff";
        if (answ == "svg" || answ == "svgz")
            return "image/svg+xml";
        if (answ == "mp3")
            return "audio/mpeg";
        return "application/octet-stream";
    }

    bool IsFileExist(const fs::path& file_path) {
        return std::filesystem::exists(file_path);
    }

    bool IsAccessibleFile(fs::path file_path, fs::path base_path) {
        file_path = fs::weakly_canonical(file_path);
        base_path = fs::weakly_canonical(base_path);

        for (auto b = base_path.begin(), p = file_path.begin(); b != base_path.end(); ++b, ++p) {
            if (p == file_path.end() || *p != *b) {
                return false;
            }
        }
        return true;
    }

    http::file_body::value_type ReadFile(const fs::path& file_path) {
        http::file_body::value_type file;
        boost::system::error_code ec;
        file.open(file_path.string().data(), beast::file_mode::read, ec);
        if (ec)
          throw("Failed to open static-file: " + ec.message());
        return file;
    }

    std::string UrlDeCode(const std::string& url_path) {
        std::string answ;
        auto it = url_path.begin();
        while (it != url_path.end()) {
            if (*it == '+') {
                answ.push_back(' ');
                ++it;
                continue;
            }
            if (*it == '%') {
                std::string temp;
                temp.push_back(*(++it));
                temp.push_back(*(++it));
                char* p_end{};
                answ.push_back(static_cast<char>(std::strtol(temp.data(), &p_end, 16)));
                ++it;
                continue;
            }
            answ.push_back(*(it++));
        }
        return answ;
    }

    void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, MapInfo const& map_info) {
        jv = {
            {"id", *map_info.id_},
            {"name", map_info.name_}
        };
    }


    FileResponse RequestHandler::HEADMakeFileResponse(http::status status, fs::path file_path, unsigned http_version, bool keep_alive, std::string content_type) {
        FileResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.content_length(0);
        return response;
    }

    StringResponse RequestHandler::MakeStringResponse(http::status status, std::string_view body, unsigned http_version, bool keep_alive, boost::beast::string_view content_type) {
        StringResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = body;
        response.content_length(body.size());
        response.keep_alive(keep_alive);
        return response;
    }

    FileResponse RequestHandler::GETMakeFileResponse(http::status status, fs::path file_path, unsigned http_version, bool keep_alive, std::string content_type) {
        FileResponse response(status, http_version);
        response.set(http::field::content_type, content_type);
        response.body() = ReadFile(file_path);
        response.prepare_payload();
        return response;
    }

    void RequestHandler::SetFilePath(fs::path file_path) {
        path_ = file_path;
    }

    void RequestHandler::SetSerializationParams(double is_save, double is_auto_save, double save_interval, std::filesystem::path state_file_path) {
        serializating_listener_.SetParams(is_save, is_auto_save, save_interval, state_file_path);
    }

    bool RequestHandler::CheckAuthorization(const app::Token& tocken) {
        return static_cast<bool>(tokens_.FindPlayerByToken(tocken));
    }
}   // namespace http_handler
