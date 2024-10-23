# This file is included by DuckDB's build system. It specifies which extension to load

duckdb_extension_load(httpfs
	DONT_LINK
	SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}
	INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/extension/httpfs/include
	LOAD_TESTS)
