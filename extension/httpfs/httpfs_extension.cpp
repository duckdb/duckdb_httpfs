#define DUCKDB_EXTENSION_MAIN

#include "duckdb.hpp"
#include "duckdb/function/function_set.hpp"
#include "duckdb/main/extension_util.hpp"
#include "duckdb/parser/tableref/table_function_ref.hpp"
#include "duckdb/parser/expression/comparison_expression.hpp"
#include "duckdb/parser/expression/constant_expression.hpp"
#include "duckdb/parser/expression/function_expression.hpp"
#include "httpfs_extension.hpp"
#include "s3fs.hpp"

namespace duckdb {

void CreateS3Credentials(ClientContext &context, named_parameter_map_t named_parameters) {
	auto& opener = context.client_data->file_opener;
	FileOpenerInfo info;

	vector<string> scope_paths;
	Value scope;

	scope_paths = {"s3://"};
	scope =  Value::LIST({"s3://"});

	string filesystem_name = "HTTPFileSystem";
	auto params = S3AuthParams::ReadFrom(opener.get(), info);
	auto cred = make_shared<S3RegisteredCredential>(scope_paths, filesystem_name, params);
	cred->SetAlias("alias_hardcoded_1");

	// TODO return credentials instead of doing this
	context.db->config.credential_manager->RegisterCredentials(cred);
}

static void AddNamedParameters(TableFunction& table_function) {
	RegisteredCredential::AddNamedParametersToSetFunction(table_function);
}

static void RegisterSetCredentialFunction(DatabaseInstance &instance) {
	CreateSecretFunction create_secret_fun("s3", "", CreateS3Credentials);
	ExtensionUtil::RegisterFunction(instance, create_secret_fun);
}

static void LoadInternal(DatabaseInstance &instance) {
	S3FileSystem::Verify(); // run some tests to see if all the hashes work out
	auto &fs = instance.GetFileSystem();

	fs.RegisterSubSystem(make_uniq<HTTPFileSystem>());
	fs.RegisterSubSystem(make_uniq<S3FileSystem>(BufferManager::GetBufferManager(instance)));

	auto &config = DBConfig::GetConfig(instance);

	// Global HTTP config
	// Single timeout value is used for all 4 types of timeouts, we could split it into 4 if users need that
	config.AddExtensionOption("http_timeout", "HTTP timeout read/write/connection/retry (default 30000ms)",
	                          LogicalType::UBIGINT, Value(30000));
	config.AddExtensionOption("http_retries", "HTTP retries on I/O error (default 3)", LogicalType::UBIGINT, Value(3));
	config.AddExtensionOption("http_retry_wait_ms", "Time between retries (default 100ms)", LogicalType::UBIGINT,
	                          Value(100));
	config.AddExtensionOption("force_download", "Forces upfront download of file", LogicalType::BOOLEAN, Value(false));
	// Reduces the number of requests made while waiting, for example retry_wait_ms of 50 and backoff factor of 2 will
	// result in wait times of  0 50 100 200 400...etc.
	config.AddExtensionOption("http_retry_backoff",
	                          "Backoff factor for exponentially increasing retry wait time (default 4)",
	                          LogicalType::FLOAT, Value(4));
	// Global S3 config
	config.AddExtensionOption("s3_region", "S3 Region", LogicalType::VARCHAR);
	config.AddExtensionOption("s3_access_key_id", "S3 Access Key ID", LogicalType::VARCHAR);
	config.AddExtensionOption("s3_secret_access_key", "S3 Access Key", LogicalType::VARCHAR);
	config.AddExtensionOption("s3_session_token", "S3 Session Token", LogicalType::VARCHAR);
	config.AddExtensionOption("s3_endpoint", "S3 Endpoint (default 's3.amazonaws.com')", LogicalType::VARCHAR,
	                          Value("s3.amazonaws.com"));
	config.AddExtensionOption("s3_url_style", "S3 url style ('vhost' (default) or 'path')", LogicalType::VARCHAR,
	                          Value("vhost"));
	config.AddExtensionOption("s3_use_ssl", "S3 use SSL (default true)", LogicalType::BOOLEAN, Value(true));
	config.AddExtensionOption("s3_url_compatibility_mode", "Disable Globs and Query Parameters on S3 urls",
	                          LogicalType::BOOLEAN, Value(false));

	// S3 Uploader config
	config.AddExtensionOption("s3_uploader_max_filesize",
	                          "S3 Uploader max filesize (between 50GB and 5TB, default 800GB)", LogicalType::VARCHAR,
	                          "800GB");
	config.AddExtensionOption("s3_uploader_max_parts_per_file",
	                          "S3 Uploader max parts per file (between 1 and 10000, default 10000)",
	                          LogicalType::UBIGINT, Value(10000));
	config.AddExtensionOption("s3_uploader_thread_limit", "S3 Uploader global thread limit (default 50)",
	                          LogicalType::UBIGINT, Value(50));

	auto provider = make_uniq<AWSEnvironmentCredentialsProvider>(config);
	provider->SetAll();

	RegisterSetCredentialFunction(instance);
}

void HttpfsExtension::Load(DuckDB &db) {
	LoadInternal(*db.instance);
}
std::string HttpfsExtension::Name() {
	return "httpfs";
}

} // namespace duckdb

extern "C" {

DUCKDB_EXTENSION_API void httpfs_init(duckdb::DatabaseInstance &db) {
	LoadInternal(db);
}

DUCKDB_EXTENSION_API const char *httpfs_version() {
	return duckdb::DuckDB::LibraryVersion();
}
}

#ifndef DUCKDB_EXTENSION_MAIN
#error DUCKDB_EXTENSION_MAIN not defined
#endif
