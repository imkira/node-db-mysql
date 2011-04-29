// Copyright 2011 Mariano Iglesias <mgiglesias@gmail.com>
#include "./result.h"

node_db_mysql::Result::Column::Column(const MYSQL_FIELD& column) {
    this->binary = column.flags & BINARY_FLAG;
    this->name = column.name;

    switch (column.type) {
        case MYSQL_TYPE_TINY:
            this->type = (column.length == 1 ? BOOL : INT);
            break;
        case MYSQL_TYPE_BIT:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_YEAR:
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_LONGLONG:
            this->type = INT;
            break;
        case MYSQL_TYPE_FLOAT:
        case MYSQL_TYPE_DOUBLE:
        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_NEWDECIMAL:
            this->type = NUMBER;
            break;
        case MYSQL_TYPE_DATE:
            this->type = DATE;
            break;
        case MYSQL_TYPE_TIME:
            this->type = TIME;
            break;
        case MYSQL_TYPE_TIMESTAMP:
        case MYSQL_TYPE_DATETIME:
            this->type = DATETIME;
            break;
        case MYSQL_TYPE_TINY_BLOB:
        case MYSQL_TYPE_MEDIUM_BLOB:
        case MYSQL_TYPE_LONG_BLOB:
        case MYSQL_TYPE_BLOB:
            this->type = TEXT;
            break;
        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_VAR_STRING:
            this->type = this->binary ? TEXT : STRING;
            break;
        case MYSQL_TYPE_SET:
            this->type = SET;
            break;
        default:
            this->type = STRING;
            break;
    }
}

node_db_mysql::Result::Column::~Column() {
}

bool node_db_mysql::Result::Column::isBinary() const {
    return this->binary;
}

std::string node_db_mysql::Result::Column::getName() const {
    return this->name;
}

node_db::Result::Column::type_t node_db_mysql::Result::Column::getType() const {
    return this->type;
}

node_db_mysql::Result::Result(MYSQL* connection, MYSQL_RES* result) throw(node_db::Exception&)
    : columns(NULL),
    totalColumns(0),
    rowNumber(0),
    connection(connection),
    result(result),
    previousRow(NULL),
    nextRow(NULL) {
    if (this->result == NULL) {
        throw node_db::Exception("Invalid result");
    }

    MYSQL_FIELD* fields = mysql_fetch_fields(this->result);
    if (fields == NULL) {
        throw node_db::Exception("Could not buffer columns");
    }

    this->totalColumns = mysql_num_fields(this->result);
    if (this->totalColumns > 0) {
        this->columns = new Column*[this->totalColumns];
        if (this->columns == NULL) {
            throw node_db::Exception("Could not allocate storage for columns");
        }

        for (uint16_t i = 0; i < this->totalColumns; i++) {
            this->columns[i] = new Column(fields[i]);
        }
    }

    this->nextRow = this->row();
}

node_db_mysql::Result::~Result() {
    if (this->columns != NULL) {
        for (uint16_t i = 0; i < this->totalColumns; i++) {
            delete this->columns[i];
        }
        delete [] this->columns;
    }
    if (this->result != NULL) {
        mysql_free_result(this->result);
    }
}

bool node_db_mysql::Result::hasNext() const {
    return (this->nextRow != NULL);
}

const char** node_db_mysql::Result::next() throw(node_db::Exception&) {
    if (this->nextRow == NULL) {
        return NULL;
    }

    this->rowNumber++;
    this->previousRow = this->nextRow;
    this->nextRow = this->row();

    return (const char**) this->previousRow;
}

uint64_t* node_db_mysql::Result::columnLengths() throw(node_db::Exception&) {
    return mysql_fetch_lengths(this->result);
}

char** node_db_mysql::Result::row() throw(node_db::Exception&) {
    return mysql_fetch_row(this->result);
}

uint64_t node_db_mysql::Result::index() const throw(std::out_of_range&) {
    if (this->rowNumber == 0) {
        throw std::out_of_range("Not standing on a row");
    }
    return (this->rowNumber - 1);
}

node_db_mysql::Result::Column* node_db_mysql::Result::column(uint16_t i) const throw(std::out_of_range&) {
    if (i < 0 || i >= this->totalColumns) {
        throw std::out_of_range("Wrong column index");
    }
    return this->columns[i];
}

uint64_t node_db_mysql::Result::insertId() const {
    return mysql_insert_id(this->connection);
}

uint64_t node_db_mysql::Result::affectedCount() const {
    return mysql_affected_rows(this->connection);
}

uint16_t node_db_mysql::Result::warningCount() const {
    return mysql_warning_count(this->connection);
}

uint16_t node_db_mysql::Result::columnCount() const {
    return this->totalColumns;
}
