#include <userver/clients/dns/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/utest/using_namespace_userver.hpp>

/// [Postgres service sample - component]
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/components/component_base.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/file_descriptor.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/handlers/http_handler_static.hpp>
#include <userver/http/content_type.hpp>

#include <iostream>
#include <filesystem>
#include <regex>
#include <string>
#include <vector>
#include <algorithm>

#include <samples_postgres_service/sql_queries.hpp>

namespace samples_postgres_service::pg {

// Компонент для обслуживания статических файлов
class StaticFilesHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-static";

    StaticFilesHandler(const components::ComponentConfig& config,
                      const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          file_path_prefix_(config["file_path_prefix"].As<std::string>()),
          fallback_file_path_(config["fallback_file_path"].As<std::string>("")) {}

    std::string HandleRequestThrow(
        const server::http::HttpRequest& request,
        server::request::RequestContext&) const override {
        
        const auto& path = request.GetRequestPath();
        
        // Проверяем, существует ли запрошенный файл
        std::string file_path = file_path_prefix_ + path;
        
        if (path == "/" || path.empty()) {
            file_path = file_path_prefix_ + "/index.html";
        }
        
        try {
            if (std::filesystem::exists(file_path) && 
                !std::filesystem::is_directory(file_path)) {
                return ServeFile(file_path, request);
            }
            
            // Если файл не найден и указан fallback_path, возвращаем его
            if (!fallback_file_path_.empty() && 
                std::filesystem::exists(fallback_file_path_)) {
                return ServeFile(fallback_file_path_, request);
            }
        } catch (const std::exception& ex) {
            LOG_ERROR() << "Error serving static file: " << ex.what();
        }
        
        // Если файл не найден и нет fallback, возвращаем 404
        request.SetResponseStatus(server::http::HttpStatus::kNotFound);
        return "File not found";
    }

private:
    std::string ServeFile(const std::string& file_path,
                         const server::http::HttpRequest& request) const {
        // Определяем Content-Type на основе расширения
        auto content_type = DetermineContentType(file_path);
        request.GetHttpResponse().SetContentType(content_type);
        
        // Возвращаем содержимое файла
        return fs::blocking::ReadFileContents(file_path);
    }
    
    std::string DetermineContentType(const std::string& file_path) const {
        static const std::unordered_map<std::string, std::string> content_types = {
            {".html", http::content_type::kTextHtml},
            {".css", http::content_type::kTextCss},
            {".js", http::content_type::kApplicationJavascript},
            {".json", http::content_type::kApplicationJson},
            {".png", http::content_type::kImagePng},
            {".jpg", http::content_type::kImageJpeg},
            {".jpeg", http::content_type::kImageJpeg},
            {".svg", http::content_type::kImageSvg},
            {".ico", "image/x-icon"}
        };
        
        std::string extension = std::filesystem::path(file_path).extension().string();
        auto it = content_types.find(extension);
        if (it != content_types.end()) {
            return it->second;
        }
        
        return http::content_type::kTextPlain;
    }
    
    std::string file_path_prefix_;
    std::string fallback_file_path_;
};

// Структура для хранения информации о миграции
struct Migration {
    std::string version;
    std::string filename;
    std::string sql;
};

// Компонент для инициализации схемы базы данных
class PostgresSchemaInit final : public components::ComponentBase {
public:
    // Имя компонента для использования в конфигурации
    static constexpr std::string_view kName = "postgres-schema-init";

    // Конструктор компонента
    PostgresSchemaInit(const components::ComponentConfig& config,
                      const components::ComponentContext& context);

private:
    // Метод для применения миграций
    void ApplyMigrations();
    
    // Метод для загрузки миграций из файлов
    std::vector<Migration> LoadMigrations(const std::string& migrations_dir);
    
    // Метод для чтения SQL-кода из файла
    std::string ReadSqlFile(const std::string& filepath);

