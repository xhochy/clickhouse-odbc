#include "connection.h"
#include "environment.h"
#include "log.h"
#include "statement.h"
#include "utils.h"

//#include <malloc.h>

static RETCODE allocEnv(SQLHENV * out_environment) {
    if (nullptr == out_environment)
        return SQL_INVALID_HANDLE;
    try {
        *out_environment = new Environment;
        return SQL_SUCCESS;
    } catch (...) {
        LOG(__FUNCTION__ << " Exception");
        return SQL_ERROR;
    }
}

static RETCODE allocConnect(SQLHENV environment, SQLHDBC * out_connection) {
    LOG(__FUNCTION__ << " environment=" << environment << " out_connection=" << out_connection);
    if (nullptr == out_connection)
        return SQL_INVALID_HANDLE;

    try {
        *out_connection = new Connection(*reinterpret_cast<Environment *>(environment));
        return SQL_SUCCESS;
    } catch (...) {
        LOG(__FUNCTION__ << " Exception");
        return SQL_ERROR;
    }
}

static RETCODE allocStmt(SQLHDBC connection, SQLHSTMT * out_statement) {
    if (nullptr == out_statement || nullptr == connection)
        return SQL_INVALID_HANDLE;

    try {
        *out_statement = new Statement(*reinterpret_cast<Connection *>(connection));
        return SQL_SUCCESS;
    } catch (...) {
        LOG(__FUNCTION__ << " Exception");
        return SQL_ERROR;
    }
}

template <typename Handle>
static RETCODE freeHandle(SQLHANDLE handle_opaque) {
    delete reinterpret_cast<Handle *>(handle_opaque);
    handle_opaque = nullptr;
    return SQL_SUCCESS;
}


extern "C" {

RETCODE SQL_API SQLAllocHandle(SQLSMALLINT handle_type, SQLHANDLE input_handle, SQLHANDLE * output_handle) {
    LOG(__FUNCTION__ << " handle_type=" << handle_type << " input_handle=" << input_handle);

    switch (handle_type) {
        case SQL_HANDLE_ENV:
            return allocEnv((SQLHENV *)output_handle);
        case SQL_HANDLE_DBC:
            return allocConnect((SQLHENV)input_handle, (SQLHDBC *)output_handle);
        case SQL_HANDLE_STMT:
            return allocStmt((SQLHDBC)input_handle, (SQLHSTMT *)output_handle);
        default:
            return SQL_ERROR;
    }
}

RETCODE SQL_API SQLAllocEnv(SQLHDBC * output_handle) {
    LOG(__FUNCTION__);
    return allocEnv(output_handle);
}

RETCODE SQL_API SQLAllocConnect(SQLHENV input_handle, SQLHDBC * output_handle) {
    LOG(__FUNCTION__ << " input_handle=" << input_handle);
    return allocConnect(input_handle, output_handle);
}

RETCODE SQL_API SQLAllocStmt(SQLHDBC input_handle, SQLHSTMT * output_handle) {
    LOG(__FUNCTION__ << " input_handle=" << input_handle);
    return allocStmt(input_handle, output_handle);
}


RETCODE SQL_API SQLFreeHandle(SQLSMALLINT handleType, SQLHANDLE handle) {
    LOG(__FUNCTION__ << " handleType=" << handleType <<" handle=" << handle);

    switch (handleType) {
        case SQL_HANDLE_ENV:
            return freeHandle<Environment>(handle);
        case SQL_HANDLE_DBC:
            return freeHandle<Connection>(handle);
        case SQL_HANDLE_STMT:
            return freeHandle<Statement>(handle);
        default:
            LOG("FreeHandle: Unknown handleType=" << handleType);
            return SQL_ERROR;
    }
}


RETCODE SQL_API SQLFreeEnv(HENV handle) {
    LOG(__FUNCTION__);
    return freeHandle<Environment>(handle);
}

RETCODE SQL_API SQLFreeConnect(HDBC handle) {
    LOG(__FUNCTION__);
    return freeHandle<Connection>(handle);
}

RETCODE SQL_API SQLFreeStmt(HSTMT statement_handle, SQLUSMALLINT option) {
    LOG(__FUNCTION__);

    return doWith<Statement>(statement_handle, [&](Statement & statement) -> RETCODE {
        LOG("option: " + std::to_string(option));

        switch (option) {
            case SQL_DROP:
                return freeHandle<Statement>(statement_handle);

            case SQL_CLOSE: /// Close the cursor, ignore the remaining results. If there is no cursor, then noop.
                statement.reset();

            case SQL_UNBIND:
                statement.bindings.clear();
                return SQL_SUCCESS;

            case SQL_RESET_PARAMS:
                return SQL_SUCCESS;

            default:
                return SQL_ERROR;
        }
    });
}
}
