#include "httplib.h"
#include <iostream>
#include <cstring>
#include <future>
#include <nlohmann/json.hpp> // JSONライブラリ
#include <sstream>
#include <string>
#include <sqlite3.h>
#include <thread>
#include <vector>
#include <mutex>
#include <sstream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <uuid/uuid.h>

#include "include/models.hpp"
#include "include/BackupDb.hpp"
#include "include/MemDatabase.hpp"

using json = nlohmann::json;

std::string BACKUP_DB_PATH = "./data/backup.db";
std::string BACKUP_SQL_PATH = "./data/backup.sql";
std::string LOG_FILE_WRITE = "0";

static int               g_next_id = 1;
static std::mutex        g_mutex;

MemDatabase memDb;

std::string readFileToString(const std::string& filePath) {
    std::ifstream file(filePath);
    
    // ファイルが開けたか確認
    if (!file.is_open()) {
        std::cerr << "エラー: ファイルを開けませんでした -> " << filePath << std::endl;
        return "";
    }

    // ファイルバッファを stringstream に読み込む
    std::ostringstream ss;
    ss << file.rdbuf();
    
    return ss.str();
}

bool extract_bool(const std::string& json, const std::string& key, bool def = false) {
    std::string pattern = "\"" + key + "\":";
    auto pos = json.find(pattern);
    if (pos == std::string::npos) return def;
    pos += pattern.size();
    return json.substr(pos, 4) == "true";
}

void backup_handle() {
    try{   
        BackupDb bLib(BACKUP_DB_PATH);
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            std::vector<QueItem> vec1 = memDb.cache_select_list();
            std::cout << "vec.size()=" << vec1.size() << std::endl; 
            if (vec1.size() > 0){
                for (const auto& citem : vec1) {
                    std::cout << "citem.uuid=" << citem.uuid << std::endl; 
                    std::cout << "citem.sql=" << citem.sql << std::endl;
                    /*
                    if(LOG_FILE_WRITE == "1"){
                        logger->info("sql=" +citem.sql);
                        logger->flush();
                    } 
                    */
                    bool ok = bLib.executeSql(citem.sql);
                    if(ok == false){
                        std::cerr << "error:, bLib.executeSql "<< std::endl;
                    }
                    memDb.cashe_delete(citem.uuid);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return;
    }
}
// ─────────────────────────────────────────
// main
// ─────────────────────────────────────────
int main() {
    httplib::Server svr;

    try{    
        std::string content = readFileToString(BACKUP_SQL_PATH);
        std::cout << "--- SQL-FILE-TEXT ---" << std::endl;
        std::cout << content << std::endl;  
        bool ret = memDb.init_import(content.c_str());  
        std::cout << "memDb.init_import.ret=" << ret << std::endl;  
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return -1;
    }

    svr.Post("/api/select", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(g_mutex);
        // 1. Content-Typeの確認
        if (req.get_header_value("Content-Type") != "application/json") {
            res.status = 400;
            res.set_content("Expected application/json", "text/plain");
            return;
        }        
        try{
            // 2. JSONデコード (req.body をパース)
            json j = json::parse(req.body);
            // 3. データの取り出し (例: {"name": "Gopher", "id": 123})
            std::string action_name = j.at("action_name").get<std::string>();
            std::cout << "action_name=" << action_name << "\n";
            std::string table = j.at("table").get<std::string>();
            std::cout << "table=" << table << "\n";
            std::string sql = j.at("sql").get<std::string>();
            std::cout << "sql=" << sql << "\n";
            json j2 = memDb.selectTableSql(table, sql);
            std::string outStr = j2.dump();
            std::cout << outStr << std::endl;

            //validate
            SearchListResp re1;
            re1.ret_code = 200;
            re1.data = outStr;
            json j1 = re1;
            std::string json_str = j1.dump();
            std::cout << json_str << std::endl;            

            res.status = 200;
            res.set_content(json_str, "application/json");
        } catch (const std::exception& e) {
            std::cout << "\n[ERROR] " << e.what() << "\n";
            // キーが存在しない場合など
            res.status = 500;
            res.set_content("Internal Server Error", "text/plain");
        }        
    });

    svr.Post("/api/update", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(g_mutex);
        // 1. Content-Typeの確認
        if (req.get_header_value("Content-Type") != "application/json") {
            res.status = 400;
            res.set_content("Expected application/json", "text/plain");
            return;
        }        
        try{
            // 2. JSONデコード (req.body をパース)
            json j = json::parse(req.body);
            // 3. データの取り出し (例: {"name": "Gopher", "id": 123})
            std::string action_name = j.at("action_name").get<std::string>();
            std::cout << "action_name=" << action_name << "\n";
            std::string table = j.at("table").get<std::string>();
            std::cout << "table=" << table << "\n";
            std::string sql = j.at("sql").get<std::string>();
            std::cout << "sql=" << sql << "\n";

            uuid_t uuid;
            char uuid_str[37];
            uuid_generate(uuid);
            uuid_unparse(uuid, uuid_str);
            std::cout << "UUID: " << uuid_str << std::endl;

            std::cout << "UUID: " << uuid_str << std::endl;
            QueItem que;
            que.uuid = uuid_str;
            que.sql = sql; 

            bool success = memDb.executeSql(sql);
            bool ok_cache = memDb.cache_add(uuid_str, sql);                       
            //validate

            SearchListResp re1;
            re1.ret_code = 200;
            re1.data = "OK";
            json j1 = re1;
            std::string json_str = j1.dump();
            std::cout << json_str << std::endl;            

            res.status = 200;
            res.set_content(json_str, "application/json");
        } catch (const std::exception& e) {
            std::cout << "\n[ERROR] " << e.what() << "\n";
            // キーが存在しない場合など
            res.status = 500;
            res.set_content("Internal Server Error", "text/plain");
        }        
    });


    svr.Post("/api/extra", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(g_mutex);
        // 1. Content-Typeの確認
        if (req.get_header_value("Content-Type") != "application/json") {
            res.status = 400;
            res.set_content("Expected application/json", "text/plain");
            return;
        }        
        try{
            // 2. JSONデコード (req.body をパース)
            json j = json::parse(req.body);

            // 3. データの取り出し (例: {"name": "Gopher", "id": 123})
            /*
            std::string table = j.at("table").get<std::string>();
            std::cout << "table=" << table << "\n";
            std::string id = j.at("id").get<std::string>();
            std::cout << "id=" << id << "\n";
            */

            NormalRespopnse re1;
            re1.ret_code = 200;
            json j1 = re1; // 構造体を代入するだけ！
            std::string json_str = j1.dump();
            std::cout << json_str << std::endl;            

            res.status = 201;
            res.set_content(json_str, "application/json");
        } catch (const std::exception& e) {
            std::cout << "\n[ERROR] " << e.what() << "\n";
            // キーが存在しない場合など
            res.status = 500;
            res.set_content("Internal Server Error", "text/plain");
        }        
    });    

    // ── 起動 ────────────────────────────────
    std::thread th1(backup_handle);

    int port_no = 8888;
    std::cout << "API Server running on http://localhost:8888\n";

    svr.listen("0.0.0.0", port_no);
    return 0;
}