    // Кластер PostgreSQL
    storages::postgres::ClusterPtr pg_cluster_;
};

PostgresSchemaInit::PostgresSchemaInit(const components::ComponentConfig& config,
                                      const components::ComponentContext& context)
    : ComponentBase(config, context),
      pg_cluster_(context.FindComponent<components::Postgres>("key-value-database").GetCluster()) {
    ApplyMigrations();
}

std::string PostgresSchemaInit::ReadSqlFile(const std::string& filepath) {
    try {
        return fs::blocking::ReadFileContents(filepath);
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Не удалось прочитать файл миграции: " << filepath
                  << ", ошибка: " << ex.what();
        throw;
    }
}

std::vector<Migration> PostgresSchemaInit::LoadMigrations(const std::string& migrations_dir) {
    std::vector<Migration> migrations;
    
    // Проверяем существование директории
    if (!std::filesystem::exists(migrations_dir)) {
        LOG_WARNING() << "Директория миграций не найдена: " << migrations_dir;
        return migrations;
    }
    
    // Регулярное выражение для извлечения версии миграции из имени файла
    std::regex version_regex("^(\\d+)_.*\\.sql$");
    
    // Перебираем все файлы в директории
    for (const auto& entry : std::filesystem::directory_iterator(migrations_dir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".sql") {
            continue;
        }
        
        std::string filename = entry.path().filename().string();
        std::smatch match;
        
        // Проверяем, соответствует ли имя файла формату NNN_description.sql
        if (std::regex_match(filename, match, version_regex)) {
            Migration migration;
            migration.version = match[1].str();  // Извлекаем номер версии
            migration.filename = filename;
            migration.sql = ReadSqlFile(entry.path().string());
            
            migrations.push_back(migration);
        } else {
            LOG_WARNING() << "Пропущен файл с неправильным форматом имени: " << filename;
        }
    }
    
    // Сортируем миграции по номеру версии
    std::sort(migrations.begin(), migrations.end(),
              [](const Migration& a, const Migration& b) {
                  return a.version < b.version;
              });
    
    return migrations;
}

void PostgresSchemaInit::ApplyMigrations() {
    // Создаем таблицу для отслеживания миграций
    pg_cluster_->Execute(
        storages::postgres::ClusterHostType::kMaster,
        "CREATE TABLE IF NOT EXISTS schema_migrations ("
        "    version VARCHAR PRIMARY KEY,"
        "    applied_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()"
        ")");
    
    // Путь к директории с миграциями
    const std::string migrations_dir = "schemas/postgresql/migrations";
    
    // Загружаем миграции из файлов
    auto migrations = LoadMigrations(migrations_dir);
    
    LOG_INFO() << "Найдено " << migrations.size() << " файлов миграций";
    
    // Применяем каждую миграцию
    for (const auto& migration : migrations) {
        // Проверяем, применена ли уже миграция
        auto res = pg_cluster_->Execute(
            storages::postgres::ClusterHostType::kMaster,
            "SELECT 1 FROM schema_migrations WHERE version = $1",
            migration.version);
        
        if (res.IsEmpty()) {
            LOG_INFO() << "Применение миграции " << migration.version 
                     << " (" << migration.filename << ")";
            
            try {
                // Выполняем SQL из файла миграции
                pg_cluster_->Execute(
                    storages::postgres::ClusterHostType::kMaster,
                    migration.sql);
                
                // Записываем информацию о применённой миграции
                pg_cluster_->Execute(
                    storages::postgres::ClusterHostType::kMaster,
                    "INSERT INTO schema_migrations (version) VALUES ($1)",
                    migration.version);
                
                LOG_INFO() << "Миграция " << migration.version << " успешно применена";
            } catch (const std::exception& ex) {
                LOG_ERROR() << "Ошибка при применении миграции " << migration.version
                          << ": " << ex.what();
                throw;
            }
        } else {
            LOG_DEBUG() << "Миграция " << migration.version << " уже применена";
        }
    }
    
    LOG_INFO() << "Все миграции применены";
}

class KeyValue final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-key-value";

    KeyValue(const components::ComponentConfig& config, const components::ComponentContext& context);

    std::string HandleRequest(server::http::HttpRequest& request, server::request::RequestContext&) const override;

private:
    std::string GetValue(std::string_view key, const server::http::HttpRequest& request) const;
    std::string PostValue(std::string_view key, const server::http::HttpRequest& request) const;
    std::string DeleteValue(std::string_view key) const;

    storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace samples_postgres_service::pg
/// [Postgres service sample - component]

namespace samples_postgres_service::pg {

/// [Postgres service sample - component constructor]
KeyValue::KeyValue(const components::ComponentConfig& config, const components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(context.FindComponent<components::Postgres>("key-value-database").GetCluster()) {}
/// [Postgres service sample - component constructor]

/// [Postgres service sample - HandleRequestThrow]
std::string KeyValue::HandleRequest(server::http::HttpRequest& request, server::request::RequestContext&) const {
    // Добавляем CORS заголовки для всех ответов
    request.GetHttpResponse().SetHeader(static_cast<std::string>("Access-Control-Allow-Origin"), static_cast<std::string>("*"));
    request.GetHttpResponse().SetHeader(static_cast<std::string>("Access-Control-Allow-Methods"), static_cast<std::string>("GET, POST, DELETE, OPTIONS"));
    request.GetHttpResponse().SetHeader(static_cast<std::string>("Access-Control-Allow-Headers"), static_cast<std::string>("Content-Type, Origin, Accept"));
    
    // Обработка предварительных запросов OPTIONS
    if (request.GetMethod() == server::http::HttpMethod::kOptions) {
        request.SetResponseStatus(server::http::HttpStatus::kOk);
        return {};
    }
    
    const auto& key = request.GetArg("key");
    if (key.empty()) {
        throw server::handlers::ClientError(server::handlers::ExternalBody{"No 'key' query argument"});
    }

    request.GetHttpResponse().SetContentType(http::content_type::kTextPlain);
    
    switch (request.GetMethod()) {
        case server::http::HttpMethod::kGet:
            return GetValue(key, request);
        case server::http::HttpMethod::kPost:
            return PostValue(key, request);
        case server::http::HttpMethod::kDelete:
            return DeleteValue(key);
        default:
            throw server::handlers::ClientError(server::handlers::ExternalBody{
                fmt::format("Unsupported method {}", request.GetMethod())});
    }
}
/// [Postgres service sample - HandleRequestThrow]

/// [Postgres service sample - GetValue]
std::string KeyValue::GetValue(std::string_view key, const server::http::HttpRequest& request) const {
    storages::postgres::ResultSet res =
        pg_cluster_->Execute(storages::postgres::ClusterHostType::kSlave, sql::kSelectValue, key);
    if (res.IsEmpty()) {
        request.SetResponseStatus(server::http::HttpStatus::kNotFound);
        return {};
    }

    return res.AsSingleRow<std::string>();
}
/// [Postgres service sample - GetValue]

/// [Postgres service sample - PostValue]
std::string KeyValue::PostValue(std::string_view key, const server::http::HttpRequest& request) const {
    const auto& value = request.GetArg("value");

    storages::postgres::Transaction transaction =
        pg_cluster_->Begin("sample_transaction_insert_key_value", storages::postgres::ClusterHostType::kMaster, {});

    auto res = transaction.Execute(sql::kInsertValue, key, value);
    if (res.RowsAffected()) {
        transaction.Commit();
        request.SetResponseStatus(server::http::HttpStatus::kCreated);
        return std::string{value};
    }

    res = transaction.Execute(sql::kSelectValue, key);
    transaction.Rollback();

    auto result = res.AsSingleRow<std::string>();
    if (result != value) {
        request.SetResponseStatus(server::http::HttpStatus::kConflict);
    }

    return res.AsSingleRow<std::string>();
}
/// [Postgres service sample - PostValue]

/// [Postgres service sample - DeleteValue]
std::string KeyValue::DeleteValue(std::string_view key) const {
    auto res = pg_cluster_->Execute(storages::postgres::ClusterHostType::kMaster, sql::kDeleteValue, key);
    return std::to_string(res.RowsAffected());
}
/// [Postgres service sample - DeleteValue]

}  // namespace samples_postgres_service::pg

/// [Postgres service sample - main]
int main(int argc, char* argv[]) {
    const auto component_list = components::MinimalServerComponentList()
                                    .Append<samples_postgres_service::pg::KeyValue>()
                                    .Append<components::Postgres>("key-value-database")
                                    .Append<samples_postgres_service::pg::PostgresSchemaInit>()
                                    .Append<samples_postgres_service::pg::StaticFilesHandler>()
                                    .Append<components::HttpClient>()
                                    .Append<components::TestsuiteSupport>()
                                    .Append<server::handlers::TestsControl>()
                                    .Append<clients::dns::Component>();
    return utils::DaemonMain(argc, argv, component_list);
}
/// [Postgres service sample - main]